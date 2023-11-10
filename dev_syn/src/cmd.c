#include "cmd.h"
#include "h264.h"
#include "sys_config.h"
#include "tool.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h>
#include <sys/time.h>
#include <unistd.h> // close

int send_cmd_broadcast(const char* cmd)
{
    int cmd_socketfd; // 指令套接字
    struct sockaddr_in cmd_addr; // 指令地址
    int broadcast = 1; // 广播标志
    int ret;

    // 创建套接字
    cmd_socketfd = socket(AF_INET, SOCK_DGRAM, 0); // UDP
    if (cmd_socketfd < 0) {
        PTR_PERROR("socket cmd error");
        return -1;
    }
    // 设置套接字选项
    ret = setsockopt(cmd_socketfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)); // 广播
    if (ret < 0) {
        PTR_PERROR("setsockopt cmd error");
        return -1;
    }

    // 设置地址
    memset(&cmd_addr, 0, sizeof(cmd_addr));
    cmd_addr.sin_family = AF_INET;
    cmd_addr.sin_port = htons(atoi(CMD_PORT));
    cmd_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // 发送指令
    ret = sendto(cmd_socketfd, cmd, strlen(cmd), 0, (struct sockaddr*)&cmd_addr, sizeof(cmd_addr)); // 广播
    if (ret < 0) {
        PTR_PERROR("sendto cmd error");
        return -1;
    }

    PTR_DEBUG("send_cmd_broadcast: %s\n", cmd); // 打印发送的数据包

    // 关闭套接字
    close(cmd_socketfd);

    return 0;
}

int send_cmd_unicast(struct sockaddr_in* addr, const char* cmd)
{
    int cmd_socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (cmd_socketfd < 0) {
        PTR_PERROR("socket cmd error");
        return -1;
    }

    // 发送指令
    if (sendto(cmd_socketfd, cmd, strlen(cmd), 0, (struct sockaddr*)addr, sizeof(*addr)) < 0) {
        PTR_PERROR("sendto cmd error");
        return -1;
    }

    PTR_DEBUG("%s\n", cmd); // 打印发送的数据包

    // 关闭套接字
    close(cmd_socketfd);

    return 0;
}

