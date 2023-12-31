#include "h264.h"
#include "tool.h"
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

RK_U32 pkt_tr_seq = 0; // 传输的第几个包

MPP_RET test_ctx_init(MpiEncMultiCtxInfo* info)
{
    MpiEncTestArgs* cmd = info->cmd; // 全局命令行信息
    MpiEncTestData* p = &info->ctx; // 测试数据
    MPP_RET ret = MPP_OK; // 返回值

    // get parameter from cmd
    p->width = cmd->width;
    p->height = cmd->height;
    p->hor_stride = (cmd->hor_stride) ? (cmd->hor_stride) : (MPP_ALIGN(cmd->width, 16));
    p->ver_stride = (cmd->ver_stride) ? (cmd->ver_stride) : (MPP_ALIGN(cmd->height, 16));
    p->fmt = cmd->format;
    p->type = cmd->type;
    p->bps = cmd->bps_target;
    p->bps_min = cmd->bps_min;
    p->bps_max = cmd->bps_max;
    p->rc_mode = cmd->rc_mode;
    p->frame_num = cmd->frame_num;
    if (cmd->type == MPP_VIDEO_CodingMJPEG && p->frame_num == 0) {
        mpp_log("jpege default encode only one frame. Use -n [num] for rc case\n");
        p->frame_num = 1;
    }
    p->gop_mode = cmd->gop_mode;
    p->gop_len = cmd->gop_len;
    p->vi_len = cmd->vi_len;

    p->fps_in_flex = cmd->fps_in_flex;
    p->fps_in_den = cmd->fps_in_den;
    p->fps_in_num = cmd->fps_in_num;
    p->fps_out_flex = cmd->fps_out_flex;
    p->fps_out_den = cmd->fps_out_den;
    p->fps_out_num = cmd->fps_out_num;
    p->mdinfo_size = (MPP_VIDEO_CodingHEVC == cmd->type)
        ? (MPP_ALIGN(p->hor_stride, 32) >> 5) * (MPP_ALIGN(p->ver_stride, 32) >> 5) * 16
        : (MPP_ALIGN(p->hor_stride, 64) >> 6) * (MPP_ALIGN(p->ver_stride, 16) >> 4) * 16;

    // 如果是摄像头输入，初始化摄像头。如果是文件输入，打开文件
    if (cmd->file_input) {
        if (!strncmp(cmd->file_input, "/dev/video0", 10)) {
            mpp_log("open camera device");
            p->cam_ctx = camera_source_init(cmd->file_input, 4, p->width, p->height, p->fmt);
            mpp_log("new framecap ok");
            if (p->cam_ctx == NULL)
                mpp_err("open %s fail", cmd->file_input);
        } else {
            p->fp_input = fopen(cmd->file_input, "rb");
            if (NULL == p->fp_input) {
                mpp_err("failed to open input file %s\n", cmd->file_input);
                mpp_err("create default yuv image for test\n");
            }
        }
    }

    if (cmd->file_output) {
        p->fp_output = fopen(cmd->file_output, "w+b");
        if (NULL == p->fp_output) {
            mpp_err("failed to open output file %s\n", cmd->file_output);
            ret = MPP_ERR_OPEN_FILE;
        }
    }

    if (cmd->file_slt) {
        p->fp_verify = fopen(cmd->file_slt, "wt");
        if (!p->fp_verify)
            mpp_err("failed to open verify file %s\n", cmd->file_slt);
    }

    // update resource parameter
    switch (p->fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP:
    case MPP_FMT_YUV420P: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 3 / 2;
    } break;

    case MPP_FMT_YUV422_YUYV:
    case MPP_FMT_YUV422_YVYU:
    case MPP_FMT_YUV422_UYVY:
    case MPP_FMT_YUV422_VYUY:
    case MPP_FMT_YUV422P:
    case MPP_FMT_YUV422SP: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 2;
    } break;
    case MPP_FMT_RGB444:
    case MPP_FMT_BGR444:
    case MPP_FMT_RGB555:
    case MPP_FMT_BGR555:
    case MPP_FMT_RGB565:
    case MPP_FMT_BGR565:
    case MPP_FMT_RGB888:
    case MPP_FMT_BGR888:
    case MPP_FMT_RGB101010:
    case MPP_FMT_BGR101010:
    case MPP_FMT_ARGB8888:
    case MPP_FMT_ABGR8888:
    case MPP_FMT_BGRA8888:
    case MPP_FMT_RGBA8888: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64);
    } break;

    default: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 4;
    } break;
    }

    if (MPP_FRAME_FMT_IS_FBC(p->fmt)) {
        if ((p->fmt & MPP_FRAME_FBC_MASK) == MPP_FRAME_FBC_AFBC_V1)
            p->header_size = MPP_ALIGN(MPP_ALIGN(p->width, 16) * MPP_ALIGN(p->height, 16) / 16, SZ_4K);
        else
            p->header_size = MPP_ALIGN(p->width, 16) * MPP_ALIGN(p->height, 16) / 16;
    } else {
        p->header_size = 0;
    }

    return ret;
}

MPP_RET test_ctx_deinit(MpiEncTestData* p)
{
    if (p) {
        if (p->cam_ctx) {
            camera_source_deinit(p->cam_ctx);
            p->cam_ctx = NULL;
        }
        if (p->fp_input) {
            fclose(p->fp_input);
            p->fp_input = NULL;
        }
        if (p->fp_output) {
            fclose(p->fp_output);
            p->fp_output = NULL;
        }
        if (p->fp_verify) {
            fclose(p->fp_verify);
            p->fp_verify = NULL;
        }
    }
    return MPP_OK;
}

