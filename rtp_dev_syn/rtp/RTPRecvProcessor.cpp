#include "RTPRecvProcessor.h"
//#include "LoggerHelp.h"
#include <QDebug>
#ifdef _WIN32
#include <winsock.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#endif // _WIN32
#include <iostream>
#include<string.h>
#define LOG_DEBUG qDebug()
CRTPRecvProcessor::CRTPRecvProcessor() : m_ssrc(0), m_payloadType(-1), m_codecType(enAgentCode_unKnow)
{
}


CRTPRecvProcessor::~CRTPRecvProcessor()
{
}

static void DumpHex(const unsigned char* pData, int dataSize, int showNum)
{
	char szBuf[1024] = { 0 };
	int len = showNum < dataSize ? showNum : dataSize;

	for (int i = 0; i < len; i++)
	{
		char sz[16] = { 0 };
		snprintf(sz, 16, "0x%02x ", pData[i]);
		strcat(szBuf, sz);
	}

    //LOG_DEBUG << szBuf;
}

int CRTPRecvProcessor::ReceiveData(const unsigned char* pData, int dataSize)
{
	if (pData == NULL || dataSize < sizeof(RTPHeader))
		return -1;
    static unsigned short  lastsn=0;
	RTPHeader *rtpheader = (RTPHeader*)pData;

	rtpheader->sequencenumber = ntohs(rtpheader->sequencenumber);
	rtpheader->timestamp = ntohl(rtpheader->timestamp);
	rtpheader->ssrc = ntohl(rtpheader->ssrc);
   // LOG_DEBUG << "rtpheader->sequencenumber "<<rtpheader->sequencenumber;

    if((lastsn+1)!=rtpheader->sequencenumber)
    {
        LOG_DEBUG << "ReceiveData lost, last sn "<<lastsn <<"cur sn "<<rtpheader->sequencenumber  ;
    }
    lastsn=rtpheader->sequencenumber;

	if(m_payloadType == -1)
		m_payloadType = rtpheader->payloadtype;

	m_codecType = GetCodecTypeByPayload(m_payloadType);

	if(m_ssrc == 0)
		m_ssrc = rtpheader->ssrc;

	switch (m_payloadType)
	{
	case RTP_PAYLOAD_PCMU: //pcmu
	case RTP_PAYLOAD_PCMA: //pcma
		//LOG_DEBUG << "PCMA rtp packet size=" << dataSize;
		//DumpHex(pData, dataSize, 12);
		//LOG_DEBUG << "PCMA rtp packet, seq=" << rtpheader->sequencenumber << ",ts=" << rtpheader->timestamp << ",ssrc=" << rtpheader->ssrc << ",mark=" << rtpheader->marker;
	case RTP_PAYLOAD_OPUS: //opus
		ProcessAudio(pData + sizeof(RTPHeader), dataSize - sizeof(RTPHeader), rtpheader->timestamp);
		break;
	case RTP_PAYLOAD_H264: //h264
		//LOG_DEBUG << "H264 rtp packet size=" << dataSize;
		//DumpHex(pData, dataSize, 16);
		//LOG_DEBUG << "H264 rtp packet, seq=" << rtpheader->sequencenumber << ",ts=" << rtpheader->timestamp << ",ssrc=" << rtpheader->ssrc << ",mark=" << rtpheader->marker;
		ProcessH264Video(pData + sizeof(RTPHeader), dataSize - sizeof(RTPHeader), rtpheader->timestamp, rtpheader->marker);
		break;
	case RTP_PAYLOAD_H265: //h265
		//LOG_DEBUG << "H265 rtp packet size=" << dataSize;
		//DumpHex(pData, dataSize, 16);
		//LOG_DEBUG << "H265 rtp packet, seq=" << rtpheader->sequencenumber << ",ts=" << rtpheader->timestamp << ",ssrc=" << rtpheader->ssrc << ",mark=" << rtpheader->marker;
		ProcessH265Video(pData + sizeof(RTPHeader), dataSize - sizeof(RTPHeader), rtpheader->timestamp, rtpheader->marker);
		break;
	}

	return 0;
}

