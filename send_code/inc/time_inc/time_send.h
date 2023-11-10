#ifndef __TIME_SEND_H__
#define __TIME_SEND_H__

#define TIME_PORT "8111" // 授时端口
#define BROADCAST_IP "255.255.255.255" // 广播地址
#define PACKET_HEAD "shiruixun-fenbushi-shoushi" // 数据包头部

#ifdef DEBUG
#define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(format, ...)
#endif

extern const char* ip[2]; // ip 数组

typedef struct {
    char head[64]; // 数据包头部
    unsigned char net_id; // 组网ID
    unsigned char priority; // 优先级(0-0xff 0最高)
    unsigned char net_flag; // 组网标志(指定服务器，自主)
    unsigned long long time; // 本机系统时间(us)
} time_packet_t;

int send_time();
int recv_time();

#endif // __TIME_SEND_H__