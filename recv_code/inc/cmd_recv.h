#ifndef __CMD_RECV_H__
#define __CMD_RECV_H__

#define CMD_PORT "8112" // 指令端口
#define CMD_DETECT_TIME_GAP "/detectTimeGap:d,1;" // 检测时间差
#define CMD_ACK_TIME_GAP "/ackTimeGap:d,1;" // 应答时间差
#define CMD_SET_TIME_GAP(gap) "/setTimeGap:d, #gap ;" // 设置时间差

// 记录网络中所有节点的 IP 地址，100 个节点
extern char ip_addr[100][16];

int recv_cmd();

#endif // __CMD_RECV_H__