// 接收时间

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define PORT 8111 // 授时端口
#define CMD_PORT 8112 // 指令端口
#define BROADCAST_IP "255.255.255.255" // 广播地址
#define UNICAST_IP "192.168.0.100" // 单播地址

#define CMD_DETECT_TIME_GAP "/detectTimeGap:d,1;" // 检测时间差
#define CMD_ACK_TIME_GAP "/ackTimeGap:d,1;" // 应答时间差
#define CMD_SET_TIME_GAP(gap) "/setTimeGap:d," #gap ";" // 设置时间差

// 授时包格式
typedef struct {
    char head[64]; // 数据包头部
    unsigned char net_id; // 组网ID
    unsigned char priority; // 优先级(0-0xff 0最高)
    unsigned char net_flag; // 组网标志(指定服务器，自主)
    unsigned long long time; // 本机系统时间(us)
} time_packet_t;

int main(int argc, char* argv[])
{
    int time_socketfd; // 时间套接字
    int cmd_socketfd; // 指令套接字
    struct sockaddr_in time_addr; // 时间地址
    struct sockaddr_in cmd_addr; // 指令地址
    time_packet_t packet;
    int ret;

    // 创建套接字
    time_socketfd = socket(AF_INET, SOCK_DGRAM, 0); // UDP
    if (time_socketfd < 0) {
        perror("socket");
        exit(1);
    }
    cmd_socketfd = socket(AF_INET, SOCK_DGRAM, 0); // UDP
    if (cmd_socketfd < 0) {
        perror("socket");
        exit(1);
    }

    // 设置地址
    memset(&time_addr, 0, sizeof(time_addr));
    time_addr.sin_family = AF_INET;
    time_addr.sin_port = htons(PORT);
    time_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);
    memset(&cmd_addr, 0, sizeof(cmd_addr));
    cmd_addr.sin_family = AF_INET;
    cmd_addr.sin_port = htons(CMD_PORT);
    cmd_addr.sin_addr.s_addr = inet_addr(UNICAST_IP);

    // 绑定地址
    ret = bind(time_socketfd, (struct sockaddr*)&time_addr, sizeof(time_addr));
    if (ret < 0) {
        perror("bind");
        exit(1);
    }
    ret = bind(cmd_socketfd, (struct sockaddr*)&cmd_addr, sizeof(cmd_addr));
    if (ret < 0) {
        perror("bind");
        exit(1);
    }

    char buf[128];
    unsigned long long gap; // 时间差
    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    // 接收数据包
    while (1) {
        // 接收授时包
        /* ret = recvfrom(time_socketfd, &packet, sizeof(packet), 0, NULL, NULL);
        if (ret < 0) {
            perror("recvfrom");
            exit(1);
        }

        // 打印数据包
        printf("head: %s\n", packet.head);
        printf("net_id: %d\n", packet.net_id);
        printf("priority: %d\n", packet.priority);
        printf("net_flag: %d\n", packet.net_flag);
        printf("time: %llu\n", packet.time);
        printf("------------------------\n"); */

        // 接收探测时间差指令
        ret = recvfrom(cmd_socketfd, buf, sizeof(buf), 0, (struct sockaddr*)&server_addr, &server_addr_len);
        if (ret < 0) {
            perror("recvfrom");
            exit(1);
        }
        buf[ret] = '\0';
        printf("recv_detect: %s\n", buf);

        // 应答时间差
        ret = sendto(cmd_socketfd, CMD_ACK_TIME_GAP, strlen(CMD_ACK_TIME_GAP), 0, (struct sockaddr*)&server_addr, server_addr_len);
        if (ret < 0) {
            perror("sendto");
            exit(1);
        }
        printf("send_ack: %s\n", CMD_ACK_TIME_GAP);

        // 接收设置时间差指令
        ret = recvfrom(cmd_socketfd, buf, sizeof(buf), 0, (struct sockaddr*)&server_addr, &server_addr_len);
        if (ret < 0) {
            perror("recvfrom");
            exit(1);
        }
        buf[ret] = '\0';
        printf("recv_set: %s\n", buf);

        // 设置时间差
        sscanf(buf, "/setTimeGap:d,%llu;", &gap);
        printf("gap: %llu\n", gap);
        printf("------------------------\n");
    }

    // 关闭套接字
    close(time_socketfd);
    close(cmd_socketfd);

    return 0;
}