unsigned int CRTPRecvProcessor::GetSSRC()
{
	return m_ssrc;
}

int CRTPRecvProcessor::GetPayloadType()
{
	return m_payloadType;
}

enAgentCodec CRTPRecvProcessor::GetCodecType()
{
	return m_codecType;
}

int CRTPRecvProcessor::GetExtraData(PacketDataPtr& pkt)
{
	if (m_extraData && m_extraData->bMark)
	{
		pkt = m_extraData;
		return 0;
	}

	return -1;
}

int CRTPRecvProcessor::GetPacketData(PacketDataPtr& pkt)
{
	std::lock_guard<std::mutex> lock(m_pktMutex);
	if (m_pktQue.empty())
	{
		return -1;
	}
	pkt = m_pktQue.front();
	if (!pkt->bMark)
	{
		return -1;
	}
	/*
	if (m_codecType == enAgentCodec_H264 || m_codecType == enAgentCodec_H265)
	{
		LOG_DEBUG << "Get a video packet, size=" << pkt->dataSize << ", timestamp=" << pkt->timeStamp 
			<< ", NALU type=" << pkt->naluType << ", has key frame " << pkt->bHaveKeyFrame;
		DumpHex(pkt->pData, pkt->dataSize, 16);
	}
	*/


	m_pktQue.pop_front();

	return 0;
}

int CRTPRecvProcessor::ProcessAudio(const unsigned char* pData, int dataSize, unsigned int ts)
{
	if (pData == NULL || dataSize <= 0)
	{
		return -1;
	}

	std::lock_guard<std::mutex> lock(m_pktMutex);
	PacketDataPtr pkt(new PacketData(pData, dataSize, ts));
	pkt->codecType = m_codecType;
	pkt->bMark = true;
	m_pktQue.push_back(pkt);
	return 0;
}

