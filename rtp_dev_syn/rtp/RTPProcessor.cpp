#include "RTPProcessor.h"
#include <string.h>
#define PKT_BUF_LEN 33554432L// 4096*4096*2 //BY DSS
struct RTPCodecParam
{
	std::string strCodecName;
	enAgentCodec codecType;
	int payloadType;
};

static RTPCodecParam g_rtpCodecs[5] = {
	{ "pcmu", enAgentCodec_G711U, RTP_PAYLOAD_PCMU },
	{ "pcma", enAgentCodec_G711A, RTP_PAYLOAD_PCMA },
	{ "opus", enAgentCodec_OPUS, RTP_PAYLOAD_OPUS },
	{ "h264", enAgentCodec_H264, RTP_PAYLOAD_H264 },
	{ "h265", enAgentCodec_H265, RTP_PAYLOAD_H265 }
};

int GetPayloadTypeByCodecType(enAgentCodec codecType)
{
	for (int i = 0; i < 5; i++)
	{
		if (g_rtpCodecs[i].codecType == codecType)
		{
			return g_rtpCodecs[i].payloadType;
		}
	}

	return -1;
}

int GetPayloadTypeByCodecName(const std::string& codecName)
{
	for (int i = 0; i < 5; i++)
	{
		if (g_rtpCodecs[i].strCodecName == codecName)
		{
			return g_rtpCodecs[i].payloadType;
		}
	}

	return -1;
}

enAgentCodec GetCodecTypeByPayload(int payloadtype)
{
	for (int i = 0; i < 5; i++)
	{
		if (g_rtpCodecs[i].payloadType == payloadtype)
		{
			return g_rtpCodecs[i].codecType;
		}
	}

	return enAgentCode_unKnow;
}








PacketData::PacketData() : codecType(enAgentCode_unKnow), pData(NULL), dataSize(0), timeStamp(0), bMark(false), bHaveKeyFrame(false)
{

}

//PacketData::PacketData(const unsigned char* pBuf, int len, unsigned int ts)
//{
//	if (pBuf != NULL && len > 0)
//	{
//		dataSize = len;
//		pData = new unsigned char[len];
//		memcpy(pData, pBuf, len);
//	}

//	timeStamp = ts;

//	bMark = false;
//	bHaveKeyFrame = false;
//}
PacketData::PacketData(const unsigned char* pBuf, int len, unsigned int ts)
{
    if (pBuf != NULL && len > 0)
    {
        dataSize = len;
        pData = new unsigned char[PKT_BUF_LEN];
        memcpy(pData, pBuf, len);
    }

    timeStamp = ts;

    bMark = false;
    bHaveKeyFrame = false;
}
PacketData::~PacketData()
{
	if (pData)
	{
		delete pData;
		pData = NULL;
	}
}

//void PacketData::Push(const unsigned char* pBuf, int len)
//{
//	if (pBuf == NULL || len <= 0)
//	{
//		return;
//	}

//	unsigned char* pTemp = new unsigned char[dataSize + len];
//	if (pData != NULL && dataSize > 0)
//	{
//		memcpy(pTemp, pData, dataSize);
//		delete pData;
//	}
//	memcpy(pTemp + dataSize, pBuf, len);

//	dataSize += len;
//	pData = pTemp;
//}

void PacketData::Push(const unsigned char* pBuf, int len)
{
    if (pBuf == NULL || len <= 0)
    {
        return;
    }

    memcpy(pData + dataSize, pBuf, len);
    dataSize += len;

}
