#include "cmd.h"
#include "sys_config.h"
#include "time.h"
#include "tool.h"
#include <pthread.h>
#include <signal.h> // signal()
#include <stdio.h>
#include <stdlib.h> // system()
#include <string.h>
#include <sys/time.h> // setitimer()

struct sockaddr_in time_addr; // 授时包发送端地址
struct sockaddr_in cmd_addr; // 指令发送端地址
char cmd[CMD_BUF_SIZE] = { 0 }; // 指令缓冲区
time_packet_t time_packet; // 时间包

// 定时器处理函数
void timer_handler(int signum)
{
    if (server_client_flag == 1) { // 服务器
        if (send_time_broadcast() < 0) {
            PTRERR("send_time_broadcast error");
            exit(1);
        }
    }
}

// 接收授时包线程(所有设备始终保持接收授时包)
void* thread_recv_time(void* arg)
{
    while (1) {
        memset(&time_packet, 0, sizeof(time_packet));
        if (recv_time(&time_addr, &time_packet) < 0) {
            PTRERR("recv_time error");
            exit(1);
        }
    }
}

// 接收指令线程
void* thread_recv_cmd(void* arg)
{
    while (1) {
        memset(cmd, 0, sizeof(cmd));
        if (recv_cmd(&cmd_addr, cmd) < 0) {
            PTRERR("recv_cmd error");
            exit(1);
        }
    }
}

// 处理指令线程
void* thread_deal_cmd(void* arg)
{
    while (1) {
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

// 确认所有客户端是否就绪线程
void* thread_confirm_ready(void* arg)
{
    int ret = -1;
    while (1) {
        ret = confirm_ready();
        if (ret == -1) {
            PTRERR("confirm_ready error");
            exit(1);
        } else if (ret == 1) { // 所有客户端就绪
            // break;
            if (send_cmd_broadcast(CMD_START) < 0) {
                PTRERR("send_cmd_broadcast error");
                exit(1);
            }
        }
    }
}

int main(int argc, const char* argv[])
{
// 1. 每个节点独立配置 IP 地址和掩码
#ifdef ARM
    if (system("udhcpc -i eth0 -b") < 0) { // 申请IP地址 -i eth0:指定网卡 -b:后台运行
        PTRERR("system udhcpc error");
        return -1;
    }
    if (get_local_ip("eth0", local_ip) < 0) { // 获取本机IP地址
        PTRERR("get_local_ip error");
        return -1;
    }
#endif
#ifdef X86
    if (get_local_ip("ens33", local_ip) < 0) { // 获取本机IP地址
        PTRERR("get_local_ip error");
        return -1;
    }
#endif

    /* // 2. 初始化当前设备系统配置，保存到JSON文件
    fps.frame_rate = 60; // 帧率
    fps.sync_encoder = 0; // 同步编码器
    fps.priority = 0; // 优先级
    fps.net_flag = 0; // 组网标志
    fps.ip = ip; // IP 地址表

    window.id = 1; // 设备 id
    strcpy(window.src_ip, local_ip); // 数据源 IP
    window.x = 0; // 窗口 x 坐标
    window.y = 0; // 窗口 y 坐标
    window.w = 1920; // 窗口宽度
    window.h = 1080; // 窗口高度
    if (save_sys_config(&fps, &window) < 0) {
        PTRERR("save_sys_config error");
        return -1;
    } */
    // 2. 加载当前设备系统配置，从JSON文件中加载
    if (load_sys_config(&fps, &window) < 0) {
        PTRERR("load_sys_config error");
        return -1;
    }

    // 3. 根据组网标志，设置组网方式
    if (fps.net_flag == 0) { // 指定服务器
        server_client_flag = 0;
    } else {
        server_client_flag = 1;
    }

    // 创建接收授时包线程
    pthread_t tid_recv_time;
    if (pthread_create(&tid_recv_time, NULL, thread_recv_time, NULL) != 0) {
        PTR_PERROR("pthread_create recv_time error");
        return -1;
    }
    // 创建接收指令线程
    pthread_t tid_recv;
    if (pthread_create(&tid_recv, NULL, thread_recv_cmd, NULL) != 0) {
        PTR_PERROR("pthread_create recv error");
        return -1;
    }
    // 创建处理指令线程
    pthread_t tid_cmd;
    if (pthread_create(&tid_cmd, NULL, thread_deal_cmd, NULL) != 0) {
        PTR_PERROR("pthread_create cmd error");
        return -1;
    }
    // 创建广播发送授时包线程
    pthread_t tid_time;
    if (pthread_create(&tid_time, NULL, thread_send_time, NULL) != 0) {
        PTR_PERROR("pthread_create time error");
        return -1;
    }
    // 创建网络延时校正线程
    pthread_t tid_delay_correct;
    if (pthread_create(&tid_delay_correct, NULL, thread_delay_correct, NULL) != 0) {
        PTR_PERROR("pthread_create delay_correct error");
        return -1;
    }
    // 创建确认所有客户端是否就绪线程
    pthread_t tid_confirm_ready;
    if (pthread_create(&tid_confirm_ready, NULL, thread_confirm_ready, NULL) != 0) {
        PTR_PERROR("pthread_create confirm_ready error");
        return -1;
    }

    // 5. 等待线程结束
    pthread_join(tid_cmd, NULL);

    return 0;
}