MPP_RET test_mpp_enc_cfg_setup(MpiEncMultiCtxInfo* info)
{
    MpiEncTestArgs* cmd = info->cmd;
    MpiEncTestData* p = &info->ctx;
    MppApi* mpi = p->mpi; // mpp api
    MppCtx ctx = p->ctx; // 编码器上下文
    MppEncCfg cfg = p->cfg; // 编码器配置
    RK_U32 quiet = cmd->quiet; // 是否静默
    MPP_RET ret;
    RK_U32 rotation;
    RK_U32 mirroring;
    RK_U32 flip;
    RK_U32 gop_mode = p->gop_mode;
    MppEncRefCfg ref = NULL;

    /* setup default parameter */
    if (p->fps_in_den == 0)
        p->fps_in_den = 1;
    if (p->fps_in_num == 0)
        p->fps_in_num = 30;
    if (p->fps_out_den == 0)
        p->fps_out_den = 1;
    if (p->fps_out_num == 0)
        p->fps_out_num = 30;

    if (!p->bps) // bps(Bits Per Second) 每秒传输的比特数，比特率
        p->bps = p->width * p->height / 8 * (p->fps_out_num / p->fps_out_den);

    mpp_enc_cfg_set_s32(cfg, "prep:width", p->width);
    mpp_enc_cfg_set_s32(cfg, "prep:height", p->height);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", p->hor_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", p->ver_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:format", p->fmt);

    mpp_enc_cfg_set_s32(cfg, "rc:mode", p->rc_mode);

    /* fix input / output frame rate */
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", p->fps_in_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", p->fps_in_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denorm", p->fps_in_den);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", p->fps_out_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", p->fps_out_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denorm", p->fps_out_den);

    /* drop frame or not when bitrate overflow */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
    mpp_enc_cfg_set_u32(cfg, "rc:drop_thd", 20); /* 20% of max bps */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_gap", 1); /* Do not continuous drop frame */

    /* setup bitrate for different rc_mode */
    mpp_enc_cfg_set_s32(cfg, "rc:bps_target", p->bps);
    switch (p->rc_mode) {
    case MPP_ENC_RC_MODE_FIXQP: {
        /* do not setup bitrate on FIXQP mode */
    } break;
    case MPP_ENC_RC_MODE_CBR: {
        /* CBR mode has narrow bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 15 / 16);
    } break;
    case MPP_ENC_RC_MODE_VBR:
    case MPP_ENC_RC_MODE_AVBR: {
        /* VBR mode has wide bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 1 / 16);
    } break;
    default: {
        /* default use CBR mode */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 15 / 16);
    } break;
    }

    /* setup qp for different codec and rc_mode */
    switch (p->type) {
    case MPP_VIDEO_CodingAVC:
    case MPP_VIDEO_CodingHEVC: {
        switch (p->rc_mode) {
        case MPP_ENC_RC_MODE_FIXQP: {
            RK_S32 fix_qp = cmd->qp_init;

            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 0);
        } break;
        case MPP_ENC_RC_MODE_CBR:
        case MPP_ENC_RC_MODE_VBR:
        case MPP_ENC_RC_MODE_AVBR: {
            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", cmd->qp_init ? cmd->qp_init : -1);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", cmd->qp_max ? cmd->qp_max : 51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", cmd->qp_min ? cmd->qp_min : 10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", cmd->qp_max_i ? cmd->qp_max_i : 51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", cmd->qp_min_i ? cmd->qp_min_i : 10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 2);
        } break;
        default: {
            mpp_err_f("unsupport encoder rc mode %d\n", p->rc_mode);
        } break;
        }
    } break;
    case MPP_VIDEO_CodingVP8: {
        /* vp8 only setup base qp range */
        mpp_enc_cfg_set_s32(cfg, "rc:qp_init", cmd->qp_init ? cmd->qp_init : 40);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max", cmd->qp_max ? cmd->qp_max : 127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min", cmd->qp_min ? cmd->qp_min : 0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", cmd->qp_max_i ? cmd->qp_max_i : 127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", cmd->qp_min_i ? cmd->qp_min_i : 0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 6);
    } break;
    case MPP_VIDEO_CodingMJPEG: {
        /* jpeg use special codec config to control qtable */
        mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", cmd->qp_init ? cmd->qp_init : 80);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_max", cmd->qp_max ? cmd->qp_max : 99);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_min", cmd->qp_min ? cmd->qp_min : 1);
    } break;
    default: {
    } break;
    }

    /* setup codec  */
    mpp_enc_cfg_set_s32(cfg, "codec:type", p->type);
    switch (p->type) {
    case MPP_VIDEO_CodingAVC: {
        RK_U32 constraint_set;

        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        mpp_enc_cfg_set_s32(cfg, "h264:profile", 100);
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        mpp_enc_cfg_set_s32(cfg, "h264:level", 40);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_en", 1);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_idc", 0);
        mpp_enc_cfg_set_s32(cfg, "h264:trans8x8", 1);

        mpp_env_get_u32("constraint_set", &constraint_set, 0);
        if (constraint_set & 0x3f0000)
            mpp_enc_cfg_set_s32(cfg, "h264:constraint_set", constraint_set);
    } break;
    case MPP_VIDEO_CodingHEVC:
    case MPP_VIDEO_CodingMJPEG:
    case MPP_VIDEO_CodingVP8: {
    } break;
    default: {
        mpp_err_f("unsupport encoder coding type %d\n", p->type);
    } break;
    }

    p->split_mode = 0;
    p->split_arg = 0;
    p->split_out = 0;

    mpp_env_get_u32("split_mode", &p->split_mode, MPP_ENC_SPLIT_NONE);
    mpp_env_get_u32("split_arg", &p->split_arg, 0);
    mpp_env_get_u32("split_out", &p->split_out, 0);

    if (p->split_mode) {
        mpp_log_q(quiet, "%p split mode %d arg %d out %d\n", ctx, p->split_mode, p->split_arg, p->split_out);
        mpp_enc_cfg_set_s32(cfg, "split:mode", p->split_mode);
        mpp_enc_cfg_set_s32(cfg, "split:arg", p->split_arg);
        mpp_enc_cfg_set_s32(cfg, "split:out", p->split_out);
    }

    mpp_env_get_u32("mirroring", &mirroring, 0);
    mpp_env_get_u32("rotation", &rotation, 0);
    mpp_env_get_u32("flip", &flip, 0);

    mpp_enc_cfg_set_s32(cfg, "prep:mirroring", mirroring);
    mpp_enc_cfg_set_s32(cfg, "prep:rotation", rotation);
    mpp_enc_cfg_set_s32(cfg, "prep:flip", flip);

    // config gop_len and ref cfg
    mpp_enc_cfg_set_s32(cfg, "rc:gop", p->gop_len ? p->gop_len : p->fps_out_num * 2);

    mpp_env_get_u32("gop_mode", &gop_mode, gop_mode);

    if (gop_mode) {
        mpp_enc_ref_cfg_init(&ref);

        if (p->gop_mode < 4)
            mpi_enc_gen_ref_cfg(ref, gop_mode);
        else
            mpi_enc_gen_smart_gop_ref_cfg(ref, p->gop_len, p->vi_len);

        mpp_enc_cfg_set_ptr(cfg, "rc:ref_cfg", ref);
    }

    ret = mpi->control(ctx, MPP_ENC_SET_CFG, cfg);
    if (ret) {
        mpp_err("mpi control enc set cfg failed ret %d\n", ret);
        goto RET;
    }

    if (ref)
        mpp_enc_ref_cfg_deinit(&ref);

    /* optional */
    {
        RK_U32 sei_mode;

        mpp_env_get_u32("sei_mode", &sei_mode, MPP_ENC_SEI_MODE_ONE_FRAME);
        p->sei_mode = sei_mode;
        ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &p->sei_mode);
        if (ret) {
            mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
            goto RET;
        }
    }

    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        p->header_mode = MPP_ENC_HEADER_MODE_EACH_IDR;
        ret = mpi->control(ctx, MPP_ENC_SET_HEADER_MODE, &p->header_mode);
        if (ret) {
            mpp_err("mpi control enc set header mode failed ret %d\n", ret);
            goto RET;
        }
    }

    /* setup test mode by env */
    mpp_env_get_u32("osd_enable", &p->osd_enable, 0);
    mpp_env_get_u32("osd_mode", &p->osd_mode, MPP_ENC_OSD_PLT_TYPE_DEFAULT);
    mpp_env_get_u32("roi_enable", &p->roi_enable, 0);
    mpp_env_get_u32("user_data_enable", &p->user_data_enable, 0);

    if (p->roi_enable) {
        mpp_enc_roi_init(&p->roi_ctx, p->width, p->height, p->type, 4);
        mpp_assert(p->roi_ctx);
    }

