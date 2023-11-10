#include "cmd_send.h"
#include "tool.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int device_num = 0; // 初始 0 个设备就绪

int send_cmd(const char* recv_addr)
{
    int cmd_socketfd; // 指令套接字
    struct sockaddr_in cmd_addr; // 指令地址
    int ret;
    int flag = 1; // 标志是否第一次发送指令

    // 创建套接字
    cmd_socketfd = socket(AF_INET, SOCK_DGRAM, 0); // UDP
    if (cmd_socketfd < 0) {
        perror("socket cmd error");
        return -1;
    }

    // 设置地址
    memset(&cmd_addr, 0, sizeof(cmd_addr));
    cmd_addr.sin_family = AF_INET;
    cmd_addr.sin_port = htons(atoi(CMD_PORT));
    cmd_addr.sin_addr.s_addr = inet_addr(recv_addr);

    struct timeval tv;
    unsigned long long detect_time, ack_time, gap; // 探测时间，应答时间，时间差
    char buf[128]; // 接收缓冲区

    struct sockaddr_in addr; // 接收地址
    socklen_t addrlen = sizeof(addr); // 接收地址长度

    while (1) {
        // 探测时间
        gettimeofday(&tv, NULL);
        detect_time = tv.tv_sec * 1000000 + tv.tv_usec; // us
        ret = sendto(cmd_socketfd, CMD_DETECT_TIME_GAP, strlen(CMD_DETECT_TIME_GAP), 0, (struct sockaddr*)&cmd_addr, sizeof(cmd_addr)); // 单播
        if (ret < 0) {
            perror("sendto detect time error");
            return -1;
        }
        printf("send_detect: %s, time: %llu\n", CMD_DETECT_TIME_GAP, detect_time); // 打印发送的数据包

        // 接收
        // ret = recvfrom(cmd_socketfd, buf, sizeof(buf), 0, NULL, NULL);
        {
            memset(buf, 0, sizeof(buf));
            ret = recvfrom(cmd_socketfd, buf, sizeof(buf), 0, (struct sockaddr*)&addr, &addrlen);
            if (ret < 0) {
                perror("recvfrom error");
                return -1;
            }
        }
        while (strcmp(inet_ntoa(addr.sin_addr), recv_addr) != 0)
            ; // 接收到的数据包不是从指定地址发来的，继续接收

        if (flag) {
            device_num++;
            flag = 0;
        }

        buf[ret] = '\0';
        gettimeofday(&tv, NULL);
        ack_time = tv.tv_sec * 1000000 + tv.tv_usec; // us
        printf("recv_ack: %s, time: %llu\n", buf, ack_time); // 打印接收的数据包

        // 计算时间差
        gap = (ack_time - detect_time) / 2;
        printf("gap: %llu\n", gap);

        // 设置时间差
        // sprintf(buf, CMD_SET_TIME_GAP(% llu), gap);
        sprintf(buf, "/setTimeGap:d,%llu;", gap);
        ret = sendto(cmd_socketfd, buf, strlen(buf), 0, (struct sockaddr*)&cmd_addr, sizeof(cmd_addr));
        if (ret < 0) {
            perror("sendto set time gap error");
            return -1;
        }
        printf("send_gap: %s\n", buf); // 打印发送的数据包
        printf("---------------cmd_end---------------\n");

        // 休眠 0.02s
        usleep(20000);
    }

    // 关闭套接字
    close(cmd_socketfd);

    return 0;
}

int send_cmd_broadcast(const char* cmd)
{
    int cmd_socketfd; // 指令套接字
    struct sockaddr_in cmd_addr; // 指令地址
    int broadcast = 1; // 广播标志
    int ret;

    // 创建套接字
    cmd_socketfd = socket(AF_INET, SOCK_DGRAM, 0); // UDP
    if (cmd_socketfd < 0) {
        perror("socket cmd error");
        return -1;
    }

    // 设置套接字选项
    ret = setsockopt(cmd_socketfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)); // 广播
    if (ret < 0) {
        perror("setsockopt cmd error");
        return -1;
    }

    // 设置地址
    memset(&cmd_addr, 0, sizeof(cmd_addr));
    cmd_addr.sin_family = AF_INET;
    cmd_addr.sin_port = htons(atoi(CMD_PORT));
    cmd_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // 发送指令
    ret = sendto(cmd_socketfd, cmd, strlen(cmd), 0, (struct sockaddr*)&cmd_addr, sizeof(cmd_addr)); // 广播
    if (ret < 0) {
        perror("sendto cmd error");
        return -1;
    }
    DEBUG_PRINT("send_cmd: %s\n", cmd); // 打印发送的数据包

    // 关闭套接字
    close(cmd_socketfd);

    return 0;
}

char cmd_buf[128]; // 存放接收的指令
char* recv_cmd_broadcast()
{
    int cmd_socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (cmd_socketfd < 0) {
        perror("socket cmd error");
        return NULL;
    }
    struct sockaddr_in cmd_addr; // 指令地址

    // 设置端口复用
    int opt = 1;
    if (setsockopt(cmd_socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt time error");
        return NULL;
    }

    set_bind_addr(cmd_socketfd, &cmd_addr, INADDR_BROADCAST, CMD_PORT);

    memset(cmd_buf, 0, sizeof(cmd_buf));

    // 接收命令
    if (recvfrom(cmd_socketfd, cmd_buf, sizeof(cmd_buf), 0, NULL, NULL) < 0) {
        perror("recvfrom cmd error");
        return NULL;
    }
    DEBUG_PRINT("recv_cmd: %s\n", cmd_buf);

    // 关闭套接字
    close(cmd_socketfd);

    return cmd_buf;
}

// 判段是否是 IP 地址
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

int sync_ip()
{
    while (1) {
        // 广播发送获取 IP 指令
        send_cmd_broadcast(CMD_GET_IP);
        usleep(100000); // 100ms
    }
}