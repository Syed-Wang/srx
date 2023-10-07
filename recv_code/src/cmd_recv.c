#include "cmd_recv.h"
#include "tool.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int recv_cmd()
{
    int cmd_socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (cmd_socketfd < 0) {
        perror("socket cmd error");
        return -1;
    }
    struct sockaddr_in cmd_addr; // 指令地址

    // 设置端口复用
    int opt = 1;
    if (setsockopt(cmd_socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt time error");
        return -1;
    }

    if (set_bind_addr(cmd_socketfd, &cmd_addr, INADDR_ANY, CMD_PORT) < 0) {
        printf("set_bind_addr cmd1 error");
        return -1;
    }

    // 记录100次 gap 值中的最大值和最小值，并计算平局值
    unsigned long long gap_max = 0;
    unsigned long long gap_min = 0;
    unsigned long long gap_sum = 0;
    unsigned long long gap_avg = 0;
    int i = 0, j = 0;
    char cmd_buf[128]; // 存放接收的指令
    struct sockaddr_in send_addr; // 发送者地址
    socklen_t server_addr_len = sizeof(send_addr); // 发送者地址长度
    unsigned long long gap; // 时间差(微秒)

    // 接收命令
    while (1) {
        // 接收探测时间差指令
        memset(cmd_buf, 0, sizeof(cmd_buf));
        if (recvfrom(cmd_socketfd, cmd_buf, sizeof(cmd_buf), 0, (struct sockaddr*)&send_addr, &server_addr_len) < 0) {
            perror("recvfrom cmd error");
            return -1;
        }
        DEBUG_PRINT("recv_detect: %s\n", cmd_buf);

        // 应答时间差
        if (strcmp(cmd_buf, CMD_DETECT_TIME_GAP) == 0) {
            if (sendto(cmd_socketfd, CMD_ACK_TIME_GAP, strlen(CMD_ACK_TIME_GAP), 0, (struct sockaddr*)&send_addr, server_addr_len) < 0) {
                perror("sendto cmd error");
                return -1;
            }
            DEBUG_PRINT("send_ack: %s\n", CMD_ACK_TIME_GAP);
        }

        // 接收设置时间差指令
        memset(cmd_buf, 0, sizeof(cmd_buf));
        if (recvfrom(cmd_socketfd, cmd_buf, sizeof(cmd_buf), 0, (struct sockaddr*)&send_addr, &server_addr_len) < 0) {
            perror("recvfrom cmd error");
            return -1;
        }
        DEBUG_PRINT("recv_set: %s\n", cmd_buf);
        // 设置时间差...
        // sscanf(cmd_buf, CMD_SET_TIME_GAP(gap), &gap);
        sscanf(cmd_buf, "/setTimeGap:d,%llu;", &gap);
        DEBUG_PRINT("gap: %llu\n", gap);
        DEBUG_PRINT("-------------cmd_end-------------\n");

        /* --------------------------------------------------- */
        // 记录100次 gap 值中的最大值和最小值，并计算平局值
        if (i == 0) {
            gap_max = gap;
            gap_min = gap;
        }
        if (gap > gap_max)
            gap_max = gap;
        if (gap < gap_min)
            gap_min = gap;
        gap_sum += gap;
        if (i == 99) {
            gap_sum -= gap_max;
            gap_sum -= gap_min;
            gap_avg = gap_sum / 98;
            DEBUG_PRINT("gap_max: %llu\n", gap_max);
            DEBUG_PRINT("gap_min: %llu\n", gap_min);
            DEBUG_PRINT("gap_avg_98: %llu\n", gap_avg);
            DEBUG_PRINT("-------------gap_end-------------\n");
            // 重新计数
            gap_max = 0;
            gap_min = 0;
            gap_sum = 0;
            gap_avg = 0;
            i = 0;
            // 退出线程
            // pthread_exit(NULL);
        } else {
            i++;
        }
        /* --------------------------------------------------- */
    }

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

    if (set_bind_addr(cmd_socketfd, &cmd_addr, INADDR_BROADCAST, CMD_PORT) < 0) {
        printf("set_bind_addr cmd2 error");
        return NULL;
    }

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