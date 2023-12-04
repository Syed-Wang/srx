#ifndef RTP_PROCESSOR_H
#define RTP_PROCESSOR_H

#include <string>
#include <memory>
//#include "AgentEvent.h"

enum enAgentCodec
{
	enAgentCode_unKnow = -1,
	enAgentCodec_G711U,
	enAgentCodec_G711A,
	enAgentCodec_G722,
	enAgentCodec_RFC2833,
	enAgentCodec_AAC,
	enAgentCodec_H263,
	enAgentCodec_H264,
	enAgentCodec_OPUS,
	enAgentCodec_OPUS_16K,
	enAgentCodec_H265,
	enAgentCodec_MPEG4,
	enAgentCodec_G7231,
	enAgentCodec_G729,
	enAgentCodec_AMRNB,
	enAgentCodec_AMRWB,
	enAgentCodec_GSM,
	enAgentCodec_ILBC,
	enAgentCodec_SILK,
	enAgentCodec_FEC_AUDIO,
	enAgentCodec_FEC_VIDEO
};


#define RTP_PAYLOAD_PCMU	0
#define RTP_PAYLOAD_PCMA	8
#define RTP_PAYLOAD_OPUS	103
#define RTP_PAYLOAD_H264	98 //96
#define RTP_PAYLOAD_H265	100


int GetPayloadTypeByCodecType(enAgentCodec codecType);

int GetPayloadTypeByCodecName(const std::string& codecName);

enAgentCodec GetCodecTypeByPayload(int payloadtype);


struct RTPHeader
{
#ifdef RTP_BIG_ENDIAN
	unsigned char version : 2;
	unsigned char padding : 1;
	unsigned char extension : 1;
	unsigned char csrccount : 4;

	unsigned char marker : 1;
	unsigned char payloadtype : 7;
#else // little endian
	unsigned char csrccount : 4;
	unsigned char extension : 1;
	unsigned char padding : 1;
	unsigned char version : 2;

	unsigned char payloadtype : 7;
	unsigned char marker : 1;
#endif // BIG_ENDIAN

	unsigned short sequencenumber;
	unsigned int timestamp;
	unsigned int ssrc;
};


struct PacketData
{
	enAgentCodec codecType;
	unsigned char* pData;
	int dataSize;
	unsigned int timeStamp;
	int naluType;
	bool bMark;
	bool bHaveKeyFrame;

	PacketData();

	PacketData(const unsigned char* pBuf, int len, unsigned int ts);

	~PacketData();

	void Push(const unsigned char* pBuf, int len);
};

typedef std::shared_ptr<PacketData> PacketDataPtr;

#endif //RTP_PROCESSOR_H