int CRTPRecvProcessor::ProcessH264Video(const unsigned char* pData, int dataSize, unsigned int ts, bool bMark)
{
	std::lock_guard<std::mutex> lock(m_pktMutex);

	static unsigned char nal_start_code0[4] = { 0x00, 0x00, 0x00, 0x01 };
	//static unsigned char nal_start_code[3] = { 0x00, 0x00, 0x01 };

	int type = pData[0] & 0x1F;

	if (type >= 1 && type <= 24)
	{

		if (!m_extraData && type == 7)
		{
			m_extraData.reset(new PacketData(nal_start_code0, sizeof(nal_start_code0), 0));
			m_extraData->Push(pData, dataSize);
			m_extraData->naluType = type;
			m_extraData->codecType = m_codecType;

			LOG_DEBUG << "SPS dataSize: " << m_extraData->dataSize;
			DumpHex(m_extraData->pData, m_extraData->dataSize, 8);
		}
		else if (m_extraData && !m_extraData->bMark && type == 8)
		{
			m_extraData->Push(nal_start_code0, sizeof(nal_start_code0));
			m_extraData->Push(pData, dataSize);
			m_extraData->bMark = true;

			LOG_DEBUG << "PPS dataSize: " << m_extraData->dataSize;
			DumpHex(m_extraData->pData, m_extraData->dataSize, m_extraData->dataSize);
		}

		if (!m_pktQue.empty())
		{
			PacketDataPtr lastPkt = m_pktQue.back();
			if (!lastPkt->bMark)
			{
				lastPkt->Push(nal_start_code0, sizeof(nal_start_code0));
				lastPkt->Push(pData, dataSize);
				lastPkt->bMark = bMark;
				if (type == 5)
				{
					lastPkt->bHaveKeyFrame = true;
				}
				return 0;
			}

		}

		PacketDataPtr pkt(new PacketData(nal_start_code0, sizeof(nal_start_code0), ts));
		pkt->Push(pData, dataSize);
		pkt->codecType = m_codecType;
		pkt->naluType = type;
		pkt->bMark = bMark;
		if (type == 5)
		{
			pkt->bHaveKeyFrame = true;
		}
		m_pktQue.push_back(pkt);
	}
	else if (type == 28)	//FU_A
	{
		unsigned char fu_indicator = pData[0];
		unsigned char fu_header = pData[1];
		int sBit = fu_header >> 7;
		int eBit = (fu_header & 0x40) >> 6;
		unsigned char nal_header = (fu_indicator & 0xE0) | (fu_header & 0x1F);


		if (sBit == 1)
		{
			//LOG_DEBUG << "FU-A Start";
			PacketDataPtr pkt;
			if (!m_pktQue.empty())
			{
				pkt = m_pktQue.back();
				if (pkt->bMark)
				{
					PacketDataPtr newpkt(new PacketData(nal_start_code0, sizeof(nal_start_code0), ts));
					newpkt->codecType = m_codecType;
					newpkt->naluType = nal_header & 0x1F;
					newpkt->Push(&nal_header, 1);
					newpkt->Push(pData + 2, dataSize - 2);
					newpkt->bMark = bMark;
					if ((nal_header & 0x1F) == 5)
					{
						newpkt->bHaveKeyFrame = true;
					}

					m_pktQue.push_back(newpkt);
				}
				else
				{
					pkt->Push(nal_start_code0, sizeof(nal_start_code0));
					pkt->Push(&nal_header, 1);
					pkt->Push(pData + 2, dataSize - 2);
					pkt->bMark = bMark;
					if ((nal_header & 0x1F) == 5)
					{
						pkt->bHaveKeyFrame = true;
					}
				}
			}
			else
			{
				PacketDataPtr newpkt(new PacketData(nal_start_code0, sizeof(nal_start_code0), ts));
				newpkt->codecType = m_codecType;
				newpkt->naluType = nal_header & 0x1F;
				newpkt->Push(&nal_header, 1);
				newpkt->Push(pData + 2, dataSize - 2);
				newpkt->bMark = bMark;
				if ((nal_header & 0x1F) == 5)
				{
					newpkt->bHaveKeyFrame = true;
				}
				m_pktQue.push_back(newpkt);
			}
		}
		else
		{
			if (!m_pktQue.empty())
			{
				PacketDataPtr pkt = m_pktQue.back();
				pkt->Push(pData + 2, dataSize - 2);
				pkt->bMark = bMark;
				if (eBit == 1)
				{
					//LOG_DEBUG << "FU-A End";
				}
			}
		}
	}

	return 0;
}

