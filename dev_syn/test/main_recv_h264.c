#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define H264_PATH "./test.h264" // H264文件路径
#define H264_PORT "8113" // H264端口
#define GROUP_IP "224.100.200.1" // 组播地址
#define RET_LEN 1400 // 接收缓冲区大小

#define PTR_PERROR(msg)                                     \
    do {                                                    \
        printf("%s %s %d: ", __FILE__, __func__, __LINE__); \
        perror(msg);                                        \
    } while (0)

#define PTRERR(msg)                                         \
    do {                                                    \
        printf("%s %s %d: ", __FILE__, __func__, __LINE__); \
        printf("%s\n", msg);                                \
    } while (0)

#ifdef DEBUG // 编译时定义 -DDEBUG
#define PTR_DEBUG(format, ...)                              \
    do {                                                    \
        printf("%s %s %d: ", __FILE__, __func__, __LINE__); \
        printf(format, ##__VA_ARGS__);                      \
    } while (0)
#else
#define PTR_DEBUG(format, ...)
#endif

// RTP固定头 (RTP: Real-time Transport Protocol)
typedef struct _RTP_FIXED_HEADER {
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
} RTP_FIXED_HEADER;

// NALU头 (NALU: Network Abstraction Layer Unit)
typedef struct _NALU_HEADER {
    // byte 0
    unsigned char TYPE : 5;
    unsigned char NRI : 2;
    unsigned char F : 1;

} NALU_HEADER; /**/ /* 1 BYTES */

// FU指示器 (FU: Fragmentation Unit)
typedef struct _FU_INDICATOR {
    // byte 0
    unsigned char TYPE : 5;
    unsigned char NRI : 2;
    unsigned char F : 1;
} FU_INDICATOR; /**/ /* 1 BYTES */

// FU头 (FU: Fragmentation Unit)
typedef struct _FU_HEADER {
    // byte 0
    unsigned char TYPE : 5;
    unsigned char R : 1; // 忽略
    unsigned char E : 1; // 最后一包
    unsigned char S : 1; // 第一包
} FU_HEADER; /**/ /* 1 BYTES */

int set_bind_addr(int socketfd, struct sockaddr_in* addr, unsigned int ip, const char* port)
{
    int ret;
    // 设置地址
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(atoi(port));
    addr->sin_addr.s_addr = ip;
    // 绑定地址
    ret = bind(socketfd, (struct sockaddr*)addr, sizeof(*addr));
    if (ret < 0) {
        PTR_PERROR("bind error");
        return -1;
    }

    return ret;
}

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
    // 写入前先清空文件
    FILE* fp = fopen(H264_PATH, "wb+"); // H264文件 wb以二进制写入方式打开
    if (fp == NULL) {
        perror("fopen error");
        return -1;
    }
    NALU_HEADER* nalu_hdr = (NALU_HEADER*)(rtp_buf + 12); // NALU头

    unsigned char start_code[4] = { 0x00, 0x00, 0x00, 0x01 };

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

            // 1.3 插入起始码
            fwrite(start_code, 1, 4, fp);

            // 1.3 将负载数据写入文件
            fwrite(payload, 1, payload_len, fp);
            // 打印包信息，序号
            if (pkt_tr_seq++ == ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no))
                PTR_DEBUG("单包: pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no));
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

            if (fu_hdr->S == 1) { // 2.5 判断是否是第一包
                // 2.5.1 申请内存
                if (h264_buf == NULL) {
                    perror("malloc error");
                    break;
                }

                // 2.5.2 插入起始码
                memcpy(h264_buf, start_code, 4);
                ptr = h264_buf + 4;

                // 2.5.3 将负载数据写入h264_buf
                // memcpy(h264_buf, payload, payload_len);
                memcpy(ptr, payload, payload_len);
                ptr = h264_buf + payload_len;
                // 打印包信息，序号
                if (pkt_tr_seq++ != ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no))
                    PTR_DEBUG("第一包: pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no));
            } else if (fu_hdr->E == 1) { // 2.6 判断是否是最后一包
                // 2.6.1 将负载数据写入h264_buf
                memcpy(ptr, payload, payload_len);
                ptr += payload_len;
                // 2.6.2 将h264_buf写入文件
                fwrite(h264_buf, 1, ptr - h264_buf, fp);
                // 打印包信息，序号
                if (pkt_tr_seq++ != ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no))
                    PTR_DEBUG("最后一包: pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no));
                // 接受完毕，跳出循环
                // break;
            } else {
                // 2.7 中间包
                // 2.7.1 将负载数据写入h264_buf
                memcpy(ptr, payload, payload_len);
                ptr += payload_len;
                // 打印包信息，序号
                if (pkt_tr_seq++ != ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no))
                    PTR_DEBUG("中间包: pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no));
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

int main(int argc, const char* argv[])
{
    if (recv_h264() < 0) {
        PTRERR("recv_h264 error");
        return -1;
    }
    printf("recv_h264 success\n");

    return 0;
}