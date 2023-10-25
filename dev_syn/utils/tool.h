#ifndef __TOOL_H__
#define __TOOL_H__

#include <netinet/in.h> // sockaddr_in

#define PTR_PERROR(msg)                                     \
    do {                                                    \
        printf("%s %s %d: ", __FILE__, __func__, __LINE__); \
        perror(msg);                                        \
    } while (0)

#define PTRERR(msg)                                         \
    do {                                                    \
        printf("%s %s %d: ", __FILE__, __func__, __LINE__); \
        printf("%s\n", msg);                                \
    } while (0)

#ifdef DEBUG // 编译时定义 -D DEBUG
#define PTR_DEBUG(format, ...)                              \
    do {                                                    \
        printf("%s %s %d: ", __FILE__, __func__, __LINE__); \
        printf(format, ##__VA_ARGS__);                      \
    } while (0)
#else
#define PTR_DEBUG(format, ...)
#endif

/**
 * @brief 设置绑定地址
 *
 * @param socketfd socket文件描述符
 * @param addr 地址
 * @param ip ip地址
 * @param port 端口
 * @return int 0:成功 -1:失败
 */
int set_bind_addr(int socketfd, struct sockaddr_in* addr, unsigned int ip, const char* port);

/**
 * @brief 获取当前时间(us)
 *
 * @return unsigned long long 当前时间(us)
 */
unsigned long long get_time_us();

/**
 * @brief 获取本地指定网卡的ip地址
 *
 * @param ifname 网卡名称
 * @param ip ip地址
 * @return int 0:成功 -1:失败
 */
int get_local_ip(const char* ifname, char* ip);

/**
 * @brief 判断是否是 IP 地址
 *
 * @param ip ip地址
 * @return int 1:是 0:否
 */
int is_ip(const char* ip);

#endif // __TOOL_H__