// udp 服务器端
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "8456" // 服务器端口号
#define ADDR_IP "192.168.0.100" // 服务器地址
#define GROUP_IP "224.100.200.1" // 组播地址

#define __PACKED__
#define RET_LEN 1400

struct _RTP_FIXED_HEADER {
    /**/ /* byte 0 */
    unsigned char csrc_len : 4; /**/ /* expect 0 */
    unsigned char extension : 1; /**/ /* expect 1, see RTP_OP below */
    unsigned char padding : 1; /**/ /* expect 0 */
    unsigned char version : 2; /**/ /* expect 2 */
    /**/ /* byte 1 */
    unsigned char payload : 7; /**/ /* RTP_PAYLOAD_RTSP */
    unsigned char marker : 1; /**/ /* expect 1 */
    /**/ /* bytes 2, 3 */
    unsigned short seq_no;
    /**/ /* bytes 4-7 */
    unsigned int timestamp;
    /**/ /* bytes 8-11 */
    unsigned int ssrc; /**/ /* stream number is used here. */
} __PACKED__;

typedef struct _RTP_FIXED_HEADER RTP_FIXED_HEADER;

struct _NALU_HEADER {
    // byte 0
    unsigned char TYPE : 5;
    unsigned char NRI : 2;
    unsigned char F : 1;

} __PACKED__; /**/ /* 1 BYTES */

typedef struct _NALU_HEADER NALU_HEADER;

struct _FU_INDICATOR {
    // byte 0
    unsigned char TYPE : 5;
    unsigned char NRI : 2;
    unsigned char F : 1;

} __PACKED__; /**/ /* 1 BYTES */

typedef struct _FU_INDICATOR FU_INDICATOR;

struct _FU_HEADER {
    // byte 0
    unsigned char TYPE : 5;
    unsigned char R : 1; // 忽略
    unsigned char E : 1; // 最后一包
    unsigned char S : 1; // 第一包
} __PACKED__; /**/ /* 1 BYTES */
typedef struct _FU_HEADER FU_HEADER;

unsigned char* h264_buf = NULL; // 存放分包后的数据
unsigned int pkt_tr_seq = 0; // 传输的第几个包

int main(int argc, const char* argv[])
{
    int ret = 0;
    // 多播组接收端
    // 1. 创建套接字
    int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfd == -1) {
        perror("socket error");
        exit(1);
    }

    // 2. 绑定地址
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(PORT));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(socketfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == -1) {
        perror("bind error");
        exit(1);
    }
    // 3. 加入组播
    // 设置组播属性
    struct ip_mreqn group;
    group.imr_multiaddr.s_addr = inet_addr(GROUP_IP);
    group.imr_address.s_addr = htonl(INADDR_ANY);
    group.imr_ifindex = 0;
    ret = setsockopt(socketfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group));
    if (ret == -1) {
        perror("setsockopt error");
        exit(1);
    }

    unsigned char buf[RET_LEN + sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER)] = { 0 }; // 存放接收的数据
    // int i;
    h264_buf = (unsigned char*)malloc(4096 * 2048 * 3);
    if (h264_buf == NULL) {
        perror("malloc error");
        exit(1);
    }
    FILE* fp = fopen("test.h264", "ab+");
    if (fp == NULL) {
        perror("fopen error");
        exit(1);
    }
    unsigned char* ptr = NULL;

    // 3. 通信
    while (1) {
        printf("recvfrom...\n");
        memset(buf, 0, sizeof(buf));
        ssize_t recv_len = recvfrom(socketfd, buf, sizeof(buf), 0, NULL, NULL);
        if (recv_len == -1) {
            perror("recvfrom error");
            // exit(1);
            break;
        }
        printf("recv_len=%ld\n", recv_len);
        // 解 RTP 包，获取负载数据(H264)
        // 分为两种情况：1. 单包 2. 分包
        NALU_HEADER* nalu_hdr = (NALU_HEADER*)(buf + 12);
        // 1. 单包
        if (nalu_hdr->TYPE != 28) {
            // 1.1 获取负载数据
            unsigned char* payload = buf + 13;
            // 1.2 获取负载数据长度
            int payload_len = recv_len - 13;
            // 1.3 将负载数据写入文件
            fwrite(payload, 1, payload_len, fp);
            // 打印包信息，序号
            if (pkt_tr_seq++ == ntohs(((RTP_FIXED_HEADER*)buf)->seq_no))
                printf("单包：pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)buf)->seq_no));
        } else {
            // 2. 分包
            // 2.1 获取FU_INDICATOR
            FU_INDICATOR* fu_ind = (FU_INDICATOR*)(buf + 12);
            // 2.2 获取FU_HEADER
            FU_HEADER* fu_hdr = (FU_HEADER*)(buf + 13);
            // 2.3 获取负载数据
            unsigned char* payload = buf + 14;
            // 2.4 获取负载数据长度F
            int payload_len = recv_len - 14;
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
                if (pkt_tr_seq++ == ntohs(((RTP_FIXED_HEADER*)buf)->seq_no))
                    printf("分包：pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)buf)->seq_no));
            } else if (fu_hdr->E == 1) {
                // 2.6 判断是否是最后一包
                // 2.6.1 将负载数据写入h264_buf
                memcpy(ptr, payload, payload_len);
                ptr += payload_len;
                // 打印包信息，序号
                if (pkt_tr_seq++ == ntohs(((RTP_FIXED_HEADER*)buf)->seq_no))
                    printf("分包：pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)buf)->seq_no));
                // 2.6.2 将h264_buf写入文件
                fwrite(h264_buf, 1, ptr - h264_buf, fp);
            } else {
                // 2.7 中间包
                // 2.7.1 将负载数据写入h264_buf
                memcpy(ptr, payload, payload_len);
                ptr += payload_len;
                // 打印包信息，序号
                if (pkt_tr_seq++ == ntohs(((RTP_FIXED_HEADER*)buf)->seq_no))
                    printf("分包：pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)buf)->seq_no));
            }
        }
    }

    free(h264_buf);
    h264_buf = NULL;
    fclose(fp);
    close(socketfd);

    return 0;
}
