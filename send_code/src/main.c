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

// 线程函数
void* thread_time(void* arg)
{
    // 发送时间
    send_time();
    return NULL;
}

void* thread_cmd(void* arg)
{
    // 发送命令
    send_cmd();
    return NULL;
}

void* thread_h264(void* arg)
{
    // 发送H264码流
    send_h264(((h264_arg_t*)arg)->argc, ((h264_arg_t*)arg)->argv);
    // 发送结束，退出线程
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char** argv)
{
    // 创建线程
    pthread_t tid[3]; // 线程ID
    h264_arg_t arg; // H264参数
    arg.argc = argc;
    arg.argv = argv;
    if (pthread_create(&tid[0], NULL, thread_time, NULL) != 0) {
        perror("pthread_create time error");
        exit(1);
    }
    if (pthread_create(&tid[1], NULL, thread_cmd, NULL) != 0) {
        perror("pthread_create cmd error");
        exit(1);
    }
    if (pthread_create(&tid[2], NULL, thread_h264, &arg) != 0) {
        perror("pthread_create h264 error");
        exit(1);
    }
    // 等待h264线程结束
    pthread_join(tid[2], NULL);
    // pthread_join(tid[0], NULL);

    return 0;
}