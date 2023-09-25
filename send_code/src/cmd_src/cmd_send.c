#include "cmd_send.h"
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

int send_cmd()
{
    int cmd_socketfd; // 指令套接字
    struct sockaddr_in cmd_addr; // 指令地址
    int ret;

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
    cmd_addr.sin_addr.s_addr = inet_addr(UNICAST_IP);

    struct timeval tv;
    unsigned long long detect_time, ack_time, gap; // 探测时间，应答时间，时间差
    char buf[128]; // 接收缓冲区
    char ip_addr[100][16]; // 记录网络中所有节点的 IP 地址，100 个节点
    memset(ip_addr, 0, sizeof(ip_addr)); // 初始化
    memcpy(ip_addr[0], "192.168.0.180", 16); // 本机IP

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
        ret = recvfrom(cmd_socketfd, buf, sizeof(buf), 0, NULL, NULL);
        if (ret < 0) {
            perror("recvfrom error");
            return -1;
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