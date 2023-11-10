#include "h264_recv.h"
#include "tool.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int recv_h264()
{
    int h264_socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (h264_socketfd < 0) {
        perror("socket h264 error");
        return -1;
    }
    struct sockaddr_in h264_addr; // H264地址

    // 设置端口复用
    int opt = 1; // 端口复用标志
    if (setsockopt(h264_socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt error");
        return -1;
    }

    if (set_bind_addr(h264_socketfd, &h264_addr, INADDR_ANY, H264_PORT) < 0) {
        printf("set_bind_addr error");
        return -1;
    }
    // 设置组播属性
    struct ip_mreqn group;
    memset(&group, 0, sizeof(group));
    group.imr_multiaddr.s_addr = inet_addr(GROUP_IP); // 组播地址
    group.imr_address.s_addr = htonl(INADDR_ANY); // 本机地址
    group.imr_ifindex = 0; // 网卡索引
    // 加入组播
    if (setsockopt(h264_socketfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group)) < 0) {
        perror("setsockopt error");
        return -1;
    }

    unsigned char rtp_buf[RET_LEN + sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER)] = { 0 }; // 存放接收的数据
    unsigned char* h264_buf = (unsigned char*)malloc(4096 * 2048 * 3); // 存放接收的H264数据
    if (h264_buf == NULL) {
        perror("malloc error");
        return -1;
    }
    unsigned int pkt_tr_seq = 0; // 传输的第几个包
    unsigned char* ptr = NULL; // 指向h264_buf的末尾
    FILE* fp = fopen("../video/test.h264", "ab+"); // H264文件 ab+以二进制追加方式打开
    if (fp == NULL) {
        perror("fopen error");
        return -1;
    }
    NALU_HEADER* nalu_hdr = (NALU_HEADER*)(rtp_buf + 12); // NALU头

    // 接收H264码流
    while (1) {
        // 接收H264数据
        memset(rtp_buf, 0, sizeof(rtp_buf));
        if (recvfrom(h264_socketfd, rtp_buf, sizeof(rtp_buf), 0, NULL, NULL) < 0) {
            perror("recvfrom h264 error");
            return -1;
        }
        // 解 RTP 包，获取负载数据(H264)
        // 分为两种情况：1. 单包 2. 分包
        // 1. 单包
        if (nalu_hdr->TYPE != 28) {
            // 1.1 获取负载数据
            unsigned char* payload = rtp_buf + 13;
            // 1.2 获取负载数据长度
            int payload_len = sizeof(rtp_buf) - 13;
            // 1.3 将负载数据写入文件
            fwrite(payload, 1, payload_len, fp);
            // 打印包信息，序号
            if (pkt_tr_seq++ == ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no))
                DEBUG_PRINT("单包：pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no));
        } else {
            // 2. 分包
            // 2.1 获取FU_INDICATOR
            FU_INDICATOR* fu_ind = (FU_INDICATOR*)(rtp_buf + 12);
            // 2.2 获取FU_HEADER
            FU_HEADER* fu_hdr = (FU_HEADER*)(rtp_buf + 13);
            // 2.3 获取负载数据
            unsigned char* payload = rtp_buf + 14;
            // 2.4 获取负载数据长度F
            int payload_len = sizeof(rtp_buf) - 14;
            // 2.5 判断是否是第一包
            if (fu_hdr->S == 1) {
                // 2.5.1 申请内存
                if (h264_buf == NULL) {
                    perror("malloc error");
                    break;
                }
                // 2.5.3 将负载数据写入h264_buf
                memcpy(h264_buf, payload, payload_len);
                ptr = h264_buf + payload_len;
                // 打印包信息，序号
                if (pkt_tr_seq++ == ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no))
                    DEBUG_PRINT("第一包：pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no));
            } else if (fu_hdr->E == 1) {
                // 2.6 判断是否是最后一包
                // 2.6.1 将负载数据写入h264_buf
                memcpy(ptr, payload, payload_len);
                ptr += payload_len;
                // 2.6.2 将h264_buf写入文件
                fwrite(h264_buf, 1, ptr - h264_buf, fp);
                // 打印包信息，序号
                if (pkt_tr_seq++ == ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no))
                    DEBUG_PRINT("最后一包：pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no));
            } else {
                // 2.7 中间包
                // 2.7.1 将负载数据写入h264_buf
                memcpy(ptr, payload, payload_len);
                ptr += payload_len;
                // 打印包信息，序号
                if (pkt_tr_seq++ == ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no))
                    DEBUG_PRINT("中间包：pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no));
            }
        }
    }

    // 关闭文件
    fclose(fp);
    // 释放内存
    free(h264_buf);
    h264_buf = NULL;
    // 关闭套接字
    close(h264_socketfd);

    return 0;
}