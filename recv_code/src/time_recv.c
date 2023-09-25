#include "time_recv.h"
#include "tool.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

int recv_time()
{
    int time_socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (time_socketfd < 0) {
        perror("socket time error");
        return -1;
    }
    struct sockaddr_in time_addr; // 时间地址
    set_bind_addr(time_socketfd, &time_addr, INADDR_BROADCAST, TIME_PORT);

    time_packet_t time_packet; // 时间包

    // 接收时间
    while (1) {
        // 接收授时包
        if (recvfrom(time_socketfd, &time_packet, sizeof(time_packet), 0, NULL, NULL) < 0) {
            perror("recvfrom time error");
            return -1;
        }
        // 打印授时包
        DEBUG_PRINT("recvfrom time: %s\n", time_packet.head);
        DEBUG_PRINT("net_id: %d\n", time_packet.net_id);
        DEBUG_PRINT("priority: %d\n", time_packet.priority);
        DEBUG_PRINT("net_flag: %d\n", time_packet.net_flag);
        DEBUG_PRINT("time: %llu\n", time_packet.time);
        DEBUG_PRINT("-------------time_end-------------\n");
    }

	close(time_socketfd);

    return 0;
}