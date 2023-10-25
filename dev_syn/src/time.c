#include "time.h"
#include "sys_config.h"
#include "tool.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

int send_time_broadcast()
{
    int time_socketfd; // 时间套接字
    struct sockaddr_in time_addr; // 时间地址
    time_packet_t time_packet; // 时间包
    int broadcast = 1; // 广播标志
    int ret;

    // 创建套接字
    time_socketfd = socket(AF_INET, SOCK_DGRAM, 0); // UDP
    if (time_socketfd < 0) {
        PTR_PERROR("socket time error");
        return -1;
    }

    // 设置套接字选项
    ret = setsockopt(time_socketfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)); // 广播
    if (ret < 0) {
        PTR_PERROR("setsockopt time error");
        return -1;
    }

    // 设置地址
    memset(&time_addr, 0, sizeof(time_addr));
    time_addr.sin_family = AF_INET;
    time_addr.sin_port = htons(atoi(TIME_PORT));
    time_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // 发送时间
    memset(&time_packet, 0, sizeof(time_packet));
    strcpy(time_packet.head, PACKET_HEAD);
    time_packet.net_id = net_id;
    time_packet.priority = fps.priority; // 优先级
    time_packet.net_flag = fps.net_flag; // 组网标志
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_packet.time = tv.tv_sec * 1000000 + tv.tv_usec; // us
    ret = sendto(time_socketfd, &time_packet, sizeof(time_packet), 0, (struct sockaddr*)&time_addr, sizeof(time_addr)); // 广播
    if (ret < 0) {
        PTR_PERROR("sendto time error");
        return -1;
    }

    // 关闭套接字
    close(time_socketfd);

    return 0;
}

int recv_time(struct sockaddr_in* addr, time_packet_t* time_packet)
{
    int time_socketfd; // 时间套接字
    struct sockaddr_in time_addr; // 时间地址

    // 创建套接字
    time_socketfd = socket(AF_INET, SOCK_DGRAM, 0); // UDP
    if (time_socketfd < 0) {
        PTR_PERROR("socket time error");
        return -1;
    }

    // 设置端口复用
    int opt = 1;
    if (setsockopt(time_socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        PTR_PERROR("setsockopt time error");
        return -1;
    }

    // 设置绑定地址
    if (set_bind_addr(time_socketfd, &time_addr, INADDR_ANY, TIME_PORT) < 0) {
        PTRERR("set_bind_addr time error");
        return -1;
    }

    socklen_t addr_len = sizeof(struct sockaddr_in);
    // 接收时间
    if (recvfrom(time_socketfd, time_packet, sizeof(time_packet_t), 0, (struct sockaddr*)addr, &addr_len) < 0) {
        PTR_PERROR("recvfrom time error");
        return -1;
    }

    PTR_DEBUG("%s\n", time_packet->head);

    close(time_socketfd);

    return 0;
}