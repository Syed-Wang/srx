#ifndef __CMD_SEND_H__
#define __CMD_SEND_H__

#define CMD_PORT "8112" // 指令端口
#define UNICAST_IP "192.168.0.100" // 单播地址
#define CMD_DETECT_TIME_GAP "/detectTimeGap:d,1;" // 检测时间差
#define CMD_ACK_TIME_GAP "/ackTimeGap:d,1;" // 应答时间差
#define CMD_SET_TIME_GAP(gap) "/setTimeGap:d," #gap ";" // 设置时间差

#endif // __CMD_SEND_H__