#include "RTPSendProcessor.h"
#include "sendpacket.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

SendPacket::SendPacket()
{
}

SendPacket::~SendPacket()
{
    close(fd);
}
int SendPacket::maintest(int argc, char* argv[]) //./testrtp 192.168.2.77 3950 1
{
    char buf[640] = { 1 };
    int localport = 13008;
    memset(buf, 1, sizeof(buf));
    desip = argv[1];
    destport = atoi(argv[2]);
    sec = atoi(argv[3]);
    count = sec * 1000 / 20;
    fd = -1;

    //  localport=destport;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    fcntl(fd, F_SETFL, O_NDELAY);

    int reuse_addr_flag = 1;
    int success = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_flag, sizeof(reuse_addr_flag));
    if (success < 0) {
        perror("udp setsockopt(SO_REUSEADDR): ");
        close(fd);
        return 0;
    }
    int sendBufSize, sendBufLength;
    sendBufSize = 4 * 1024 * 1024;
    sendBufLength = sizeof sendBufSize;

    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufSize, sendBufLength) < 0) {
        perror("udp setsockopt(SO_SNDBUF): ");
        return 0;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&sendBufSize, sendBufLength) < 0) {
        perror("udp setsockopt(SO_RCVBUF): ");
        return 0;
    }

    //    // 设置多播TTL参数
    unsigned char ttl = 1;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        printf("Error setting multicast TTL");
        return -1;
    }

    struct sockaddr_in localaddr;
    in_addr_t net_addr = inet_addr("0.0.0.0");
    // ntohl(l_ipv4.ip_dw)
    memset(&localaddr, 0, sizeof(struct sockaddr_in));
    localaddr.sin_family = AF_INET;
    localaddr.sin_port = localport; // 13008;
    memcpy((struct sockaddr*)&localaddr.sin_addr, (void*)(&net_addr), sizeof(net_addr));
    if (bind(fd, (struct sockaddr*)&localaddr, sizeof(struct sockaddr_in)) != 0) {
        return 0;
    }
    joinGroup();
    int i = 0;
    //   for(i=0;i<count;i++)
    //   {
    //     sendUDPMsg(fd,desip,destport,buf,sizeof(buf));
    //     usleep(1000 * 20);
    //   }
    return 1;
}

void SendPacket::sendUDPMsg(int fd, char* host, unsigned short port, char* buffer, int length)
{
    int success;
    struct sockaddr* pDestAddr = NULL;
    struct sockaddr_in destAddr;
    int len;
    memset(&destAddr, 0, sizeof(struct sockaddr_in));
    in_addr_t net_addr = inet_addr(host);
    // in_addr_t net_addr=inet_addr(multicastip.c_str());
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);
    memcpy((struct sockaddr*)&destAddr.sin_addr, (void*)(&net_addr), sizeof(net_addr));
    pDestAddr = (struct sockaddr*)&destAddr;
    len = sizeof(destAddr);
    success = sendto(fd, buffer, length, 0, pDestAddr, len);
    if (length != success) {
        printf("length %d success send  %d\r\n", length, success);
    }
    if (success < 0) {
        char* buf = new char[length + 128];
        sprintf(buf, "sendto(%d, %s, %d, %s, %d):", fd, buffer, length, host, port);
        perror(buf);
        delete[] buf;
    }
}

void SendPacket::joinGroup()
{
#if 1 // MULTICAST_FLG

    //    getsockopt()/setsockopt() 的选项 含 义
    //        IP_MULTICAST_TTL 设置多播组数据的 TTL 值
    //        IP_ADD_MEMBERSHIP  在指定接口上加入组播组
    //        IP_DROP_MEMBERSHIP 退出组播组
    //        IP_MULTICAST_IF 获取默认接口或设置接口
    //        IP_MULTICAST_LOOP 禁止组播数据回送
    //        // 加入到多播组
    //        struct ip_mreq req;
    //        char *intfc_ip="0.0.0.0";
    //        memset(&req, 0, sizeof(req));
    //        inet_aton(intfc_ip, &req.imr_interface);
    //        inet_aton("230.0.0.1", &req.imr_multiaddr);
    //        if( setsockopt(client_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &req, sizeof(req)) < 0 ) {
    //        perror("setsockopt IP_ADD_MEMBERSHIP");
    //        return -1;
    //        }

    //        // 加入到多播组
    //        struct ip_mreqn opt;
    //        // 要加入到哪个多播组, 通过组播地址来区分
    //        inet_pton(AF_INET, "238.0.1.10", &opt.imr_multiaddr.s_addr);
    //        opt.imr_address.s_addr = INADDR_ANY;
    //        opt.imr_ifindex = if_nametoindex("eth0");//if_nametoindex("ens11");
    //        setsockopt(client_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &opt, sizeof(opt));

    // 加入到多播组
    struct ip_mreq join_adr; // 声明加入多播组的结构体变量
    // 初始化ip_mreq结构体变量join_adr
    join_adr.imr_multiaddr.s_addr = inet_addr(multicastip.c_str()); // inet_addr("230.0.0.1"); //初始化多播组IP地址
    join_adr.imr_interface.s_addr = htonl(INADDR_ANY); // 初始化待加入多播组主机的IP地址
    int ret = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &join_adr, sizeof(join_adr));
    if (ret < 0) {
        printf("joinGroup error %d\r\n", ret);
    }
#endif
}
int SendPacket::dropGroup()
{
    // 多播组
    struct ip_mreq join_adr; // 声明加入多播组的结构体变量
    // 初始化ip_mreq结构体变量join_adr
    join_adr.imr_multiaddr.s_addr = inet_addr(multicastip.c_str()); // inet_addr("230.0.0.1"); //初始化多播组IP地址
    join_adr.imr_interface.s_addr = htonl(INADDR_ANY); // 初始化待加入多播组主机的IP地址
    // 离开多播组
    int ret = setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &join_adr, sizeof(join_adr));
    if (ret < 0) {
        printf("dropGroup error %d\r\n", ret);
        return -1;
    }
    return 0;
}

extern "C" {
CRTPSendProcessor rtpproc;
IDataIOCallback udpsend;

void send_init()
{
    /* char* argv[] = {
        "./testrtp",
        "224.100.200.1",
        // "192.168.1.111",
        "8113",
        "1",
    }; */
    char arg1[] = "./testrtp";
    char arg2[] = "224.100.200.1"; // "192.168.1.111"
    char arg3[] = "8113";
    char arg4[] = "1";

    char* argv[] = {
        arg1,
        arg2,
        arg3,
        arg4,
    };

    udpsend.maintest(4, argv);
    rtpproc.SetCodec(enAgentCodec_H264);
    rtpproc.SetDataOut(&udpsend);
    rtpproc.SetPayloadType(RTP_PAYLOAD_H264);
}

int send_data_to_udp(char* ptr, size_t len)
{
    rtpproc.ReceiveData((const unsigned char*)ptr, len);

    return 0;
}
}