int recv_cmd(struct sockaddr_in* addr, char* cmd)
{
    int cmd_socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (cmd_socketfd < 0) {
        PTR_PERROR("socket cmd error");
        return -1;
    }

    struct sockaddr_in cmd_addr; // 指令地址

    // 设置端口复用
    int opt = 1;
    if (setsockopt(cmd_socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        PTR_PERROR("setsockopt cmd error");
        return -1;
    }

    if (set_bind_addr(cmd_socketfd, &cmd_addr, INADDR_ANY, CMD_PORT) < 0) {
        PTRERR("set_bind_addr cmd error");
        return -1;
    }

    socklen_t server_addr_len = sizeof(*addr);
    // 接收指令
    if (recvfrom(cmd_socketfd, cmd, CMD_BUF_SIZE, 0, (struct sockaddr*)addr, &server_addr_len) < 0) {
        PTR_PERROR("recvfrom cmd error");
        return -1;
    }

    PTR_DEBUG("recv_cmd: %s\n", cmd);

    // 关闭套接字
    close(cmd_socketfd);

    return 0;
}

char ip[128][16] = { 0 }; // 组网中所有节点 ip 数组
// char ip[128][16] = { "192.168.0.180", "192.168.0.100" };
int net_id = 0; // 组网 ID
char local_ip[16] = { 0 }; // 本机 IP 地址
int node_num = 0; // 组网中节点数量
// int node_num = 2;
unsigned long long* detect_time = NULL; // 探测时间
unsigned long long time_gap = 0; // 时间差
char tmp_ip[128][16] = { 0 }; // 临时 ip 数组
char missing_ip[128][16] = { 0 }; // 缺失的 ip 地址
char min_ip[16] = { 0 }; // 缺失的最小 ip 地址
unsigned char request_mode_flag; // 请求模式标志(1请求模式 0非请求模式)
int stop_flag = 0; // 程序退出标志(1退出 0不退出)

int cmd_handler(struct sockaddr_in* addr, const char* cmd)
{
    if (!strcmp(cmd, CMD_GET_IP)) { // 获取节点 IP
        char ip_buf[16] = { 0 };
        if (send_cmd_unicast(addr, local_ip) < 0) { // 单播发送本机 IP 地址
            PTRERR("send_cmd_unicast error");
            return -1;
        }
    } else if (!strncmp(cmd, CMD_SET_NETWORK_ID(0, 0), 13)) { // 选择设备进行组网，并分配组网ID
        // 字符串解析
        char ip_buf[16] = { 0 }; // IP 地址
        int net_id_buf = 0; // 组网 ID
        sscanf(cmd, "/setNetworkID:s,%[^,],%d;", ip_buf, &net_id_buf); // %[^,]：匹配除逗号以外的所有字符

        PTR_DEBUG("ip: %s, id: %d\n", ip_buf, net_id_buf);

        if (is_ip(ip_buf) && net_id_buf > 0) { // 判断是否为 IP 地址和组网 ID
            // 判断是否为本机 IP 地址
            if (!strcmp(ip_buf, local_ip)) { // 如果是本机 IP 地址
                if (net_id_buf != net_id) { // 如果组网 ID 不同
                    // 清空 IP 地址
                    memset(ip, 0, sizeof(ip));
                    node_num = 0;
                    // 设置组网 ID
                    net_id = net_id_buf;
                    PTR_DEBUG("set network id: %d\n", net_id);
                }
            }
            // 组网内所有节点同步组网中的所有节点 IP 地址
            if (net_id_buf == net_id) { // 组网 ID 相同，保存 IP 地址
                for (int i = 0; i < sizeof(ip) / sizeof(ip[0]); i++) {
                    if (!strcmp(ip[i], ip_buf)) {
                        break; // 已存在，跳过
                    }
                    if (!strcmp(ip[i], "")) {
                        // 不存在，保存
                        strcpy(ip[i], ip_buf);
                        node_num++;
                        PTR_DEBUG("ip[%d]: %s, node_num: %d\n", i, ip[i], node_num);
                        // 更新设备系统配置文件
                        memcpy(fps.ip, ip, sizeof(ip)); // 拷贝 IP 地址表
                        if (save_sys_config(&fps, &window) < 0) {
                            PTRERR("save_sys_config error");
                            return -1;
                        }
                        break;
                    }
                }
                // 再发送自己的 IP 地址和组网 ID，与其他节点同步
                char cmd_buf[CMD_BUF_SIZE] = { 0 };
                sprintf(cmd_buf, "/setNetworkID:s,%s,%d;", local_ip, net_id);
                send_cmd_broadcast(cmd_buf);
            }
        }
    } else if (!strcmp(cmd, CMD_DETECT_TIME_GAP)) { // 检测时间差
        if (send_cmd_unicast(addr, CMD_ACK_TIME_GAP) < 0) {
            PTRERR("send_cmd_unicast error");
            return -1;
        }
    } else if (!strcmp(cmd, CMD_ACK_TIME_GAP)) { // 应答时间差
        struct timeval tv;
        unsigned long long ack_time; // 应答时间
        gettimeofday(&tv, NULL);
        ack_time = tv.tv_sec * 1000000 + tv.tv_usec; // us
        PTR_DEBUG("recv_ack: %s, time: %llu\n", cmd, ack_time); // 打印接收的数据包
        // 查找 IP 地址对应的数组下标
        int index = -1;
        for (int i = 0; i < node_num; i++) {
            if (!strcmp(ip[i], inet_ntoa(addr->sin_addr))) {
                index = i;
                break;
            }
        }
        if (index < 0) {
            PTRERR("ip not found");
            return -1;
        }
        // 计算时间差
        unsigned long long gap = (ack_time - detect_time[index]) / 2;
        PTR_DEBUG("gap: %llu\n", gap);
        // 设置时间差
        char cmd_buf[CMD_BUF_SIZE] = { 0 };
        sprintf(cmd_buf, CMD_SET_TIME_GAP(% llu), gap);
        if (send_cmd_unicast(addr, cmd_buf) < 0) {
            PTRERR("send_cmd_unicast error");
            return -1;
        }
    } else if (!strncmp(cmd, CMD_SET_TIME_GAP(0), 13)) { // 设置时间差
        // 字符串解析
        sscanf(cmd, CMD_SET_TIME_GAP(% llu), &time_gap);
        PTR_DEBUG("gap: %llu\n", time_gap);
    } else if (!strcmp(cmd, CMD_GET_IP_LIST)) {
        // 发送 IP 地址数组
        for (int i = 0; i < node_num; i++) {
            send_cmd_unicast(addr, ip[i]);
        }
    } else if (is_ip(cmd)) {
        PTR_DEBUG("recv ip: %s\n", cmd);
        // 保存 IP 地址
        for (int i = 0; i < sizeof(tmp_ip) / sizeof(tmp_ip[0]); i++) {
            if (!strcmp(tmp_ip[i], cmd)) {
                // 已存在，跳过
                break;
            }
            if (!strcmp(tmp_ip[i], "")) {
                // 不存在，保存
                strcpy(tmp_ip[i], cmd);
                PTR_DEBUG("tmp_ip[%d]: %s\n", i, tmp_ip[i]);
                break;
            }
        }
    } else if (!strcmp(cmd, CMD_START)) { // 开始指令
        // 如果客户端，开始接收视频文件
        if (server_client_flag == 0) {
            if (recv_h264() < 0) {
                PTRERR("recv_h264 error");
                return -1;
            }
        }
        PTR_DEBUG("start\n");
    } else if (!strncmp(cmd, CMD_SET_TIME_SERVER(0), 17)) { // 设置授时服务器
        // 字符串解析
        char ip_buf[16] = { 0 }; // IP 地址
        sscanf(cmd, CMD_SET_TIME_SERVER(% s), ip_buf);

        if (is_ip(ip_buf)) { // 判断是否为 IP 地址
            for (int i = 0; i < node_num; i++) { // 判断是否在组网中
                if (!strcmp(ip[i], ip_buf)) {
                    if (!strcmp(ip_buf, local_ip)) { // 如果是本机 IP 地址
                        server_client_flag = 1; // 服务器
                    } else {
                        server_client_flag = 0; // 客户端
                    }
                    break;
                }
            }
        } else {
            PTRERR("ip error");
            return -1;
        }
    } else if (!strcmp(cmd, CMD_REQUEST_CONNECT)) { // 请求连接
        if (server_client_flag == 1 && min_ip[0] != '\0') { // 服务器且缺失的最小 IP 地址不为空
            // 回应json文件
            if (send_file(addr, SYS_CONFIG_PATH) < 0) {
                PTRERR("send_file error");
                return -1;
            }

            // 发送网络中的全部信息(IP 地址表)
            char buf[CMD_BUF_SIZE] = { 0 };
            for (int i = 0; i < node_num; i++) { // 拼接 IP 地址表
                sprintf(buf, "%s%s,", buf, ip[i]);
            }
            // 将缺失的最小 IP 地址拼接到字符串末尾
            sprintf(buf, "%s%s", buf, min_ip);

            char cmd_buf[CMD_BUF_SIZE] = { 0 };
            sprintf(cmd_buf, "/setIPList:s,%s;", buf);
            if (send_cmd_unicast(addr, cmd_buf) < 0) {
                PTRERR("send_cmd_unicast error");
                return -1;
            }
        }
    } else if (!strncmp(cmd, "/setIPList:s", 12)) { // 设置 IP 地址表
        // 字符串解析
        char buf[CMD_BUF_SIZE] = { 0 }; // IP 地址表
        char ip_buf[16] = { 0 }; // IP 地址表的最后一个 IP 地址
        sscanf(cmd, "/setIPList:s,%[^;];", buf); // %[^;]：匹配除分号以外的所有字符

        // 解析 IP 地址表
        memset(ip, 0, sizeof(ip)); // 清空 IP 地址表
        int i = 0, j = 0, k = 0;
        for (i = 0; i < sizeof(ip) / sizeof(ip[0]); i++) {
            for (j = 0; j < sizeof(ip[0]); j++) {
                if (buf[k] == ',') {
                    k++;
                    break;
                }
                ip[i][j] = buf[k++];
            }
            if (buf[k] == '\0') {
                break;
            }
        }
        // 解析出 IP 地址表的最后一个 IP 地址，作为本机 IP 地址
        memcpy(ip_buf, ip[i], sizeof(ip[i]));
        memset(ip[i], 0, sizeof(ip[i])); // 清空 IP 地址表的最后一个 IP 地址
        // 更新节点数量
        node_num = i;
        // 设置更新本机 IP 地址
        strcpy(local_ip, ip_buf);
        char cmd_buf[CMD_BUF_SIZE] = { 0 };
#ifdef ARM
        sprintf(cmd_buf, "ifconfig eth0 %s netmask 255.255.255.0 up", local_ip);
#elif X86
        // wsy: 需要 root 权限
        sprintf(cmd_buf, "ifconfig ens33 %s netmask 255.255.255.0 up", local_ip);
#endif
        // 重新设置 IP 地址
        if (system(cmd_buf) < 0) { // 设置本机 IP 地址
            PTRERR("system ifconfig error");
            return -1;
        }
        // 更新设备系统配置文件
        memcpy(fps.ip, ip, sizeof(ip)); // 拷贝 IP 地址表
        if (save_sys_config(&fps, &window) < 0) {
            PTRERR("save_sys_config error");
            return -1;
        }
    }

    return 0;
}

int detect_time_gap()
{
    if (node_num < 2) {
        return 1; // 组网中节点数量小于 2，无法进行时间校正，等待其他节点加入
    }

    // 向组网中的除自己以外的所有节点发送探测时间差指令
    int cmd_socketfd; // 指令套接字
    struct sockaddr_in cmd_addr[node_num]; // 指令地址

    // 创建套接字
    cmd_socketfd = socket(AF_INET, SOCK_DGRAM, 0); // UDP
    if (cmd_socketfd < 0) {
        PTR_PERROR("socket cmd error");
        return -1;
    }

    // 设置地址
    for (int i = 0; i < node_num; i++) {
        memset(&cmd_addr[i], 0, sizeof(cmd_addr[i]));
        /* if (!strcmp(ip[i], local_ip)) {
            continue; // 跳过自己
        } */
        cmd_addr[i].sin_family = AF_INET;
        cmd_addr[i].sin_port = htons(atoi(CMD_PORT));
        cmd_addr[i].sin_addr.s_addr = inet_addr(ip[i]);
    }

    // 发送指令
    struct timeval tv;
    detect_time = (unsigned long long*)malloc(sizeof(unsigned long long) * node_num); // 探测时间
    if (detect_time == NULL) {
        PTR_PERROR("malloc error");
        return -1;
    }

    for (int i = 0; i < node_num; i++) {
        gettimeofday(&tv, NULL);
        detect_time[i] = tv.tv_sec * 1000000 + tv.tv_usec; // us
        if (sendto(cmd_socketfd, CMD_DETECT_TIME_GAP, strlen(CMD_DETECT_TIME_GAP), 0, (struct sockaddr*)&cmd_addr[i], sizeof(cmd_addr[i])) < 0) {
            PTR_PERROR("sendto cmd error");
            return -1;
        }
        PTR_DEBUG("send_detect: %s, time: %llu\n", CMD_DETECT_TIME_GAP, detect_time[i]); // 打印发送的数据包
    }

    sleep(1); // 等待 1s

    free(detect_time);
    detect_time = NULL;
    close(cmd_socketfd);

    return 0;
}

int confirm_ready()
{
    struct sockaddr_in cmd_addr; // 指令地址
    cmd_addr.sin_family = AF_INET;
    cmd_addr.sin_port = htons(atoi(CMD_PORT));
    int i, j, k, flag = 0;
    flag = 1; // 标志是否所有节点都已就绪

    for (i = 0; i < node_num; i++) {
        cmd_addr.sin_addr.s_addr = inet_addr(ip[i]);
        memset(tmp_ip, 0, sizeof(tmp_ip)); // 先清空临时 ip 数组
        if (send_cmd_unicast(&cmd_addr, CMD_GET_IP_LIST) < 0) {
            PTRERR("send_cmd_unicast error");
            return -1;
        }
        sleep(1); // 等待 1s，完成接收
        // 记录所有缺失的 IP(判断tmp_ip是否为空)
        if (!strcmp(tmp_ip[0], "")) {
            strcpy(missing_ip[i], ip[i]);
            PTR_DEBUG("missing_ip[%d]: %s\n", i, missing_ip[i]);
            flag = 0;
            continue; // 跳过当前节点
        }

        // 比对 IP 表
        for (j = 0; j < node_num; j++) {
            for (k = 0; k < node_num; k++) {
                if (!strcmp(ip[j], tmp_ip[k])) {
                    break;
                }
                if (!strcmp(tmp_ip[k], "")) {
                    flag = 0;
                    break;
                }
            }
            if (flag == 0) {
                break;
            }
        }
    }

    if (flag == 1) { // 所有节点都已就绪
        memset(missing_ip, 0, sizeof(missing_ip)); // 清空缺失的 IP 地址
        PTR_DEBUG("all ready\n");
        return 1;
    } else if (flag == 0) { // 有节点未就绪
        PTR_DEBUG("not ready\n");
        // 获取缺失的最小 IP 地址
        for (i = 0; i < node_num; i++) {
            if (!strcmp(missing_ip[i], "")) {
                continue; // 跳过空 IP 地址
            }
            if (!strcmp(min_ip, "")) {
                strcpy(min_ip, missing_ip[i]);
                continue;
            }
            if (strcmp(min_ip, missing_ip[i]) > 0) {
                strcpy(min_ip, missing_ip[i]);
            }
        }
        return 0;
    }

    return 0; // 有节点未就绪
}