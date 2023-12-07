#include "cmd.h"
#include "dev_time.h"
#include "h264.h"
#include "link_queue.h"
#include "sys_config.h"
#include "tool.h"
#include <pthread.h>
#include <signal.h> // signal()
#include <stdio.h>
#include <stdlib.h> // system()
#include <string.h>
#include <sys/time.h> // setitimer()
#include <unistd.h> // sleep()

struct sockaddr_in time_addr; // 授时包发送端地址
time_packet_t time_packet; // 时间包
link_queue_t* cmd_queue = NULL; // 指令队列
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 互斥锁

// 定时器处理函数
void timer_handler(int signum)
{
    if (stop_flag == 1) {
        pthread_exit(NULL);
    }
    if (server_client_flag == 1) { // 服务器
        if (send_time_broadcast() < 0) {
            PTRERR("send_time_broadcast error");
        }
    }
}

// 接收授时包线程(所有设备始终保持接收授时包)
void* thread_recv_time(void* arg)
{
    while (1) {
        if (stop_flag == 1) {
            pthread_exit(NULL);
        }
        memset(&time_packet, 0, sizeof(time_packet));
        if (recv_time(&time_addr, &time_packet) < 0) {
            PTRERR("recv_time error");
        }
    }
}

// 接收指令线程
void* thread_recv_cmd(void* arg)
{
    while (1) {
        if (stop_flag == 1) {
            pthread_exit(NULL);
        }
        if (recv_cmd(cmd_queue) < 0) {
            PTRERR("recv_cmd error");
        }
    }
}

