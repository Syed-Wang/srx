#ifndef __CMD_H__
#define __CMD_H__

#include <netinet/in.h> // sockaddr_in

#define CMD_BUF_SIZE 128 // 指令缓冲区大小
#define CMD_PORT "8112" // 指令端口
#define CMD_GET_IP "/getIP:s,ip;" // 获取IP指令
#define CMD_SET_NETWORK_ID(ip, id) "/setNetworkID:s," #ip "," #id ";" // 选择设备进行组网，并分配组网ID
#define CMD_DETECT_TIME_GAP "/detectTimeGap:d,1;" // 检测时间差
#define CMD_ACK_TIME_GAP "/ackTimeGap:d,1;" // 应答时间差
#define CMD_SET_TIME_GAP(gap) "/setTimeGap:d," #gap ";" // 设置时间差
#define CMD_START "/setup:d,1;" // 开始指令

extern char ip[128][16]; // ip 数组
extern int net_id; // 组网 ID
extern char local_ip[16]; // 本机 IP

/**
 * @brief 广播发送指令函数
 *
 * @param cmd 指令
 * @return int 成功返回 0，失败返回 -1
 */
int send_cmd_broadcast(const char* cmd);

/**
 * @brief 单播发送指令函数
 * 
 * @param addr 接收方地址
 * @param cmd 指令
 * @return int 成功返回 0，失败返回 -1
 */
int send_cmd_unicast(struct sockaddr_in* addr, const char* cmd);

/**
 * @brief 接收指令函数(本机任意地址)
 *
 * @param addr 存放发送方地址
 * @param cmd 存放接收的指令
 * @return int 成功返回 0，失败返回 -1
 */
int recv_cmd(struct sockaddr_in* addr, char* cmd);

/**
 * @brief 指令处理函数
 *
 * @param addr 发送方地址
 * @param cmd 指令
 * @return int 成功返回 0，失败返回 -1
 */
int cmd_handler(struct sockaddr_in* addr, const char* cmd);

/**
 * @brief 网络延时校正函数(单播)
 *
 * @note 服务端定期向网络中的客户端发送校正包，分别记录发包时间；服务端收到客户端应答后，记录该客户端收包时间；计算时间差，再发送到客户端
 */
int detect_time_gap();

#endif // __CMD_H__