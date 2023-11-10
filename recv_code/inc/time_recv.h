#ifndef __TIME_RECV_H__
#define __TIME_RECV_H__

#define TIME_PORT "8111" // 授时端口
#define PACKET_HEAD "shiruixun-fenbushi-shoushi" // 数据包头部

extern char ip[128][16]; // ip 数组
extern unsigned char net_id; // 组网 id

// 授时包结构体
typedef struct {
    char head[64]; // 数据包头部
    unsigned char net_id; // 组网ID
    unsigned char priority; // 优先级(0-0xff 0最高)
    unsigned char net_flag; // 组网标志(指定服务器，自主) 0指定服务器 1自主
    unsigned long long time; // 本机系统时间(us)
} time_packet_t;

int recv_time();
int send_time();

#endif // __TIME_RECV_H__