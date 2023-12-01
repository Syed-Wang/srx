#include "tool.h"
#include <arpa/inet.h> // htons
#include <ifaddrs.h> // getifaddrs
#include <netdb.h> // getnameinfo
#include <stdio.h> // perror
#include <stdlib.h> // atoi
#include <string.h> // memset
#include <sys/socket.h> // socket bind
#include <sys/time.h> // gettimeofday
#include <unistd.h> // close

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

int send_file(struct sockaddr_in* addr, const char* file_path)
{
    int ret;
    FILE* fp;
    char buf[FILE_BUF_SIZE] = { 0 };
    int file_len = 0; // 文件长度
    int send_len = 0; // 发送长度
    int send_total = 0; // 发送总长度
    int send_percent = 0; // 发送百分比
    int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfd < 0) {
        PTR_PERROR("socket error");
        return -1;
    }

    // 打开文件
    fp = fopen(file_path, "rb");
    if (fp == NULL) {
        PTR_PERROR("fopen error");
        return -1;
    }

    // 获取文件长度
    fseek(fp, 0, SEEK_END);
    file_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // 先发送文件长度
    ret = sendto(socketfd, &file_len, sizeof(file_len), 0, (struct sockaddr*)addr, sizeof(*addr));
    if (ret < 0) {
        PTR_PERROR("sendto error");
        return -1;
    }

    // 发送文件
    while (1) {
        memset(buf, 0, sizeof(buf));
        ret = fread(buf, 1, sizeof(buf), fp);
        if (ret < 0) {
            PTR_PERROR("fread error");
            return -1;
        }
        send_len = sendto(socketfd, buf, ret, 0, (struct sockaddr*)addr, sizeof(*addr));
        if (send_len < 0) {
            PTR_PERROR("sendto error");
            return -1;
        }
        send_total += send_len;
        send_percent = (int)((float)send_total / file_len * 100);
        printf("\rsend file: %d/%d %d%%", send_total, file_len, send_percent);
        fflush(stdout);
        if (send_total >= file_len) {
            break;
        }
    }
    printf("\n");

    // 关闭文件
    fclose(fp);
    close(socketfd);

    return 0;
}

int recv_file(const char* port, const char* file_path)
{
    int ret;
    FILE* fp;
    char buf[FILE_BUF_SIZE] = { 0 };
    int file_len = 0; // 文件长度
    int recv_len = 0; // 接收长度
    int recv_total = 0; // 接收总长度
    int recv_percent = 0; // 接收百分比
    int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfd < 0) {
        PTR_PERROR("socket error");
        return -1;
    }
    struct sockaddr_in bind_addr;

    // 设置端口复用
    int opt = 1;
    ret = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret < 0) {
        PTR_PERROR("setsockopt error");
        return -1;
    }

    // 绑定地址
    ret = set_bind_addr(socketfd, &bind_addr, INADDR_ANY, port);
    if (ret < 0) {
        PTRERR("set_bind_addr error");
        return -1;
    }

    // 打开文件
    fp = fopen(file_path, "wb");
    if (fp == NULL) {
        PTR_PERROR("fopen error");
        return -1;
    }

    // 先接收文件长度
    ret = recvfrom(socketfd, &file_len, sizeof(file_len), 0, NULL, NULL);
    if (ret < 0) {
        PTR_PERROR("recvfrom error");
        return -1;
    }

    // 接收文件
    while (1) {
        memset(buf, 0, sizeof(buf));
        recv_len = recvfrom(socketfd, buf, sizeof(buf), 0, NULL, NULL);
        if (recv_len < 0) {
            PTR_PERROR("recvfrom error");
            return -1;
        }
        ret = fwrite(buf, 1, recv_len, fp);
        if (ret < 0) {
            PTR_PERROR("fwrite error");
            return -1;
        }
        recv_total += recv_len;
        recv_percent = (int)((float)recv_total / file_len * 100);
        printf("\rrecv file: %d/%d %d%%", recv_total, file_len, recv_percent);
        fflush(stdout);
        if (recv_total >= file_len) {
            break;
        }
    }
    printf("\n");

    // 关闭文件
    fclose(fp);
    close(socketfd);

    return 0;
}