int CRTPRecvProcessor::ProcessH265Video(const unsigned char* pData, int dataSize, unsigned int ts, bool bMark)
{
	std::lock_guard<std::mutex> lock(m_pktMutex);

	static unsigned char nal_start_code[4] = { 0x00, 0x00, 0x00, 0x01 };

	int type = (pData[0] & 0x7E) >> 1;

	if (49 == type)
	{//FU
		int startBit = (pData[2] & 0x80) >> 7;
		int endBit = (pData[2] & 0x40) >> 6;
		int NaluType = pData[2] & 0x3F;

		if (startBit == 1)
		{
			//LOG_DEBUG << "FU Start";
			unsigned char new_nal_hdr[2] = { 0x00 };
			//×énalu header
			new_nal_hdr[0] = (pData[0] & 0x81) | (NaluType << 1);
			new_nal_hdr[1] = pData[1];

			PacketDataPtr pkt;
			if (!m_pktQue.empty())
			{
				pkt = m_pktQue.back();
				if (pkt->bMark)
				{
					PacketDataPtr newpkt(new PacketData(nal_start_code, sizeof(nal_start_code), ts));
					newpkt->codecType = m_codecType;
					newpkt->naluType = NaluType;
					newpkt->Push(new_nal_hdr, 2);
					newpkt->Push(pData + 3, dataSize - 3);
					newpkt->bMark = bMark;
					if (NaluType >= 16 && NaluType <= 21)
					{
						newpkt->bHaveKeyFrame = true;
					}
					m_pktQue.push_back(newpkt);
				}
				else
				{
					pkt->Push(nal_start_code, sizeof(nal_start_code));
					pkt->Push(new_nal_hdr, 2);
					pkt->Push(pData + 3, dataSize - 3);
					pkt->bMark = bMark;
					if (NaluType >= 16 && NaluType <= 21)
					{
						pkt->bHaveKeyFrame = true;
					}
				}
			}
			else
			{
				PacketDataPtr newpkt(new PacketData(nal_start_code, sizeof(nal_start_code), ts));
				newpkt->codecType = m_codecType;
				newpkt->naluType = NaluType;
				newpkt->Push(new_nal_hdr, 2);
				newpkt->Push(pData + 3, dataSize - 3);
				newpkt->bMark = bMark;
				if (NaluType >= 16 && NaluType <= 21)
				{
					newpkt->bHaveKeyFrame = true;
				}
				m_pktQue.push_back(newpkt);
			}
		}
		else
		{
			if (!m_pktQue.empty())
			{
				PacketDataPtr pkt = m_pktQue.back();
				pkt->Push(pData + 3, dataSize - 3);
				pkt->bMark = bMark;
				if (endBit == 1)
				{
					//LOG_DEBUG << "FU End";
				}
			}
		}
	}
	else if (48 == type)
	{

	}
	else
	{
		if (!m_extraData && type == 32)
		{
			m_extraData.reset(new PacketData(nal_start_code, sizeof(nal_start_code), 0));
			m_extraData->Push(pData, dataSize);
			m_extraData->naluType = type;
			m_extraData->codecType = m_codecType;

			LOG_DEBUG << "VPS dataSize: " << m_extraData->dataSize;
			DumpHex(m_extraData->pData, m_extraData->dataSize, 8);
		}
		else if (m_extraData && !m_extraData->bMark && type == 33)
		{
			m_extraData->Push(nal_start_code, sizeof(nal_start_code));
			m_extraData->Push(pData, dataSize);

			LOG_DEBUG << "SPS dataSize: " << m_extraData->dataSize;
			DumpHex(m_extraData->pData, m_extraData->dataSize, m_extraData->dataSize);
		}
		else if (m_extraData && !m_extraData->bMark && type == 34)
		{
			m_extraData->Push(nal_start_code, sizeof(nal_start_code));
			m_extraData->Push(pData, dataSize);
			m_extraData->bMark = true;

			LOG_DEBUG << "PPS dataSize: " << m_extraData->dataSize;
			DumpHex(m_extraData->pData, m_extraData->dataSize, m_extraData->dataSize);
		}

		if (!m_pktQue.empty())
		{
			PacketDataPtr lastPkt = m_pktQue.back();
			if (!lastPkt->bMark)
			{
				lastPkt->Push(nal_start_code, sizeof(nal_start_code));
				lastPkt->Push(pData, dataSize);
				lastPkt->bMark = bMark;
				if (type >= 16 && type <= 21)
				{
					lastPkt->bHaveKeyFrame = true;
				}
				return 0;
			}

		}

		PacketDataPtr pkt(new PacketData(nal_start_code, sizeof(nal_start_code), ts));
		pkt->Push(pData, dataSize);
		pkt->codecType = m_codecType;
		pkt->naluType = type;
		pkt->bMark = bMark;
		if (type >= 16 && type <= 21)
		{
			pkt->bHaveKeyFrame = true;
		}
		m_pktQue.push_back(pkt);
	}

	return 0;
}
