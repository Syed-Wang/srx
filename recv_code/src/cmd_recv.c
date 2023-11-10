#include "cmd_recv.h"
#include "tool.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

unsigned char request_flag = 0; // 请求模式标志

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