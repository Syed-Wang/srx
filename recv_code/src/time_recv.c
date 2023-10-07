#include "time_recv.h"
#include "tool.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

// ip 数组
const char* ip[2] = {
    "192.168.0.180", // 设备0 (开发板)
    "192.168.0.100", // 设备1 (本机)
};

void* thread_send_time_1(void* arg)
{
    if (send_time() < 0) {
        perror("send_time error");
        exit(1); // 退出进程
    }
    return NULL;
}

int recv_time()
{
    int time_socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (time_socketfd < 0) {
        perror("socket time error");
        return -1;
    }
    struct sockaddr_in time_addr; // 时间地址

    // 设置端口复用
    int opt = 1;
    if (setsockopt(time_socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt time error");
        return -1;
    }

    if (set_bind_addr(time_socketfd, &time_addr, INADDR_BROADCAST, TIME_PORT) < 0) {
        printf("set_bind_addr time error");
        return -1;
    }

    time_packet_t time_packet; // 时间包
    int broadcast = 1; // 广播标志

    // 设置套接字选项
    if (setsockopt(time_socketfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) { // 广播
        perror("setsockopt time error");
        return -1;
    }

    // 设置超时时间 5s
    struct timeval timeout;
    timeout.tv_sec = 5; // 秒
    timeout.tv_usec = 0; // 微秒
    if (setsockopt(time_socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) { // 接收超时
        perror("setsockopt time error");
        return -1;
    }

    struct timeval tv;
    pthread_t tid; // 线程ID
    int flag = 0; // 线程创建标志
    struct sockaddr_in addr; // 地址
    socklen_t addr_len = sizeof(addr);
    // 接收时间
    while (1) {
        // 接收授时包
        if (recvfrom(time_socketfd, &time_packet, sizeof(time_packet), 0, (struct sockaddr*)&addr, &addr_len) < 0) {
            // 如果超时，执行发送授时包线程，继续接收
            if (errno == EAGAIN && flag == 0) {
                if (pthread_create(&tid, NULL, thread_send_time_1, NULL) != 0) {
                    perror("pthread_create time error");
                    return -1;
                }
                flag = 1;
            } else if (errno == EAGAIN && flag == 1) {
                // 如果超时，且已经创建发送授时包线程，继续接收
                continue;
            } else {
                perror("recvfrom time error");
                return -1;
            }
        }

        // 如果收到同优先级授时包，并且ip地址比当前授时ip小，并且如果授时包线程在执行则停止发送授时包，当前设备作为客户端只接收授时包
        if (strcmp(time_packet.head, PACKET_HEAD) == 0 && time_packet.priority == 0x01 && ntohl(addr.sin_addr.s_addr) < ntohl(inet_addr(ip[1])) && flag == 1) {
            pthread_cancel(tid); // 取消发送授时包线程
            flag = 0;
        }
        // 收到高优先级授时包，当前服务器转为客户端
        if (strcmp(time_packet.head, PACKET_HEAD) == 0 && time_packet.priority > 0x01 && flag == 1) {
            pthread_cancel(tid); // 取消发送授时包线程
            flag = 0;
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

int send_time()
{
    int time_socketfd; // 时间套接字
    struct sockaddr_in time_addr; // 时间地址
    time_packet_t time_packet; // 时间包
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
    time_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // 发送时间
    memset(&time_packet, 0, sizeof(time_packet));
    strcpy(time_packet.head, PACKET_HEAD);
    time_packet.net_id = 0x01;
    time_packet.priority = 0x01; // 优先级 1
    time_packet.net_flag = 0x01; // 组网标志 1
    struct timeval tv;
    while (1) {
        gettimeofday(&tv, NULL);

        time_packet.time = tv.tv_sec * 1000000 + tv.tv_usec; // us
        ret = sendto(time_socketfd, &time_packet, sizeof(time_packet), 0, (struct sockaddr*)&time_addr, sizeof(time_addr)); // 广播
        if (ret < 0) {
            perror("sendto time error");
            return -1;
        }
        // printf("---------------time_end---------------\n");
        sleep(1); // 1s
    }

    // 关闭套接字
    close(time_socketfd);

    return 0;
}