RET:
    return ret;
}

MPP_RET send_data_to_udp(char* ptr, size_t len)
{
    // 多播组发送端
    // 1. 创建套接字
    int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketfd == -1) {
        perror("socket error");
        exit(1);
    }

    // 设置允许发送广播数据
    int on = 1; // 允许发送广播数据
    int ret = setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    if (ret == -1) {
        perror("setsockopt error");
        exit(1);
    }

    // 2. 设置组播属性
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(H264_PORT));
    addr.sin_addr.s_addr = inet_addr(GROUP_IP);

    // 3. 发送数据
    ssize_t send_len = sendto(socketfd, ptr, len, 0, (struct sockaddr*)&addr, sizeof(addr));
    if (send_len == -1) {
        perror("sendto error");
        exit(1);
    }
    printf("send_len=%ld\n", send_len);
    // 4. 关闭套接字
    close(socketfd);
    return MPP_OK;
}

MPP_RET test_mpp_run(MpiEncMultiCtxInfo* info)
{
    MpiEncTestArgs* cmd = info->cmd;
    MpiEncTestData* p = &info->ctx;
    MppApi* mpi = p->mpi;
    MppCtx ctx = p->ctx;
    RK_U32 quiet = cmd->quiet;
    RK_S32 chn = info->chn;
    RK_U32 cap_num = 0;
    DataCrc checkcrc;
    MPP_RET ret = MPP_OK;

    memset(&checkcrc, 0, sizeof(checkcrc));
    checkcrc.sum = mpp_malloc(RK_ULONG, 512);

    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        MppPacket packet = NULL;

        /*
         * Can use packet with normal malloc buffer as input not pkt_buf.
         * Please refer to vpu_api_legacy.cpp for normal buffer case.
         * Using pkt_buf buffer here is just for simplifing demo.
         */
        mpp_packet_init_with_buffer(&packet, p->pkt_buf);
        /* NOTE: It is important to clear output packet length!! */
        mpp_packet_set_length(packet, 0);

        ret = mpi->control(ctx, MPP_ENC_GET_HDR_SYNC, packet);
        if (ret) {
            mpp_err("mpi control enc get extra info failed\n");
            goto RET;
        } else {
            /* get and write sps/pps for H.264 */

            void* ptr = mpp_packet_get_pos(packet);
            size_t len = mpp_packet_get_length(packet);

            if (p->fp_output) {
                fwrite(ptr, 1, len, p->fp_output);
            } else { // 发送 sps pps
                if (len) {
                    // 将 ptr 通过 udp 发送
                    RK_U32 pkt_num = len / RET_LEN; // 包的数量
                    RK_U32 pkt_idx = 0; // 包的索引
                    struct timeval tv; // 时间
                    // struct timezone tz;
                    RTP_FIXED_HEADER* rtp_hdr; // rtp 头
                    NALU_HEADER* nalu_hdr; // nalu 头
                    FU_INDICATOR* fu_ind; // fu-a 头
                    FU_HEADER* fu_hdr; // fu-a 头
                    char arr[RET_LEN + sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER)];
                    rtp_hdr = (RTP_FIXED_HEADER*)arr;
                    nalu_hdr = (NALU_HEADER*)(arr + sizeof(RTP_FIXED_HEADER));
                    RK_U32 nalu_type = 0; // nalu 的类型，1：sps，2：pps，5：IDR，6：SEI，7：SPS，8：PPS
                    RK_U16 pkt_long = 0; // 包的长度

                    gettimeofday(&tv, NULL);
                    rtp_hdr->timestamp = tv.tv_sec * 1000000 + tv.tv_usec; // 时间戳(微秒)
                    rtp_hdr->payload = 96; // 负载类型号96(动态负载类型号)
                    rtp_hdr->version = 2; // 版本号，此版本固定为2
                    rtp_hdr->marker = 0; // 标志位，由具体协议规定其值( 1 为最后一个包)
                    nalu_type = ((char*)ptr)[4] & 0x1F; // nalu 头的第5个字节的后5位表示 nalu 的类型

                    // if (len < RET_LEN)  // 单包
                    rtp_hdr->marker = 1;
                    memset(arr, 0, sizeof(arr));
                    // note: 4字节的 nalu 头不发送
                    memcpy(&arr[sizeof(RTP_FIXED_HEADER) + sizeof(NALU_HEADER)], ptr + 4, len - 4);
                    // memcpy(&arr[sizeof(RTP_FIXED_HEADER) + sizeof(NALU_HEADER)], ptr, len);
                    nalu_hdr->F = 0; // 1 bit，禁止位，固定为0，发送端将此位置为0，接收端会忽略该比特位的信息
                    nalu_hdr->TYPE = nalu_type; // 5 bit
                    if (nalu_hdr->TYPE == 5 || nalu_hdr->TYPE == 7 || nalu_hdr->TYPE == 8) { // 5 IDR 7 SPS 8 PPS
                        nalu_hdr->NRI = 3; // 2 bit，重要性指示，固定为3
                    } else {
                        nalu_hdr->NRI = 0;
                    }
                    pkt_long = len + sizeof(RTP_FIXED_HEADER) + sizeof(NALU_HEADER) - 4; // 减去4字节的nalu头
                    rtp_hdr->seq_no = htons(pkt_tr_seq);
                    pkt_tr_seq++;
                    send_data_to_udp(arr, pkt_long); // 发送包的内容
                    printf("--------------单包--------------\n");
                }
            }
        }

        mpp_packet_deinit(&packet);
    }

    while (!p->pkt_eos) {
        MppMeta meta = NULL;
        MppFrame frame = NULL;
        MppPacket packet = NULL;
        void* buf = mpp_buffer_get_ptr(p->frm_buf);
        RK_S32 cam_frm_idx = -1;
        MppBuffer cam_buf = NULL;
        RK_U32 eoi = 1;

        if (p->fp_input) {
            ret = read_image(buf, p->fp_input, p->width, p->height, p->hor_stride, p->ver_stride, p->fmt);
            if (ret == MPP_NOK || feof(p->fp_input)) {
                p->frm_eos = 1;

                if (p->frame_num < 0 || p->frame_count < p->frame_num) {
                    clearerr(p->fp_input);
                    rewind(p->fp_input);
                    p->frm_eos = 0;
                    mpp_log_q(quiet, "chn %d loop times %d\n", chn, ++p->loop_times);
                    continue;
                }
                mpp_log_q(quiet, "chn %d found last frame. feof %d\n", chn, feof(p->fp_input));
            } else if (ret == MPP_ERR_VALUE)
                goto RET;
        } else {
            if (p->cam_ctx == NULL) {
                ret = fill_image(buf, p->width, p->height, p->hor_stride, p->ver_stride, p->fmt, p->frame_count);
                if (ret)
                    goto RET;
            } else {
                cam_frm_idx = camera_source_get_frame(p->cam_ctx);
                mpp_assert(cam_frm_idx >= 0);

                /* skip unstable frames */
                if (cap_num++ < 50) {
                    camera_source_put_frame(p->cam_ctx, cam_frm_idx);
                    continue;
                }

                cam_buf = camera_frame_to_buf(p->cam_ctx, cam_frm_idx);
                mpp_assert(cam_buf);
            }
        }

        ret = mpp_frame_init(&frame);
        if (ret) {
            mpp_err_f("mpp_frame_init failed\n");
            goto RET;
        }

        mpp_frame_set_width(frame, p->width);
        mpp_frame_set_height(frame, p->height);
        mpp_frame_set_hor_stride(frame, p->hor_stride);
        mpp_frame_set_ver_stride(frame, p->ver_stride);
        mpp_frame_set_fmt(frame, p->fmt);
        mpp_frame_set_eos(frame, p->frm_eos);

        if (p->fp_input && feof(p->fp_input))
            mpp_frame_set_buffer(frame, NULL);
        else if (cam_buf)
            mpp_frame_set_buffer(frame, cam_buf);
        else
            mpp_frame_set_buffer(frame, p->frm_buf);

        meta = mpp_frame_get_meta(frame);
        mpp_packet_init_with_buffer(&packet, p->pkt_buf);
        /* NOTE: It is important to clear output packet length!! */
        mpp_packet_set_length(packet, 0);
        mpp_meta_set_packet(meta, KEY_OUTPUT_PACKET, packet);
        mpp_meta_set_buffer(meta, KEY_MOTION_INFO, p->md_info);

        if (p->osd_enable || p->user_data_enable || p->roi_enable) {
            if (p->user_data_enable) {
                MppEncUserData user_data;
                char* str = "this is user data\n";

                if ((p->frame_count & 10) == 0) {
                    user_data.pdata = str;
                    user_data.len = strlen(str) + 1;
                    mpp_meta_set_ptr(meta, KEY_USER_DATA, &user_data);
                }
                static RK_U8 uuid_debug_info[16] = {
                    0x57, 0x68, 0x97, 0x80, 0xe7, 0x0c, 0x4b, 0x65,
                    0xa9, 0x06, 0xae, 0x29, 0x94, 0x11, 0xcd, 0x9a
                };

                MppEncUserDataSet data_group;
                MppEncUserDataFull datas[2];
                char* str1 = "this is user data 1\n";
                char* str2 = "this is user data 2\n";
                data_group.count = 2;
                datas[0].len = strlen(str1) + 1;
                datas[0].pdata = str1;
                datas[0].uuid = uuid_debug_info;

                datas[1].len = strlen(str2) + 1;
                datas[1].pdata = str2;
                datas[1].uuid = uuid_debug_info;

                data_group.datas = datas;

                mpp_meta_set_ptr(meta, KEY_USER_DATAS, &data_group);
            }

            if (p->osd_enable) {
                /* gen and cfg osd plt */
                mpi_enc_gen_osd_plt(&p->osd_plt, p->frame_count);

                p->osd_plt_cfg.change = MPP_ENC_OSD_PLT_CFG_CHANGE_ALL;
                p->osd_plt_cfg.type = MPP_ENC_OSD_PLT_TYPE_USERDEF;
                p->osd_plt_cfg.plt = &p->osd_plt;

                ret = mpi->control(ctx, MPP_ENC_SET_OSD_PLT_CFG, &p->osd_plt_cfg);
                if (ret) {
                    mpp_err("mpi control enc set osd plt failed ret %d\n", ret);
                    goto RET;
                }

                /* gen and cfg osd plt */
                mpi_enc_gen_osd_data(&p->osd_data, p->buf_grp, p->width, p->height, p->frame_count);
                mpp_meta_set_ptr(meta, KEY_OSD_DATA, (void*)&p->osd_data);
            }

            if (p->roi_enable) {
                RoiRegionCfg* region = &p->roi_region;

                /* calculated in pixels */
                region->x = MPP_ALIGN(p->width / 8, 16);
                region->y = MPP_ALIGN(p->height / 8, 16);
                region->w = 128;
                region->h = 256;
                region->force_intra = 0;
                region->qp_mode = 1;
                region->qp_val = 24;

                mpp_enc_roi_add_region(p->roi_ctx, region);

                region->x = MPP_ALIGN(p->width / 2, 16);
                region->y = MPP_ALIGN(p->height / 4, 16);
                region->w = 256;
                region->h = 128;
                region->force_intra = 1;
                region->qp_mode = 1;
                region->qp_val = 10;

                mpp_enc_roi_add_region(p->roi_ctx, region);

                /* send roi info by metadata */
                mpp_enc_roi_setup_meta(p->roi_ctx, meta);
            }
        }

        if (!p->first_frm)
            p->first_frm = mpp_time();
        /*
         * NOTE: in non-block mode the frame can be resent.
         * The default input timeout mode is block.
         *
         * User should release the input frame to meet the requirements of
         * resource creator must be the resource destroyer.
         */
        ret = mpi->encode_put_frame(ctx, frame);
        if (ret) {
            mpp_err("chn %d encode put frame failed\n", chn);
            mpp_frame_deinit(&frame);
            goto RET;
        }

        mpp_frame_deinit(&frame);

        do {
            ret = mpi->encode_get_packet(ctx, &packet);
            if (ret) {
                mpp_err("chn %d encode get packet failed\n", chn);
                goto RET;
            }

            mpp_assert(packet);

            if (packet) {
                // write packet to file here
                void* ptr = mpp_packet_get_pos(packet); // 获取包的起始地址
                size_t len = mpp_packet_get_length(packet);
                char log_buf[256];
                RK_S32 log_size = sizeof(log_buf) - 1;
                RK_S32 log_len = 0;

                if (!p->first_pkt)
                    p->first_pkt = mpp_time();

                p->pkt_eos = mpp_packet_get_eos(packet);
                // wsy 1
                if (p->fp_output) { // 如果有指定输出文件
                    fwrite(ptr, 1, len, p->fp_output);
                } else {
                    if (len) {
                        // 将 ptr 通过 udp 发送
                        RK_U32 pkt_num = len / RET_LEN; // 包的数量
                        RK_U32 pkt_idx = 0; // 包的索引
                        struct timeval tv; // 时间
                        // struct timezone tz;
                        RTP_FIXED_HEADER* rtp_hdr; // rtp 头
                        NALU_HEADER* nalu_hdr; // nalu 头
                        FU_INDICATOR* fu_ind; // fu-a 头
                        FU_HEADER* fu_hdr; // fu-a 头
                        char arr[RET_LEN + sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER)];
                        rtp_hdr = (RTP_FIXED_HEADER*)arr;
                        nalu_hdr = (NALU_HEADER*)(arr + sizeof(RTP_FIXED_HEADER));
                        RK_U32 nalu_type = 0; // nalu 的类型，1：sps，2：pps，5：IDR，6：SEI，7：SPS，8：PPS
                        RK_U16 pkt_long = 0; // 包的长度

                        gettimeofday(&tv, NULL);
                        rtp_hdr->timestamp = tv.tv_sec * 1000000 + tv.tv_usec; // 时间戳(微秒)
                        rtp_hdr->payload = 96; // 负载类型号96(动态负载类型号)
                        rtp_hdr->version = 2; // 版本号，此版本固定为2
                        rtp_hdr->marker = 0; // 标志位，由具体协议规定其值( 1 为最后一个包)
                        nalu_type = ((char*)ptr)[4] & 0x1F; // nalu 头的第5个字节的后5位表示 nalu 的类型

                        if (len < RET_LEN) { // 单包
                            rtp_hdr->marker = 1;
                            memset(arr, 0, sizeof(arr));
                            // note: 4字节的 nalu 头不发送
                            memcpy(&arr[sizeof(RTP_FIXED_HEADER) + sizeof(NALU_HEADER)], ptr + 4, len - 4);
                            // memcpy(&arr[sizeof(RTP_FIXED_HEADER) + sizeof(NALU_HEADER)], ptr, len);
                            nalu_hdr->F = 0; // 1 bit，禁止位，固定为0，发送端将此位置为0，接收端会忽略该比特位的信息
                            nalu_hdr->TYPE = nalu_type; // 5 bit
                            if (nalu_hdr->TYPE == 5 || nalu_hdr->TYPE == 7 || nalu_hdr->TYPE == 8) { // 5 IDR 7 SPS 8 PPS
                                nalu_hdr->NRI = 3; // 2 bit，重要性指示，固定为3
                            } else {
                                nalu_hdr->NRI = 0;
                            }
                            pkt_long = len + sizeof(RTP_FIXED_HEADER) + sizeof(NALU_HEADER) - 4; // 减去4字节的nalu头
                            rtp_hdr->seq_no = htons(pkt_tr_seq);
                            pkt_tr_seq++;
                            send_data_to_udp(arr, pkt_long); // 发送包的内容
                            printf("--------------单包--------------\n");
                        } else { // 多包
                            while (pkt_idx <= pkt_num) {
                                memset(arr, 0, sizeof(arr));
                                rtp_hdr->seq_no = htons(pkt_tr_seq);
                                if (pkt_idx == 0) { // 第一个包
                                    rtp_hdr->marker = 0;
                                    nalu_hdr->TYPE = nalu_type;
                                    if (nalu_hdr->TYPE == 5 || nalu_hdr->TYPE == 7 || nalu_hdr->TYPE == 8) {
                                        nalu_hdr->NRI = 3;
                                    } else {
                                        nalu_hdr->NRI = 0;
                                    }
                                    fu_ind = (FU_INDICATOR*)(arr + sizeof(RTP_FIXED_HEADER));
                                    fu_hdr = (FU_HEADER*)(arr + sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR));
                                    fu_ind->F = 0;
                                    fu_ind->TYPE = 28; // FU-A 分片
                                    fu_ind->NRI = nalu_hdr->NRI;
                                    fu_hdr->E = 0; // 末尾包
                                    fu_hdr->R = 0; // 保留位
                                    fu_hdr->S = 1; // 起始包
                                    fu_hdr->TYPE = nalu_type;

                                    // note: 4字节的nalu头不发送
                                    memcpy(&arr[sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER)], ptr + pkt_idx * RET_LEN + 4, RET_LEN - 4);
                                    // memcpy(&arr[sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER)], ptr + pkt_idx * RET_LEN, RET_LEN);
                                    pkt_long = RET_LEN + sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER) - 4; // 减去4字节的nalu头
                                } else if (pkt_idx == pkt_num) {
                                    rtp_hdr->marker = 1;
                                    fu_ind = (FU_INDICATOR*)(arr + sizeof(RTP_FIXED_HEADER));
                                    fu_hdr = (FU_HEADER*)(arr + sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR));
                                    fu_ind->F = 0;
                                    fu_ind->TYPE = 28; // FU-A 分片
                                    fu_ind->NRI = nalu_hdr->NRI;
                                    fu_hdr->E = 1; // 末尾包
                                    fu_hdr->R = 0; // 保留位
                                    fu_hdr->S = 0; // 起始包
                                    fu_hdr->TYPE = nalu_type;
                                    memcpy(&arr[sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER)], ptr + pkt_idx * RET_LEN, len % RET_LEN);
                                    pkt_long = len % RET_LEN + sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER);
                                } else if (pkt_idx < pkt_num && pkt_idx > 0) {
                                    rtp_hdr->marker = 0;
                                    fu_ind = (FU_INDICATOR*)(arr + sizeof(RTP_FIXED_HEADER));
                                    fu_hdr = (FU_HEADER*)(arr + sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR));
                                    fu_ind->F = 0;
                                    fu_ind->TYPE = 28; // FU-A 分片
                                    fu_ind->NRI = nalu_hdr->NRI;
                                    fu_hdr->E = 0; // 末尾包
                                    fu_hdr->R = 0; // 保留位
                                    fu_hdr->S = 0; // 起始包
                                    fu_hdr->TYPE = nalu_type;
                                    memcpy(&arr[sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER)], ptr + pkt_idx * RET_LEN, RET_LEN);
                                    pkt_long = RET_LEN + sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER);
                                } else {
                                    break;
                                }

                                pkt_idx++;
                                send_data_to_udp(arr, pkt_long); // 发送包的内容
                                pkt_tr_seq++;
                            }
                        }
                    }
                    printf("len = %d\n", len);

                    printf("---------------h264_end---------------\n");
                }

                if (p->fp_verify && !p->pkt_eos) {
                    calc_data_crc((RK_U8*)ptr, (RK_U32)len, &checkcrc);
                    mpp_log("p->frame_count=%d, len=%d\n", p->frame_count, len);
                    write_data_crc(p->fp_verify, &checkcrc);
                }

                log_len += snprintf(log_buf + log_len, log_size - log_len, "encoded frame %-4d", p->frame_count);

                /* for low delay partition encoding */
                if (mpp_packet_is_partition(packet)) {
                    eoi = mpp_packet_is_eoi(packet);

                    log_len += snprintf(log_buf + log_len, log_size - log_len, " pkt %d", p->frm_pkt_cnt);
                    p->frm_pkt_cnt = (eoi) ? (0) : (p->frm_pkt_cnt + 1);
                }

                log_len += snprintf(log_buf + log_len, log_size - log_len, " size %-7zu", len);

                if (mpp_packet_has_meta(packet)) {
                    meta = mpp_packet_get_meta(packet);
                    RK_S32 temporal_id = 0;
                    RK_S32 lt_idx = -1;
                    RK_S32 avg_qp = -1;

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_TEMPORAL_ID, &temporal_id))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                            " tid %d", temporal_id);

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_LONG_REF_IDX, &lt_idx))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                            " lt %d", lt_idx);

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_ENC_AVERAGE_QP, &avg_qp))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                            " qp %d", avg_qp);
                }

                mpp_log_q(quiet, "chn %d %s\n", chn, log_buf);

                mpp_packet_deinit(&packet);
                fps_calc_inc(cmd->fps);

                p->stream_size += len;
                p->frame_count += eoi;

                if (p->pkt_eos) {
                    mpp_log_q(quiet, "chn %d found last packet\n", chn);
                    mpp_assert(p->frm_eos);
                }
            }
        } while (!eoi);

        if (cam_frm_idx >= 0)
            camera_source_put_frame(p->cam_ctx, cam_frm_idx);

        if (p->frame_num > 0 && p->frame_count >= p->frame_num)
            break;

        if (p->loop_end)
            break;

        if (p->frm_eos && p->pkt_eos)
            break;
    }
