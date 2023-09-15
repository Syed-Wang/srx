#ifndef __SRX_H__
#define __SRX_H__

#define TIME_PORT "8111" // 授时端口
#define CMD_PORT "8112" // 指令端口
#define PORT "8113" // H264端口
#define BROADCAST_IP "255.255.255.255" // 广播地址
#define GROUP_IP "224.100.200.1" // 组播地址
#define UNICAST_IP "192.168.0.100" // 单播地址
#define CMD_DETECT_TIME_GAP "/detectTimeGap:d,1;" // 检测时间差
#define CMD_ACK_TIME_GAP "/ackTimeGap:d,1;" // 应答时间差
#define CMD_SET_TIME_GAP(gap) "/setTimeGap:d," #gap ";" // 设置时间差
#define RET_LEN 1400 // 接收缓冲区大小

// 授时包格式
typedef struct {
    char head[64]; // 数据包头部
    unsigned char net_id; // 组网ID
    unsigned char priority; // 优先级(0-0xff 0最高)
    unsigned char net_flag; // 组网标志(指定服务器，自主)
    unsigned long long time; // 本机系统时间(us)
} time_packet_t;


#endif // __SRX_H__