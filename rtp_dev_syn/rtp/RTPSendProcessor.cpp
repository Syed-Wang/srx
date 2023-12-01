#include "RTPSendProcessor.h"
#ifdef _WIN32
#include <winsock.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#endif // _WIN32

#include<string.h>
static unsigned int g_rtpSSRCStart = 0x60000000;

CRTPSendProcessor::CRTPSendProcessor() : m_pDataOut(NULL), m_maxPayloadSize(1200), m_payloadType(-1),
		m_codecType(enAgentCode_unKnow), m_pktSeq(0), m_timeStampInc(0), m_curTimeStamp(0)
{
	m_ssrc = g_rtpSSRCStart++;
}


CRTPSendProcessor::~CRTPSendProcessor()
{
}

void CRTPSendProcessor::SetDataOut(IDataIOCallback *pDataOut)
{
	m_pDataOut = pDataOut;
}

void CRTPSendProcessor::SetPayloadType(int type)
{
	m_payloadType = type;
	m_codecType = GetCodecTypeByPayload(type);
}

void CRTPSendProcessor::SetCodec(enAgentCodec codec)
{
	m_codecType = codec;

	m_payloadType = GetPayloadTypeByCodecType(codec);
}

void CRTPSendProcessor::SetTimeStampInc(unsigned int inc)
{
	m_timeStampInc = inc;
}

