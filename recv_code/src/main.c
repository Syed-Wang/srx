// 多线程分别接收广播time 组播H264 单播cmd
#include "cmd_recv.h"
#include "h264_recv.h"
#include "sys_config.h"
#include "time_recv.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tool.h>

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

// 保持同步网络中的节点IP线程
void* thread_sync_ip(void* arg)
{
    if (sync_ip() < 0) {
        perror("sync_ip error");
        exit(1); // 退出进程
    }
}

int main(int argc, char* argv[])
{
    pthread_t tid[4]; // 线程ID
    pthread_t tid_sync_ip; // 保持同步网络中的节点IP线程ID
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
    /*     if (pthread_create(&tid_sync_ip, NULL, thread_sync_ip, NULL) != 0) {
            perror("pthread_create sync_ip error");
            exit(1);
        } */

    // 系统配置
    fps_t fps = { 60, 0, 0, 0 }; // 帧率结构体
    window_t window = { 1, "192.168.0.180", 0, 0, 1920, 1080 }; // 布局结构体
    if (save_sys_config(&fps, &window) < 0) {
        perror("save_sys_config error");
        exit(1);
    }
    /*     fps_t fps_test;
        window_t window_test;
        if (load_sys_config(&fps_test, &window_test) < 0) {
            perror("load_sys_config error");
            exit(1);
        }
        printf("fps_test.frame_rate: %d\n", fps_test.frame_rate); */

    // 等待接收 /stopTimeServer:d,0; 指令 (广播) 和 修改授时服务器 /setTimeServer:s," #ip "; 指令 (广播)
    char buf[128] = { 0 }; // 接收缓冲区
    char ip_buf[16] = { 0 };
    int i;
    unsigned char flag; // 组网标志
    while (1) {
        recv_cmd_broadcast();
        memset(buf, 0, sizeof(buf));
        if (strcmp(cmd_buf, CMD_TIME_SERVER_STOP) == 0) { // 停止授时服务器
            printf("stop time server\n");
            pthread_cancel(tid[1]);
            memset(ip_buf, 0, sizeof(ip_buf));
            get_local_ip("eth0", ip_buf); // 获取本机IP地址
            sprintf(buf, "/setTimeServer:s,%s;", ip_buf);
            // 等待接收修改授时服务器指令，判断 IP 是否为本机 IP
            if (strcmp(recv_cmd_broadcast(), buf) == 0) { // 接收要修改授时服务器的 IP
                // 是本机 IP，创建发送授时线程
                if (pthread_create(&tid[1], NULL, thread_send_time, NULL) != 0) {
                    perror("pthread_create time error");
                    exit(1);
                }
                printf("start time server\n");
            }
        } else if (strcmp(cmd_buf, CMD_SET_IP) == 0) { // 设置 IP (每个节点独立配置IP地址)
            printf("set ip\n");
            // 独立配置IP地址和掩码 (DHCP)
            sprintf(buf, "udhcpc -i eth0 -b"); // -i 网卡接口 -b 后台运行
            if (system(buf) < 0) {
                perror("system error");
                exit(1);
            }
        } else if (strcmp(cmd_buf, CMD_GET_IP) == 0) { // 获取每个节点的 IP
            DEBUG_PRINT("get ip\n");
            // 返回发送本机IP地址
            memset(ip_buf, 0, sizeof(ip_buf));
            get_local_ip("eth0", ip_buf); // 获取本机IP地址
            send_cmd_broadcast(ip_buf);
        } else if (strncmp(cmd_buf, "/setNetworkID:s,",16) == 0) { // 选择设备进行组网，并分配组网ID
            memset(ip_buf, 0, sizeof(ip_buf));
            get_local_ip("eth0", ip_buf); // 获取本机IP地址
            if (strcmp(ip_buf, cmd_buf + 16) == 0) { // 判断是否为本机IP地址
                printf("set network id\n");
                // 获取组网ID
                net_id = atoi(cmd_buf + 16 + strlen(ip_buf) + 1);
                printf("net_id: %d\n", net_id);
            }
        } else if (is_ip(cmd_buf)) { // 判断是否为 IP 地址
            // 是 IP 地址，保存到 ip 数组
            for (i = 0; i < sizeof(ip) / sizeof(ip[0]); i++) {
                if (strcmp(ip[i], cmd_buf) == 0) {
                    // 已存在，跳过
                    break;
                }
                if (strcmp(ip[i], "") == 0) {
                    // 不存在，保存
                    strcpy(ip[i], cmd_buf);
                    break;
                }
            }
        } else if (strncmp(cmd_buf, "/setSYSTimeServer:s,", 20) == 0) { // 系统配置授时服务器
            sscanf(cmd_buf, "/setSYSTimeServer:s,%hhu;", &flag);
            if (set_net_flag(&fps, flag) < 0) {
                perror("set_net_flag error");
                exit(1);
            }
            if (save_sys_config(&fps, &window) < 0) {
                perror("save_sys_config error");
                exit(1);
            }
        }
    }

    // 等待h264线程结束
    // pthread_join(tid[1], NULL);

    return 0;
}
