// 广播授时
// 单播网络延时校正
// 服务端定期向网络中的客户端发送校正包，记录发包时间；/detectTimeGap:d,1;
// 客户端收到校正包后，应答/ackTimeGap:d,1;
// 服务端收到客户端应答后，记录该客户端收包时间
// 计算时间差，发送到客户端/setTimeGap:d,gap;

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define TIME_PORT 8111 // 授时端口
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
    time_packet_t packet; // 授时包
    int broadcast = 1; // 广播标志
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

    // 设置套接字选项
    ret = setsockopt(time_socketfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)); // 广播
    if (ret < 0) {
        perror("setsockopt");
        exit(1);
    }

    // 设置地址
    memset(&time_addr, 0, sizeof(time_addr));
    time_addr.sin_family = AF_INET;
    time_addr.sin_port = htons(TIME_PORT);
    time_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    memset(&cmd_addr, 0, sizeof(cmd_addr));
    cmd_addr.sin_family = AF_INET;
    cmd_addr.sin_port = htons(CMD_PORT);
    cmd_addr.sin_addr.s_addr = inet_addr(UNICAST_IP);

    // 填充数据包
    strcpy(packet.head, "shiruixun-fenbushi-shoushi");
    packet.net_id = 0x01;
    packet.priority = 0x01;
    packet.net_flag = 0x01;
    struct timeval tv;

    unsigned long long detect_time, ack_time, gap; // 探测时间，应答时间，时间差
    char buf[128]; // 接收缓冲区
    // 发送
    while (1) {
        // 探测时间
        gettimeofday(&tv, NULL);
        detect_time = tv.tv_sec * 1000000 + tv.tv_usec; // us
        packet.time = detect_time;
        /* ret = sendto(time_socketfd, &packet, sizeof(packet), 0, (struct sockaddr*)&time_addr, sizeof(time_addr)); // 广播
        if (ret < 0) {
            perror("sendto");
            exit(1);
        } */

        ret = sendto(cmd_socketfd, CMD_DETECT_TIME_GAP, strlen(CMD_DETECT_TIME_GAP), 0, (struct sockaddr*)&cmd_addr, sizeof(cmd_addr)); // 单播
        if (ret < 0) {
            perror("sendto");
            exit(1);
        }
        printf("send_detect: %s, time: %llu\n", CMD_DETECT_TIME_GAP, detect_time); // 打印发送的数据包

        // 接收
        ret = recvfrom(cmd_socketfd, buf, sizeof(buf), 0, NULL, NULL);
        if (ret < 0) {
            perror("recvfrom");
            exit(1);
        }
        buf[ret] = '\0';
        gettimeofday(&tv, NULL);
        ack_time = tv.tv_sec * 1000000 + tv.tv_usec; // us
        printf("recv_ack: %s, time: %llu\n", buf, ack_time); // 打印接收的数据包

        // 计算时间差
        gap = (ack_time - detect_time) / 2;
        printf("gap: %llu\n", gap);

        // 设置时间差
        sprintf(buf, CMD_SET_TIME_GAP(% llu), gap);
        ret = sendto(cmd_socketfd, buf, strlen(buf), 0, (struct sockaddr*)&cmd_addr, sizeof(cmd_addr));
        if (ret < 0) {
            perror("sendto");
            exit(1);
        }
        printf("send_gap: %s\n", buf); // 打印发送的数据包
        printf("------------------------------\n");

        // 休眠
        sleep(1);
    }

    // 关闭套接字
    close(time_socketfd);
    close(cmd_socketfd);

    return 0;
}