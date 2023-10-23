#include "cmd.h"
#include "sys_config.h"
#include "tool.h"
#include <pthread.h>
#include <signal.h> // signal()
#include <stdio.h>
#include <stdlib.h> // system()
#include <string.h>
#include <sys/time.h> // setitimer()

// 定时器处理函数
void timer_handler(int signum)
{
    if (send_time_broadcast() < 0) {
        PTRERR("send_time_broadcast error");
        exit(1);
    }
}

// 接收广播指令然后处理线程
void* thread_recv_deal_cmd(void* arg)
{
    struct sockaddr_in cmd_addr; // 指令地址
    char cmd[CMD_BUF_SIZE] = { 0 }; // 指令缓冲区

    while (1) {
        memset(cmd, 0, sizeof(cmd));
        if (recv_cmd(&cmd_addr, cmd) < 0) {
            PTRERR("recv_cmd error");
            exit(1);
        }
        PTR_DEBUG("recv_cmd: %s\n", cmd); // 打印接收的指令
        if (cmd_handler(&cmd_addr, cmd) < 0) {
            PTRERR("cmd_handler error");
            exit(1);
        }
    }
}

// 广播发送授时包线程
void* thread_send_time(void* arg)
{
    // setitimer 定时器发送授时包
    struct itimerval itv;
    itv.it_interval.tv_sec = 0; // 定时器间隔时间
    itv.it_interval.tv_usec = 16000; // 16ms
    itv.it_value.tv_sec = 0; // 定时器启动时间
    itv.it_value.tv_usec = 16000; // 16ms

    if (signal(SIGALRM, timer_handler) == SIG_ERR) { // 注册信号处理函数
        PTRERR("signal error");
        exit(1);
    }

    if (setitimer(ITIMER_REAL, &itv, NULL) < 0) { // ITIMER_REAL:真实时间
        PTRERR("setitimer error");
        exit(1);
    }
}

// 网络延时校正线程
void* thread_delay_correct(void* arg)
{
    while (1) {
        if (detect_time_gap() < 0) {
            PTRERR("detect_time_gap error");
            exit(1);
        }
    }
}

int main(int argc, const char* argv[])
{
    // 1. 每个节点独立配置IP地址和掩码
    if (system("udhcpc -i eth0 -b") < 0) { // 申请IP地址 -i eth0:指定网卡 -b:后台运行
        PTRERR("system udhcpc error");
        return -1;
    }
    if (get_local_ip("eth0", local_ip) < 0) { // 获取本机IP地址
        PTRERR("get_local_ip error");
        return -1;
    }

    // 2. 创建接收广播指令然后处理线程
    pthread_t tid_cmd;
    if (pthread_create(&tid_cmd, NULL, thread_recv_deal_cmd, NULL) != 0) {
        PTR_PERROR("pthread_create cmd error");
        return -1;
    }

    // 2. 初始化当前设备系统配置，保存到JSON文件
    fps_t fps = {
        .frame_rate = 60,
        .sync_encoder = 0,
        .priority = 0,
        .net_flag = 0,
        .ip = ip
    };
    window_t window = {
        .id = 1,
        .src_ip = "192.168.0.180",
        .x = 0,
        .y = 0,
        .w = 1920,
        .h = 1080
    };
    if (save_sys_config(&fps, &window) < 0) {
        PTRERR("save_sys_config error");
        return -1;
    }

    // 3. 创建广播发送授时包线程
    pthread_t tid_time;
    if (pthread_create(&tid_time, NULL, thread_send_time, NULL) != 0) {
        PTR_PERROR("pthread_create time error");
        return -1;
    }
    // 4. 创建网络延时校正线程
    pthread_t tid_delay_correct;
    if (pthread_create(&tid_delay_correct, NULL, thread_delay_correct, NULL) != 0) {
        PTR_PERROR("pthread_create delay_correct error");
        return -1;
    }

    return 0;
}