RET:
    MPP_FREE(checkcrc.sum);

    return ret;
}

void* enc_test(void* arg)
{
    MpiEncMultiCtxInfo* info = (MpiEncMultiCtxInfo*)arg;
    MpiEncTestArgs* cmd = info->cmd;
    MpiEncTestData* p = &info->ctx;
    MpiEncMultiCtxRet* enc_ret = &info->ret;
    MppPollType timeout = MPP_POLL_BLOCK;
    RK_U32 quiet = cmd->quiet;
    MPP_RET ret = MPP_OK;
    RK_S64 t_s = 0;
    RK_S64 t_e = 0;

    mpp_log_q(quiet, "%s start\n", info->name);

    ret = test_ctx_init(info);
    if (ret) {
        mpp_err_f("test data init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_group_get_internal(&p->buf_grp, MPP_BUFFER_TYPE_DRM);
    if (ret) {
        mpp_err_f("failed to get mpp buffer group ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_get(p->buf_grp, &p->frm_buf, p->frame_size + p->header_size);
    if (ret) {
        mpp_err_f("failed to get buffer for input frame ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_get(p->buf_grp, &p->pkt_buf, p->frame_size);
    if (ret) {
        mpp_err_f("failed to get buffer for output packet ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_get(p->buf_grp, &p->md_info, p->mdinfo_size);
    if (ret) {
        mpp_err_f("failed to get buffer for motion info output packet ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    // encoder demo
    ret = mpp_create(&p->ctx, &p->mpi);
    if (ret) {
        mpp_err("mpp_create failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    mpp_log_q(quiet, "%p encoder test start w %d h %d type %d\n",
        p->ctx, p->width, p->height, p->type);

    ret = p->mpi->control(p->ctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
    if (MPP_OK != ret) {
        mpp_err("mpi control set output timeout %d ret %d\n", timeout, ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_init(p->ctx, MPP_CTX_ENC, p->type);
    if (ret) {
        mpp_err("mpp_init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_enc_cfg_init(&p->cfg);
    if (ret) {
        mpp_err_f("mpp_enc_cfg_init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = p->mpi->control(p->ctx, MPP_ENC_GET_CFG, p->cfg);
    if (ret) {
        mpp_err_f("get enc cfg failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = test_mpp_enc_cfg_setup(info);
    if (ret) {
        mpp_err_f("test mpp setup failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    t_s = mpp_time();
    ret = test_mpp_run(info);
    t_e = mpp_time();
    if (ret) {
        mpp_err_f("test mpp run failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = p->mpi->reset(p->ctx);
    if (ret) {
        mpp_err("mpi->reset failed\n");
        goto MPP_TEST_OUT;
    }

    enc_ret->elapsed_time = t_e - t_s;
    enc_ret->frame_count = p->frame_count;
    enc_ret->stream_size = p->stream_size;
    enc_ret->frame_rate = (float)p->frame_count * 1000000 / enc_ret->elapsed_time;
    enc_ret->bit_rate = (p->stream_size * 8 * (p->fps_out_num / p->fps_out_den)) / p->frame_count;
    enc_ret->delay = p->first_pkt - p->first_frm;

MPP_TEST_OUT:
    if (p->ctx) {
        mpp_destroy(p->ctx);
        p->ctx = NULL;
    }

    if (p->cfg) {
        mpp_enc_cfg_deinit(p->cfg);
        p->cfg = NULL;
    }

    if (p->frm_buf) {
        mpp_buffer_put(p->frm_buf);
        p->frm_buf = NULL;
    }

    if (p->pkt_buf) {
        mpp_buffer_put(p->pkt_buf);
        p->pkt_buf = NULL;
    }

    if (p->md_info) {
        mpp_buffer_put(p->md_info);
        p->md_info = NULL;
    }

    if (p->osd_data.buf) {
        mpp_buffer_put(p->osd_data.buf);
        p->osd_data.buf = NULL;
    }

    if (p->buf_grp) {
        mpp_buffer_group_put(p->buf_grp);
        p->buf_grp = NULL;
    }

    if (p->roi_ctx) {
        mpp_enc_roi_deinit(p->roi_ctx);
        p->roi_ctx = NULL;
    }

    test_ctx_deinit(p);

    return NULL;
}

int enc_test_multi(MpiEncTestArgs* cmd, const char* name)
{
    MpiEncMultiCtxInfo* ctxs = NULL;
    float total_rate = 0.0;
    RK_S32 ret = MPP_NOK;
    RK_S32 i = 0;

    ctxs = mpp_calloc(MpiEncMultiCtxInfo, cmd->nthreads);
    if (NULL == ctxs) {
        mpp_err("failed to alloc context for instances\n");
        return -1;
    }

    for (i = 0; i < cmd->nthreads; i++) {
        ctxs[i].cmd = cmd;
        ctxs[i].name = name;
        ctxs[i].chn = i;

        ret = pthread_create(&ctxs[i].thd, NULL, enc_test, &ctxs[i]);
        if (ret) {
            mpp_err("failed to create thread %d\n", i);
            return ret;
        }
    }

    if (cmd->frame_num < 0) {
        // wait for input then quit encoding
        mpp_log("*******************************************\n");
        mpp_log("**** Press Enter to stop loop encoding ****\n");
        mpp_log("*******************************************\n");

        getc(stdin);
        for (i = 0; i < cmd->nthreads; i++)
            ctxs[i].ctx.loop_end = 1;
    }

    for (i = 0; i < cmd->nthreads; i++)
        pthread_join(ctxs[i].thd, NULL);

    for (i = 0; i < cmd->nthreads; i++) {
        MpiEncMultiCtxRet* enc_ret = &ctxs[i].ret;

        mpp_log("chn %d encode %d frames time %lld ms delay %3d ms fps %3.2f bps %lld\n",
            i, enc_ret->frame_count, (RK_S64)(enc_ret->elapsed_time / 1000),
            (RK_S32)(enc_ret->delay / 1000), enc_ret->frame_rate, enc_ret->bit_rate);

        total_rate += enc_ret->frame_rate;
    }

    MPP_FREE(ctxs);

    total_rate /= cmd->nthreads;
    mpp_log("%s average frame rate %.2f\n", name, total_rate);

    return ret;
}

int send_h264(int argc, char** argv)
{
    RK_S32 ret = MPP_NOK; // 返回值
    MpiEncTestArgs* cmd = mpi_enc_test_cmd_get(); // 获取命令行参数
    ret = mpi_enc_test_cmd_update_by_args(cmd, argc, argv); // 更新命令行参数
    if (!ret) { // 更新成功
        mpi_enc_test_cmd_show_opt(cmd); // 显示命令行参数
        ret = enc_test_multi(cmd, argv[0]); // 多路编码
    }
    mpi_enc_test_cmd_put(cmd); // 释放命令行参数

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
    unsigned char start_code[4] = { 0x00, 0x00, 0x00, 0x01 }; // NALU头的起始码

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

            // note: 写入起始码
            fwrite(start_code, 1, 4, fp);

            // 去除末尾的 0
            while (*(payload + payload_len - 1) == 0) {
                payload_len--;
            }

            // 1.3 将负载数据写入文件
            fwrite(payload, 1, payload_len, fp);
            // 打印包信息，序号
            if (pkt_tr_seq++ == ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no))
                printf("单包: pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no));
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

                // note: 插入起始码
                memcpy(h264_buf, start_code, 4);
                ptr = h264_buf + 4;

                // 2.5.2 将负载数据写入h264_buf
                memcpy(ptr, payload, payload_len);
                ptr = h264_buf + payload_len;
                // 打印包信息，序号
                if (pkt_tr_seq++ == ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no))
                    printf("第一包: pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no));
            } else if (fu_hdr->E == 1) { // 2.6 判断是否是最后一包
                // 2.6.1 将负载数据写入h264_buf
                memcpy(ptr, payload, payload_len);
                ptr += payload_len;

                // 去除末尾的0
                while (*(ptr - 1) == 0) {
                    ptr--;
                }

                // 2.6.2 将h264_buf写入文件
                fwrite(h264_buf, 1, ptr - h264_buf, fp);
                ptr = NULL; // 重置ptr
                // 打印包信息，序号
                if (pkt_tr_seq++ == ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no))
                    printf("最后一包: pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no));
            } else {
                // 2.7 中间包
                // 2.7.1 将负载数据写入h264_buf
                memcpy(ptr, payload, payload_len);
                ptr += payload_len;
                // 打印包信息，序号
                if (pkt_tr_seq++ == ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no))
                    printf("中间包: pkt_tr_seq=%d, bao=%d\n", pkt_tr_seq - 1, ntohs(((RTP_FIXED_HEADER*)rtp_buf)->seq_no));
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