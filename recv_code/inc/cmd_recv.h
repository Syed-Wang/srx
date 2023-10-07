#ifndef __CMD_RECV_H__
#define __CMD_RECV_H__

#define CMD_PORT "8112" // 指令端口
#define CMD_DETECT_TIME_GAP "/detectTimeGap:d,1;" // 检测时间差
#define CMD_ACK_TIME_GAP "/ackTimeGap:d,1;" // 应答时间差
#define CMD_SET_TIME_GAP(gap) "/setTimeGap:d," #gap ";" // 设置时间差
#define CMD_START "/setup:d,1;" // 开始指令
#define CMD_TIME_SERVER_STOP "/stopTimeServer:d,0;" // 停止授时服务器
#define CMD_SET_TIME_SERVER(ip) "/setTimeServer:s," #ip ";"

int recv_cmd();

/**
 * @brief 接收指令函数(广播)
 *
 * @return
 */
char* recv_cmd_broadcast();

#endif // __CMD_RECV_H__