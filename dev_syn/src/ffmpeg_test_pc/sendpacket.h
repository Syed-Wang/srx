#ifndef SENDPACKET_H
#define SENDPACKET_H
#include<stdio.h>
#include<string>
class SendPacket
{
public:
    SendPacket();
    ~SendPacket();
    void sendUDPMsg(int fd,char* host,unsigned short port,char* buffer,int length);
    int maintest(int argc,char* argv[]);


    char buf[640]={1};
    char * desip = NULL;
    int destport = 8888;
    int sec = 0;
    int count = 0;
    int fd=-1;

    void   joinGroup();
    int   dropGroup();
    std::string multicastip="224.100.200.1";
};
class IDataIOCallback :public SendPacket
{
public:
    int Write(unsigned char * buf,int len){sendUDPMsg(  fd,desip,destport,(char *)buf,len); return 0;}
};//add by dss
#endif // SENDPACKET_H
