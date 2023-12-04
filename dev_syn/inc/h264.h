#ifndef __H264_H__
#define __H264_H__

#include "camera_source.h"
#include "mpi_enc_utils.h"
#include "mpp_common.h"
#include "mpp_enc_roi_utils.h"
#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "rk_mpi.h"
#include "utils.h"

#define H264_PATH "./test.h264" // H264文件路径
#define H264_PORT "8113" // H264端口
#define GROUP_IP "224.100.200.1" // 组播地址
#define RET_LEN 1400 // 接收缓冲区大小

// 测试多路编码参数
typedef struct
{
    // base flow context
    MppCtx ctx; // 上下文
    MppApi* mpi; // 接口
    RK_S32 chn; // 通道

    // global flow control flag
    RK_U32 frm_eos; // 帧结束标志
    RK_U32 pkt_eos; // 包结束标志
    RK_U32 frm_pkt_cnt; // 帧包计数
    RK_S32 frame_num; // 帧数
    RK_S32 frame_count; // 帧计数
    RK_U64 stream_size; // 流大小
    /* end of encoding flag when set quit the loop */
    volatile RK_U32 loop_end; // 循环结束标志

    // src and dst
    FILE* fp_input; // 输入文件
    FILE* fp_output; // 输出文件
    FILE* fp_verify; // 验证文件

    /* encoder config set */
    MppEncCfg cfg; // 编码器配置
    MppEncPrepCfg prep_cfg; // 预处理配置
    MppEncRcCfg rc_cfg; // 码率控制配置
    MppEncCodecCfg codec_cfg; // 编码器配置
    MppEncSliceSplit split_cfg; // 切片配置
    MppEncOSDPltCfg osd_plt_cfg; // OSD调色板配置
    MppEncOSDPlt osd_plt; // OSD调色板
    MppEncOSDData osd_data; // OSD数据
    RoiRegionCfg roi_region; // ROI区域
    MppEncROICfg roi_cfg; // ROI配置

    // input / output
    MppBufferGroup buf_grp; // 缓冲区组
    MppBuffer frm_buf; // 帧缓冲区
    MppBuffer pkt_buf; // 包缓冲区
    MppBuffer md_info; // MD信息
    MppEncSeiMode sei_mode; // SEI模式
    MppEncHeaderMode header_mode; // 头模式

    // parameter for resource malloc
    RK_U32 width; // 宽度
    RK_U32 height; // 高度
    RK_U32 hor_stride; // 水平步长
    RK_U32 ver_stride; // 垂直步长
    MppFrameFormat fmt; // 帧格式
    MppCodingType type; // 编码类型
    RK_S32 loop_times; // 循环次数
    CamSource* cam_ctx; // 摄像头上下文
    MppEncRoiCtx roi_ctx; // ROI上下文

    // resources
    size_t header_size; // 头大小
    size_t frame_size; // 帧大小
    size_t mdinfo_size; // MD信息大小
    size_t packet_size; // 包大小

    RK_U32 osd_enable; // OSD使能
    RK_U32 osd_mode; // OSD模式
    RK_U32 split_mode; // 切片模式
    RK_U32 split_arg; // 切片参数
    RK_U32 split_out; // 切片输出

    RK_U32 user_data_enable; // 用户数据使能
    RK_U32 roi_enable; // ROI使能

    // rate control runtime parameter
    RK_S32 fps_in_flex; // 输入帧率
    RK_S32 fps_in_den; // 输入帧率分母
    RK_S32 fps_in_num; // 输入帧率分子
    RK_S32 fps_out_flex; // 输出帧率
    RK_S32 fps_out_den; // 输出帧率分母
    RK_S32 fps_out_num; // 输出帧率分子
    RK_S32 bps; // 比特率
    RK_S32 bps_max; // 最大比特率
    RK_S32 bps_min; // 最小比特率
    RK_S32 rc_mode; // 码率控制模式
    RK_S32 gop_mode; // GOP模式
    RK_S32 gop_len; // GOP长度
    RK_S32 vi_len; // VI长度

    RK_S64 first_frm; // 第一帧
    RK_S64 first_pkt; // 第一包
} MpiEncTestData;

// 测试多路编码返回值
typedef struct
{
    float frame_rate; // 帧率
    RK_U64 bit_rate; // 比特率
    RK_S64 elapsed_time; // 耗时
    RK_S32 frame_count; // 帧计数
    RK_S64 stream_size; // 流大小
    RK_S64 delay; // 延时
} MpiEncMultiCtxRet;

// 测试多路编码信息
typedef struct
{
    MpiEncTestArgs* cmd; // 命令行参数
    const char* name; // 名称
    RK_S32 chn; // 通道
    pthread_t thd; // 线程
    MpiEncTestData ctx; // 上下文
    MpiEncMultiCtxRet ret; // 返回值
} MpiEncMultiCtxInfo;

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
} RTP_FIXED_HEADER; /**/ /* 12 BYTES */

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

/**
 * @brief 初始化测试信息
 *
 * @param info 测试信息
 * @return MPP_RET
 */
MPP_RET test_ctx_init(MpiEncMultiCtxInfo* info);

/**
 * @brief 释放测试信息
 *
 * @param p 测试信息
 * @return MPP_RET
 */
MPP_RET test_ctx_deinit(MpiEncTestData* p);

/**
 * @brief 测试多路编码配置
 *
 * @param info 测试信息
 * @return MPP_RET
 */
MPP_RET test_mpp_enc_cfg_setup(MpiEncMultiCtxInfo* info);

/**
 * @brief 测试多路编码
 *
 * @param arg 参数
 * @return void* 返回值
 */
void* enc_test(void* arg);

/**
 * @brief 测试多路编码
 *
 * @param cmd 命令行参数
 * @param name 名称
 * @return int 返回值 0-成功 -1-失败
 */
int enc_test_multi(MpiEncTestArgs* cmd, const char* name);

/**
 * @brief 发送数据到UDP
 *
 * @param ptr 数据指针
 * @param len 数据长度
 * @return MPP_RET
 */
int send_data_to_udp(char* ptr, size_t len);

/**
 * @brief 发送H264码流
 *
 * @param argc 参数个数
 * @param argv 参数列表
 * @return int 返回值 0-成功 -1-失败
 */
int send_h264(int argc, char** argv);

/**
 * @brief 接收H264码流
 *
 * @return int 返回值 0-成功 -1-失败
 */
int recv_h264();

#endif // __H264_H__