#ifndef RTP_RECVPROCESSOR_H
#define RTP_RECVPROCESSOR_H

#include <deque>
#include <map>
#include <mutex>
#include "RTPProcessor.h"
#include<iostream>
#include<mutex>
class CRTPRecvProcessor
{
public:
	CRTPRecvProcessor();
	~CRTPRecvProcessor();

	int ReceiveData(const unsigned char* pData, int dataSize);

	unsigned int GetSSRC();

	int GetPayloadType();

	enAgentCodec GetCodecType();

	int GetExtraData(PacketDataPtr& pkt);

	int GetPacketData(PacketDataPtr& pkt);

private:
	int ProcessAudio(const unsigned char* pData, int dataSize, unsigned int ts);
	int ProcessH264Video(const unsigned char* pData, int dataSize, unsigned int ts, bool bMark);
	int ProcessH265Video(const unsigned char* pData, int dataSize, unsigned int ts, bool bMark);

private:
	unsigned int m_ssrc;
	int m_payloadType;
	enAgentCodec m_codecType;

	std::deque<PacketDataPtr> m_pktQue;
	std::mutex m_pktMutex;

	PacketDataPtr m_extraData;
};

typedef std::shared_ptr<CRTPRecvProcessor> CRTPRecvProcessorPtr;

#endif //RTP_RECVPROCESSOR_H
