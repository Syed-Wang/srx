// 多线程分别发送时间、命令、h264码流
// h264 线程结束后，时间、命令线程也结束
#include "cmd_send.h"
#include "h264_send.h"
#include "time_send.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int argc;
    char** argv;
} h264_arg_t;

// ip 数组
/* const char* ip[] = {
    "192.168.0.180", // 设备0 (本机)(开发板)
    "192.168.0.100", // 设备1
}; */

// 线程函数
void* thread_send_time(void* arg)
{
    // 发送时间
    if (send_time() < 0) {
        perror("send_time error");
        exit(1); // 退出进程
    }
    return NULL;
}

void* thread_recv_time(void* arg)
{
    // 接收时间
    if (recv_time() < 0) {
        perror("recv_time error");
        exit(1); // 退出进程
    }
    return NULL;
}

void* thread_cmd(void* arg)
{
    // 发送命令
    if (send_cmd((const char*)arg) < 0) {
        perror("send_cmd error");
        exit(1); // 退出进程
    }
    return NULL;
}

void* thread_h264(void* arg)
{
    // 发送H264码流
    if (send_h264(((h264_arg_t*)arg)->argc, ((h264_arg_t*)arg)->argv) < 0) {
        perror("send_h264 error");
        exit(1); // 退出进程
    }
    // 发送结束，退出线程
    // wsy 23-9-27 暂不退出线程
    // pthread_exit(NULL);
    return NULL;
}

int main(int argc, char** argv)
{
    // 创建线程
    pthread_t tid[3 + sizeof(ip) / sizeof(ip[0]) - 1]; // 线程ID (时间、h264、命令)
    h264_arg_t arg; // H264参数
    arg.argc = argc;
    arg.argv = argv;
    if (pthread_create(&tid[0], NULL, thread_send_time, NULL) != 0) {
        perror("pthread_create time error");
        exit(1);
    }
    if (pthread_create(&tid[1], NULL, thread_recv_time, NULL) != 0) {
        perror("pthread_create time error");
        exit(1);
    }
    if (pthread_create(&tid[2], NULL, thread_h264, (void*)&arg) != 0) {
        perror("pthread_create h264 error");
        exit(1);
    }
    // 给所有 IP 发送 cmd (本机除外)
    for (int i = 0; i < sizeof(ip) / sizeof(ip[0]); i++) {
        if (i == 0)
            continue; // 本机不发送 cmd
        if (pthread_create(&tid[3 + i - 1], NULL, thread_cmd, (void*)ip[i]) != 0) {
            perror("pthread_create cmd error");
            exit(1);
        }
    }

    // 确认所有客户端就绪后，服务器广播发送开始指令
    while (device_num < sizeof(ip) / sizeof(ip[0]) - 1)
        ;
    send_cmd_broadcast(CMD_START);

    // 等待接收 /stopTimeServer:d,0; 指令 (广播) 和 修改授时服务器 /setTimeServer:s," #ip "; 指令 (广播)
     char buf[128] = { 0 }; // 接收缓冲区
    while (1) {
        if (strcmp(recv_cmd_broadcast(), CMD_TIME_SERVER_STOP) == 0) {
            printf("stop time server\n");
            pthread_cancel(tid[0]);
            sprintf(buf, CMD_SET_TIME_SERVER(% s), ip[0]);
            // 等待接收修改授时服务器指令，判断 IP 是否为本机 IP
            if (strcmp(recv_cmd_broadcast(), buf) == 0) {
                // 是本机 IP，创建发送授时线程
                if (pthread_create(&tid[0], NULL, thread_send_time, NULL) != 0) {
                    perror("pthread_create time error");
                    exit(1);
                }
                printf("start time server\n");
            }
        }
    }

    // 等待h264线程结束
    // pthread_join(tid[1], NULL);
    // pthread_join(tid[0], NULL);

    return 0;
}