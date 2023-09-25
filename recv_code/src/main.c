// 多线程分别接收广播time 组播H264 单播cmd
#include "cmd_recv.h"
#include "h264_recv.h"
#include "time_recv.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// 线程函数
void* thread_time(void* arg)
{
    if (recv_time() < 0) {
        perror("recv_time error");
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
    pthread_t tid[3]; // 线程ID
    if (pthread_create(&tid[0], NULL, thread_time, NULL) != 0) {
        perror("pthread_create time error");
        exit(1);
    }
    if (pthread_create(&tid[1], NULL, thread_cmd, NULL) != 0) {
        perror("pthread_create cmd error");
        exit(1);
    }
    if (pthread_create(&tid[2], NULL, thread_h264, NULL) != 0) {
        perror("pthread_create h264 error");
        exit(1);
    }

    // 等待h264线程结束
    pthread_join(tid[1], NULL);

    return 0;
}
