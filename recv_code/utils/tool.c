#include "tool.h"
#include <arpa/inet.h> // htons
#include <stdio.h> // perror
#include <stdlib.h> // atoi
#include <string.h> // memset
#include <sys/socket.h> // socket bind

int set_bind_addr(int socketfd, struct sockaddr_in* addr, unsigned int ip, const char* port)
{
    int ret;
    // 设置地址
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(atoi(port));
    addr->sin_addr.s_addr = ip;
    // 绑定地址
    ret = bind(socketfd, (struct sockaddr*)addr, sizeof(*addr));
    if (ret < 0) {
        perror("bind error");
        return -1;
    }
    return ret;
}