static const uint8_t *ff_avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
	const uint8_t *a = p + 4 - ((intptr_t)p & 3);

	for (end -= 3; p < a && p < end; p++) 
	{
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	for (end -= 3; p < end; p += 4) 
	{
		uint32_t x = *(const uint32_t*)p;
		//      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
		//      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
		if ((x - 0x01010101) & (~x) & 0x80808080)
		{ // generic
			if (p[1] == 0) 
			{
				if (p[0] == 0 && p[2] == 1)
					return p;
				if (p[2] == 0 && p[3] == 1)
					return p + 1;
			}
			if (p[3] == 0) 
			{
				if (p[2] == 0 && p[4] == 1)
					return p + 2;
				if (p[4] == 0 && p[5] == 1)
					return p + 3;
			}
		}
	}

	for (end += 3; p < end; p++)
	{
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	return end + 3;
}

static const uint8_t *ff_avc_find_startcode(const uint8_t *p, const uint8_t *end)
{
	const uint8_t *out = ff_avc_find_startcode_internal(p, end);
	if (p < out && out < end && !out[-1]) 
		out--;
	return out;
}

int CRTPSendProcessor::ReceiveData(const unsigned char* pData, int dataSize)
{
	if (pData == NULL || dataSize <= 0)
	{
		return - 1;
	}

	switch (m_payloadType)
	{
	case RTP_PAYLOAD_PCMU: //pcmu
	case RTP_PAYLOAD_PCMA: //pcma
	case RTP_PAYLOAD_OPUS: //opus
		AudioPack(pData, dataSize, m_curTimeStamp);
		break;
	case RTP_PAYLOAD_H264: //h264
		H264Pack(pData, dataSize, m_curTimeStamp);
		break;
	case RTP_PAYLOAD_H265: //h265
		H265Pack(pData, dataSize, m_curTimeStamp);
		break;
	}

	m_curTimeStamp += m_timeStampInc;

	return 0;

}

void CRTPSendProcessor::AudioPack(const unsigned char* pData, int dataSize, unsigned int ts)
{
	memset(m_rtpBuf, 0, sizeof(m_rtpBuf));
	RTPHeader* pHeader = (RTPHeader*)m_rtpBuf;
	pHeader->version = 2;
	pHeader->payloadtype = m_payloadType;
	pHeader->sequencenumber = htons(++m_pktSeq);
	pHeader->timestamp = htonl(ts);
	pHeader->ssrc = htonl(m_ssrc);

	memcpy(m_rtpBuf + sizeof(RTPHeader), pData, dataSize);
	m_pDataOut->Write(m_rtpBuf, sizeof(RTPHeader) + dataSize);
}

void CRTPSendProcessor::H264Pack(const unsigned char* pData, int dataSize, unsigned int ts)
{
	const unsigned char* end = pData + dataSize;
	const unsigned char* r = NULL;
	
	r = ff_avc_find_startcode(pData, end);

	while (r < end)
	{
		const unsigned char* pNextStartcode = NULL;
		while (!*(r++));
		pNextStartcode = ff_avc_find_startcode(r, end);
		{
			const unsigned char* pNalu = r;
			int nalSize = pNextStartcode - r;
			bool bMark = (pNextStartcode == end);

			if (nalSize < m_maxPayloadSize)
			{//单个NAL包
				memset(m_rtpBuf, 0, sizeof(m_rtpBuf));
				RTPHeader* pHeader = (RTPHeader*)m_rtpBuf;
				pHeader->version = 2;
				pHeader->marker = bMark;
				pHeader->payloadtype = m_payloadType;
				pHeader->sequencenumber = htons(++m_pktSeq);
				pHeader->timestamp = htonl(ts);
				pHeader->ssrc = htonl(m_ssrc);

				memcpy(m_rtpBuf + sizeof(RTPHeader), pNalu, nalSize);
				m_pDataOut->Write(m_rtpBuf, sizeof(RTPHeader) + nalSize);

			}
			else
			{//FU-A分片包

				unsigned char naluHead = pNalu[0];
				int forbidden_bit = naluHead & 0x80;
				int nal_reference_idc = naluHead & 0x60;
				int nal_unit_type = naluHead & 0x1f;

				unsigned char fu_indicator = 0x00;
				unsigned char fu_hdr = 0x00;

				fu_indicator = 28;
				fu_indicator |= nal_reference_idc;
				fu_indicator |= forbidden_bit;

				fu_hdr |= 1 << 7;

				pNalu++;
				nalSize--;

				int realPayloadSize = m_maxPayloadSize - 2;
				while (nalSize > 0)
				{
					memset(m_rtpBuf, 0, sizeof(m_rtpBuf));
					RTPHeader* pHeader = (RTPHeader*)m_rtpBuf;
					pHeader->version = 2;
					pHeader->payloadtype = m_payloadType;
					pHeader->sequencenumber = htons(++m_pktSeq);
					pHeader->timestamp = htonl(ts);
					pHeader->ssrc = htonl(m_ssrc);


					fu_hdr |= nal_unit_type;
					if(nalSize <= m_maxPayloadSize - 2)
					{
						fu_hdr |= 1 << 6;
						pHeader->marker = bMark;

						realPayloadSize = nalSize;
					}

					*(m_rtpBuf + sizeof(RTPHeader)) = fu_indicator;
					*(m_rtpBuf + sizeof(RTPHeader) + 1) = fu_hdr;
			
					memcpy(m_rtpBuf + sizeof(RTPHeader) + 2, pNalu, realPayloadSize);
					m_pDataOut->Write(m_rtpBuf, sizeof(RTPHeader) + 2 + realPayloadSize);
					pNalu += realPayloadSize;
					nalSize -= realPayloadSize;

					fu_hdr = 0x00;
				}
			}
		}
		r = pNextStartcode;
	}
}

void CRTPSendProcessor::H265Pack(const unsigned char* pData, int dataSize, unsigned int ts)
{
	const unsigned char* end = pData + dataSize;
	const unsigned char* r = NULL;

	r = ff_avc_find_startcode(pData, end);

	while (r < end)
	{
		const unsigned char* pNextStartcode = NULL;
		while (!*(r++));
		pNextStartcode = ff_avc_find_startcode(r, end);
		{
			const unsigned char* pNalu = r;
			int nalSize = pNextStartcode - r;
			bool bMark = (pNextStartcode == end);

			if (nalSize < m_maxPayloadSize)
			{//单个NAL包
				memset(m_rtpBuf, 0, sizeof(m_rtpBuf));
				RTPHeader* pHeader = (RTPHeader*)m_rtpBuf;
				pHeader->version = 2;
				pHeader->marker = bMark;
				pHeader->payloadtype = m_payloadType;
				pHeader->sequencenumber = htons(++m_pktSeq);
				pHeader->timestamp = htonl(ts);
				pHeader->ssrc = htonl(m_ssrc);

				memcpy(m_rtpBuf + sizeof(RTPHeader), pNalu, nalSize);
				m_pDataOut->Write(m_rtpBuf, sizeof(RTPHeader) + nalSize);

			}
			else
			{//FU分片包
				int nal_unit_type = (pNalu[0] >> 1) & 0x3F;
				unsigned char nul_hdr[3] = { (pNalu[0] & 0x81) | (49 << 1), pNalu[1], 0x00 };

				nul_hdr[2] |= 1 << 7;

				pNalu += 2;
				nalSize -= 2;

				int realPayloadSize = m_maxPayloadSize - 3;
				while (nalSize > 0)
				{
					memset(m_rtpBuf, 0, sizeof(m_rtpBuf));
					RTPHeader* pHeader = (RTPHeader*)m_rtpBuf;
					pHeader->version = 2;
					pHeader->payloadtype = m_payloadType;
					pHeader->sequencenumber = htons(++m_pktSeq);
					pHeader->timestamp = htonl(ts);
					pHeader->ssrc = htonl(m_ssrc);

					nul_hdr[2] |= nal_unit_type;
					if (nalSize <= m_maxPayloadSize - 3)
					{
						nul_hdr[2] |= 1 << 6;
						pHeader->marker = bMark;

						realPayloadSize = nalSize;
					}

					memcpy(m_rtpBuf + sizeof(RTPHeader), nul_hdr, sizeof(nul_hdr));
					memcpy(m_rtpBuf + sizeof(RTPHeader) + sizeof(nul_hdr), pNalu, realPayloadSize);
					m_pDataOut->Write(m_rtpBuf, sizeof(RTPHeader) + sizeof(nul_hdr) + realPayloadSize);
					pNalu += realPayloadSize;
					nalSize -= realPayloadSize;

					nul_hdr[2] = 0x00;
				}
			}
		}
		r = pNextStartcode;
	}
}
