#include "tool.h"
#include <arpa/inet.h> // htons
#include <ifaddrs.h> // getifaddrs
#include <netdb.h> // getnameinfo
#include <stdio.h> // perror
#include <stdlib.h> // atoi
#include <string.h> // memset
#include <sys/socket.h> // socket bind
#include <sys/time.h> // gettimeofday

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
        PTR_PERROR("bind error");
        return -1;
    }

    return ret;
}

unsigned long long get_time_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec; // us
}

int get_local_ip(const char* ifname, char* ip)
{
    struct ifaddrs *ifaddr, *ifa; // ifaddr 链表头指针 ifa 链表指针
    int ret;
    ret = getifaddrs(&ifaddr); // 获取本地网卡信息
    if (ret < 0) {
        PTR_PERROR("getifaddrs error");
        return -1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, ifname) == 0) {
            struct sockaddr_in* pAddr = (struct sockaddr_in*)ifa->ifa_addr;
            strcpy(ip, inet_ntoa(pAddr->sin_addr));
            break;
        }
    }
    freeifaddrs(ifaddr);

    return 0;
}

int is_ip(const char* ip)
{
    int a, b, c, d;
    if (sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
        return 0;
    if (a < 0 || a > 255)
        return 0;
    if (b < 0 || b > 255)
        return 0;
    if (c < 0 || c > 255)
        return 0;
    if (d < 0 || d > 255)
        return 0;
    return 1;
}
