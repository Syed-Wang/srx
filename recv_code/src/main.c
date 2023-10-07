// 多线程分别接收广播time 组播H264 单播cmd
#include "cmd_recv.h"
#include "h264_recv.h"
#include "time_recv.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 线程函数
void* thread_recv_time(void* arg)
{
    if (recv_time() < 0) {
        perror("recv_time error");
        exit(1); // 退出进程
    }
    return NULL;
}

void* thread_send_time(void* arg)
{
    if (send_time() < 0) {
        perror("send_time error");
        exit(1); // 退出进程
    }
    return NULL;
}

void* thread_cmd(void* arg)
{
    if (recv_cmd() < 0) {
        perror("recv_cmd error");
        exit(1); // 退出进程
    }
    return NULL;
}

void* thread_h264(void* arg)
{
    if (recv_h264() < 0) {
        perror("recv_h264 error");
        exit(1); // 退出进程
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    pthread_t tid[4]; // 线程ID
    if (pthread_create(&tid[0], NULL, thread_recv_time, NULL) != 0) {
        perror("pthread_create time error");
        exit(1);
    }
    if (pthread_create(&tid[2], NULL, thread_cmd, NULL) != 0) {
        perror("pthread_create cmd error");
        exit(1);
    }
    if (pthread_create(&tid[3], NULL, thread_h264, NULL) != 0) {
        perror("pthread_create h264 error");
        exit(1);
    }

    // 等待接收 /stopTimeServer:d,0; 指令 (广播) 和 修改授时服务器 /setTimeServer:s," #ip "; 指令 (广播)
    char buf[128] = { 0 }; // 接收缓冲区
    while (1) {
        if (strcmp(recv_cmd_broadcast(), CMD_TIME_SERVER_STOP) == 0) {
            printf("stop time server\n");
            pthread_cancel(tid[1]);
            sprintf(buf, CMD_SET_TIME_SERVER(% s), ip[1]);
            // 等待接收修改授时服务器指令，判断 IP 是否为本机 IP
            if (strcmp(recv_cmd_broadcast(), buf) == 0) {
                // 是本机 IP，创建发送授时线程
                if (pthread_create(&tid[1], NULL, thread_send_time, NULL) != 0) {
                    perror("pthread_create time error");
                    exit(1);
                }
                printf("start time server\n");
            }
        }
    }

    // 等待h264线程结束
    // pthread_join(tid[1], NULL);

    return 0;
}
