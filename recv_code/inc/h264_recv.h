#ifndef __H264_RECV_H__
#define __H264_RECV_H__

#define H264_PORT "8113" // H264端口
#define GROUP_IP "224.100.200.1" // 组播地址
#define RET_LEN 1400 // 接收缓冲区大小

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

int recv_h264();

#endif // __H264_RECV_H__