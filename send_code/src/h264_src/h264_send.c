#include "../../inc/h264_inc/h264_send.h"

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
        ? (MPP_ALIGN(p->hor_stride, 64) >> 6) * (MPP_ALIGN(p->ver_stride, 64) >> 6) * 32
        : (MPP_ALIGN(p->hor_stride, 64) >> 6) * (MPP_ALIGN(p->ver_stride, 16) >> 4) * 8;

    // 如果是摄像头输入，初始化摄像头。如果是文件输入，打开文件
    if (cmd->file_input) {
        if (!strncmp(cmd->file_input, "/dev/video", 10)) {
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
    mpp_enc_cfg_set_s32(cfg, "rc:gop", p->gop_len ? p->gop_len : p->fps_out_num * 2);

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
            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", -1);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", 51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", 10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 2);
        } break;
        default: {
            mpp_err_f("unsupport encoder rc mode %d\n", p->rc_mode);
        } break;
        }
    } break;
    case MPP_VIDEO_CodingVP8: {
        /* vp8 only setup base qp range */
        mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 40);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max", 127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min", 0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 6);
    } break;
    case MPP_VIDEO_CodingMJPEG: {
        /* jpeg use special codec config to control qtable */
        mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", 80);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_max", 99);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_min", 1);
    } break;
    default: {
    } break;
    }

    /* setup codec  */
    mpp_enc_cfg_set_s32(cfg, "codec:type", p->type);
    switch (p->type) {
    case MPP_VIDEO_CodingAVC: {
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

    ret = mpi->control(ctx, MPP_ENC_SET_CFG, cfg);
    if (ret) {
        mpp_err("mpi control enc set cfg failed ret %d\n", ret);
        goto RET;
    }

    /* optional */
    p->sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
    ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &p->sei_mode);
    if (ret) {
        mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
        goto RET;
    }

    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        p->header_mode = MPP_ENC_HEADER_MODE_EACH_IDR;
        ret = mpi->control(ctx, MPP_ENC_SET_HEADER_MODE, &p->header_mode);
        if (ret) {
            mpp_err("mpi control enc set header mode failed ret %d\n", ret);
            goto RET;
        }
    }

    RK_U32 gop_mode = p->gop_mode;

    mpp_env_get_u32("gop_mode", &gop_mode, gop_mode);
    if (gop_mode) {
        MppEncRefCfg ref;

        mpp_enc_ref_cfg_init(&ref);

        if (p->gop_mode < 4)
            mpi_enc_gen_ref_cfg(ref, gop_mode);
        else
            mpi_enc_gen_smart_gop_ref_cfg(ref, p->gop_len, p->vi_len);

        ret = mpi->control(ctx, MPP_ENC_SET_REF_CFG, ref);
        if (ret) {
            mpp_err("mpi control enc set ref cfg failed ret %d\n", ret);
            goto RET;
        }
        mpp_enc_ref_cfg_deinit(&ref);
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

    mpp_log_q(quiet, "%p encoder test start w %d h %d type %d\n", p->ctx, p->width, p->height, p->type);

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

        mpp_log("chn %d encode %d frames time %lld ms delay %3d ms fps %3.2f bps "
                "%lld\n",
            i, enc_ret->frame_count, (RK_S64)(enc_ret->elapsed_time / 1000), (RK_S32)(enc_ret->delay / 1000),
            enc_ret->frame_rate, enc_ret->bit_rate);

        total_rate += enc_ret->frame_rate;
    }

    MPP_FREE(ctxs);

    total_rate /= cmd->nthreads;
    mpp_log("%s average frame rate %.2f\n", name, total_rate);

    return ret;
}

int h264_send(int argc, char* argv[])
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