// 处理指令线程
void* thread_deal_cmd(void* arg)
{
    while (1) {
        if (stop_flag == 1) {
            pthread_exit(NULL);
        }
        // 处理指令
        if (cmd_handler(cmd_queue) < 0) {
            PTRERR("cmd_handler error");
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
    }

    if (setitimer(ITIMER_REAL, &itv, NULL) < 0) { // ITIMER_REAL:真实时间
        PTRERR("setitimer error");
    }
}

// 网络延时校正线程
void* thread_delay_correct(void* arg)
{
    while (1) {
        if (stop_flag == 1) {
            pthread_exit(NULL);
        }
        if (server_client_flag == 1) { // 服务器
            if (detect_time_gap() < 0) {
                PTRERR("detect_time_gap error");
            }
        }
    }
}

int main(int argc, const char* argv[])
{
// 1. 每个节点独立配置 IP 地址和掩码
#ifdef ARM
    // 暂不需要申请IP地址，直接使用本地IP地址
    /* if (system("udhcpc -i eth0 -b") < 0) { // 申请IP地址 -i eth0:指定网卡 -b:后台运行
        PTRERR("system udhcpc error");
        return -1;
    } */
    if (get_local_ip("eth0", local_ip) < 0) { // 获取本机IP地址
        PTRERR("get_local_ip error");
        return -1;
    }
#elif X86
    if (get_local_ip("ens33", local_ip) < 0) { // 获取本机IP地址
        PTRERR("get_local_ip error");
        return -1;
    }
#endif

    // 2. 加载当前设备系统配置，从JSON文件中加载
    int ret = load_sys_config(&fps, &window);
    if (ret == -1) {
        PTRERR("load_sys_config error");
        return -1;
    } else if (ret == -2 || ret == -3) {
        // 如果文件不存在或为空，初始化并创建文件
        fps.frame_rate = 60; // 帧率
        fps.sync_encoder = 0; // 同步编码器
        fps.priority = 0; // 优先级
        fps.net_flag = 0; // 组网标志
        memcpy(fps.ip, ip, sizeof(ip)); // IP 地址表

        window.id = 1; // 设备 id
        strcpy(window.src_ip, local_ip); // 数据源 IP
        window.x = 0; // 窗口 x 坐标
        window.y = 0; // 窗口 y 坐标
        window.w = 1920; // 窗口宽度
        window.h = 1080; // 窗口高度
        if (save_sys_config(&fps, &window) < 0) {
            PTRERR("save_sys_config error");
            return -1;
        }
    }

    PTR_DEBUG("frame_rate: %d\n", fps.frame_rate);
    PTR_DEBUG("sync_encoder: %d\n", fps.sync_encoder);
    PTR_DEBUG("priority: %d\n", fps.priority);
    PTR_DEBUG("net_flag: %d\n", fps.net_flag);
    for (int i = 0; i < node_num; i++) {
        PTR_DEBUG("ip[%d]: %s\n", i, fps.ip[i]);
    }
    PTR_DEBUG("-----------------------------\n");
    PTR_DEBUG("id: %d\n", window.id);
    PTR_DEBUG("src_ip: %s\n", window.src_ip);
    PTR_DEBUG("x: %d\n", window.x);
    PTR_DEBUG("y: %d\n", window.y);
    PTR_DEBUG("w: %d\n", window.w);
    PTR_DEBUG("h: %d\n", window.h);
    memcpy(ip, fps.ip, sizeof(fps.ip)); // 拷贝 IP 地址表

    // 3. 根据组网标志，设置组网方式
    if (fps.net_flag == 0) { // 指定服务器
        server_client_flag = 0;
    } else {
        server_client_flag = 1;
    }

    // 4. 创建指令队列
    cmd_queue = link_queue_create();
    if (cmd_queue == NULL) {
        PTRERR("link_queue_create error");
        return -1;
    }

    /* ************************************************************************************* */

    // 1. 创建接收授时包线程
    pthread_t tid_recv_time;
    if (pthread_create(&tid_recv_time, NULL, thread_recv_time, NULL) != 0) {
        PTR_PERROR("pthread_create recv_time error");
        return -1;
    }
    // 2. 创建接收指令线程
    pthread_t tid_recv;
    if (pthread_create(&tid_recv, NULL, thread_recv_cmd, NULL) != 0) {
        PTR_PERROR("pthread_create recv error");
        return -1;
    }
    // 3. 创建处理指令线程
    pthread_t tid_cmd;
    if (pthread_create(&tid_cmd, NULL, thread_deal_cmd, NULL) != 0) {
        PTR_PERROR("pthread_create cmd error");
        return -1;
    }

    if (server_client_flag == 0 && request_mode_flag == 1) { // 客户端，请求模式(手动指定是否进入请求模式)
        // 进入请求模式，广播发送请求指令
        if (send_cmd_broadcast(CMD_REQUEST_CONNECT) < 0) { // 替换节点
            PTRERR("send_cmd_broadcast error");
            return -1;
        }
        // 接收文件
        if (recv_file(CMD_PORT, SYS_CONFIG_PATH) < 0) {
            PTRERR("recv_file error");
            return -1;
        }
        // 退出请求模式
        request_mode_flag = 0;
    }

    // 4. 创建广播发送授时包线程
    pthread_t tid_time;
    if (pthread_create(&tid_time, NULL, thread_send_time, NULL) != 0) {
        PTR_PERROR("pthread_create time error");
        return -1;
    }
    // 5. 创建网络延时校正线程
    pthread_t tid_delay_correct;
    if (pthread_create(&tid_delay_correct, NULL, thread_delay_correct, NULL) != 0) {
        PTR_PERROR("pthread_create delay_correct error");
        return -1;
    }

    // 确认所有客户端是否就绪
    ret = -1;
    while (1) {
        if (server_client_flag == 1) { // 服务器
            ret = confirm_ready();
            if (ret == -1) {
                PTRERR("confirm_ready error");
                return -1;
            } else if (ret == 1) { // 所有客户端就绪
                if (send_cmd_broadcast(CMD_START) < 0) {
                    PTRERR("send_cmd_broadcast error");
                    return -1;
                }
                sleep(1); // 等待客户端启动
                // 开始发送视频
                // BGR3(bgr24): 65543
                // ./dev_syn -w 1920 -h 1080 -t 7 -f 65543 -i /dev/video0
                if (send_h264(argc, (char**)argv) < 0) {
                    PTRERR("send_h264 error");
                    return -1;
                }
                printf("send_h264 success\n");
                break;
            }
        }
    }

    // 等待线程结束
    pthread_join(tid_recv_time, NULL);
    pthread_join(tid_recv, NULL);
    pthread_join(tid_cmd, NULL);
    pthread_join(tid_time, NULL);
    pthread_join(tid_delay_correct, NULL);

    pthread_mutex_destroy(&mutex); // 销毁互斥锁

    return 0;
}
