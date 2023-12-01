#ifndef RTP_SENDPROCESSOR_H
#define RTP_SENDPROCESSOR_H

//#include "recordstreamer/CommonDef.h"
#include "RTPProcessor.h"
#include <thread>
#include "sendpacket.h"
class IDataIOCallback;
//{
//public:
//    int Write(unsigned char * buf,int len){return 0;}
//};//add by dss
class CRTPSendProcessor
{
public:
	CRTPSendProcessor();
	~CRTPSendProcessor();

    void SetDataOut(IDataIOCallback *pDataOut);

	void SetPayloadType(int type);

	void SetCodec(enAgentCodec codec);

	void SetTimeStampInc(unsigned int inc);

	int ReceiveData(const unsigned char* pData, int dataSize);

private:
	void AudioPack(const unsigned char* pData, int dataSize, unsigned int ts);
	void H264Pack(const unsigned char* pData, int dataSize, unsigned int ts);
	void H265Pack(const unsigned char* pData, int dataSize, unsigned int ts);

private:
    IDataIOCallback *m_pDataOut;
	int m_maxPayloadSize;
	int m_payloadType;
	enAgentCodec m_codecType;

	unsigned char m_rtpBuf[1500];

	unsigned short m_pktSeq;

	unsigned int m_ssrc;

	unsigned int m_timeStampInc;

	unsigned int m_curTimeStamp;
};


#endif //RTP_SENDPROCESSOR_H
