#ifndef __TIME_H__
#define __TIME_H__

#define TIME_PORT "8111" // 授时端口
#define PACKET_HEAD "shiruixun-fenbushi-shoushi" // 数据包头部

extern char ip[128][16]; // ip 数组
extern int net_id; // 组网 ID

typedef struct {
    char head[64]; // 数据包头部
    unsigned char net_id; // 组网ID
    unsigned char priority; // 优先级(0-0xff 0最高)
    unsigned char net_flag; // 组网标志(指定服务器，自主) 0指定服务器 1自主
    unsigned long long time; // 本机系统时间(us)
} time_packet_t;

/**
 * @brief 广播发送授时包函数(16ms发送一次授时包)
 * 
 * @return int 成功返回 0，失败返回 -1
 */
int send_time_broadcast();

#endif // __TIME_H__