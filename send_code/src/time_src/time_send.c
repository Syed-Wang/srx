#include "time_send.h"
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

int send_time()
{
    int time_socketfd; // 时间套接字
    struct sockaddr_in time_addr; // 时间地址
    time_packet_t packet; // 时间包
    int broadcast = 1; // 广播标志
    int ret;

    // 创建套接字
    time_socketfd = socket(AF_INET, SOCK_DGRAM, 0); // UDP
    if (time_socketfd < 0) {
        perror("socket time error");
        return -1;
    }

    // 设置套接字选项
    ret = setsockopt(time_socketfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)); // 广播
    if (ret < 0) {
        perror("setsockopt time error");
        return -1;
    }

    // 设置地址
    memset(&time_addr, 0, sizeof(time_addr));
    time_addr.sin_family = AF_INET;
    time_addr.sin_port = htons(atoi(TIME_PORT));
    time_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    // 发送时间
    memset(&packet, 0, sizeof(packet));
    strcpy(packet.head, PACKET_HEAD);
    packet.net_id = 0x01;
    packet.priority = 0x01;
    packet.net_flag = 0x01;
    struct timeval tv;
    while (1) {
        gettimeofday(&tv, NULL);
        
        packet.time = tv.tv_sec * 1000000 + tv.tv_usec; // us
        ret = sendto(time_socketfd, &packet, sizeof(packet), 0, (struct sockaddr*)&time_addr, sizeof(time_addr)); // 广播
        if (ret < 0) {
            perror("sendto time error");
            return -1;
        }
        printf("---------------time_end---------------\n");
        sleep(1); // 1s
    }

    // 关闭套接字
    close(time_socketfd);

    return 0;
}
