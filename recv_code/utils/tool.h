#ifndef __TOOL_H__
#define __TOOL_H__

#include <netinet/in.h> // sockaddr_in

#ifdef DEBUG
#define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(format, ...)
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

#endif // __TOOL_H__