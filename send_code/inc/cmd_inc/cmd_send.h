#ifndef __CMD_SEND_H__
#define __CMD_SEND_H__

#define CMD_PORT "8112" // 指令端口
// #define UNICAST_IP "192.168.0.100" // 单播地址
#define CMD_DETECT_TIME_GAP "/detectTimeGap:d,1;" // 检测时间差
#define CMD_ACK_TIME_GAP "/ackTimeGap:d,1;" // 应答时间差
#define CMD_SET_TIME_GAP(gap) "/setTimeGap:d," #gap ";" // 设置时间差
#define CMD_START "/setup:d,1;" // 开始指令
#define CMD_TIME_SERVER_STOP "/stopTimeServer:d,0;" // 停止授时服务器
#define CMD_SET_TIME_SERVER(ip) "/setTimeServer:s," #ip ";" // 指定授时服务器
#define CMD_SET_IP "/setIP:s,ip;" // 设置 IP (每个节点独立配置IP地址)
#define CMD_GET_IP "/getIP:d,1;" // 获取 IP (每个节点独立配置IP地址) (广播)
#define CMD_SET_NETWORK_ID(ip, id) "/setNetworkID:s," #ip "," #id ";" // 选择设备进行组网，并分配组网ID

#ifdef DEBUG
#define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(format, ...)
#endif

extern int device_num; // 设备数量
extern char cmd_buf[128]; // 存放接收的指令

/**
 * @brief 发送指令函数(单播)
 *
 * @param recv_addr 接收地址
 * @return int 成功返回 0，失败返回 -1
 */
int send_cmd(const char* recv_addr);

/**
 * @brief 发送指令函数(广播)
 *
 * @param cmd 指令
 * @return int 成功返回 0，失败返回 -1
 */
int send_cmd_broadcast(const char* cmd);

/**
 * @brief 接收指令函数(广播)
 *
 * @return char* 成功返回指令，失败返回 NULL
 */
char* recv_cmd_broadcast();

/**
 * @brief 判断是否为 IP 地址
 *
 * @param ip IP 地址
 * @return int 是返回 1，否返回 0
 */
int is_ip(const char* ip);

/**
 * @brief 同步 IP (记录网络中的节点 IP 到 ip 数组)
 *
 * @return int 成功返回 0，失败返回 -1
 */
int sync_ip();

#endif // __CMD_SEND_H__