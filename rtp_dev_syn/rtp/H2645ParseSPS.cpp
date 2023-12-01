#include "H2645ParseSPS.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>

typedef unsigned char BYTE;
typedef int INT;
typedef unsigned int UINT;

typedef struct
{
	const BYTE *data;   //sps数据
	UINT size;          //sps数据大小
	UINT index;         //当前计算位所在的位置标记
} sps_bit_stream;

static BYTE *ff_avc_find_startcode_internal(BYTE *p, BYTE *end)
{
	BYTE *a = p + 4 - ((long long)p & 3);

	for (end -= 3; p < a && p < end; p++)
	{
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	for (end -= 3; p < end; p += 4)
	{
		UINT x = *(const UINT*)p;
		//      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
		//      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
		if ((x - 0x01010101) & (~x) & 0x80808080)
		{ // generic
			if (p[1] == 0)
			{
				if (p[0] == 0 && p[2] == 1)
					return p;
				if (p[2] == 0 && p[3] == 1)
					return p + 1;
			}
			if (p[3] == 0)
			{
				if (p[2] == 0 && p[4] == 1)
					return p + 2;
				if (p[4] == 0 && p[5] == 1)
					return p + 3;
			}
		}
	}

	for (end += 3; p < end; p++)
	{
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	return end + 3;
}

static BYTE *ff_avc_find_startcode(BYTE *p, BYTE *end)
{
	BYTE *out = ff_avc_find_startcode_internal(p, end);
	if (p < out && out < end && !out[-1])
		out--;
	return out;
}




/**
移除H264的NAL防竞争码(0x03)
@param data sps数据
@param dataSize sps数据大小
*/
static void del_emulation_prevention(BYTE *data, UINT *dataSize)
{
	UINT dataSizeTemp = *dataSize;
	for (UINT i = 0, j = 0; i<(dataSizeTemp - 2); i++) 
	{
		int val = (data[i] ^ 0x0) + (data[i + 1] ^ 0x0) + (data[i + 2] ^ 0x3);    //检测是否是竞争码
		if (val == 0) 
		{
			for (j = i + 2; j<dataSizeTemp - 1; j++) 
			{    //移除竞争码
				data[j] = data[j + 1];
			}

			(*dataSize)--;      //data size 减1
		}
	}
}

static void sps_bs_init(sps_bit_stream *bs, const BYTE *data, UINT size)
{
	if (bs)
	{
		bs->data = data;
		bs->size = size;
		bs->index = 0;
	}
}

/**
是否已经到数据流最后

@param bs sps_bit_stream数据
@return 1：yes，0：no
*/
static INT eof(sps_bit_stream *bs)
{
	return (bs->index >= bs->size * 8);    //位偏移已经超出数据
}

/**
读取从起始位开始的BitCount个位所表示的值

@param bs sps_bit_stream数据
@param bitCount bit位个数(从低到高)
@return value
*/
static UINT u(sps_bit_stream *bs, BYTE bitCount)
{
	UINT val = 0;
	for (BYTE i = 0; i<bitCount; i++) 
	{
		val <<= 1;
		if (eof(bs))
		{
			val = 0;
			break;
		}
		else if (bs->data[bs->index / 8] & (0x80 >> (bs->index % 8))) 
		{     //计算index所在的位是否为1
			val |= 1;
		}
		bs->index++;  //递增当前起始位(表示该位已经被计算，在后面的计算过程中不需要再次去计算所在的起始位索引，缺点：后面每个bit位都需要去位移)
	}

	return val;
}

/**
读取无符号哥伦布编码值(UE)
#2^LeadingZeroBits - 1 + (xxx)
@param bs sps_bit_stream数据
@return value
*/
static UINT ue(sps_bit_stream *bs)
{
	UINT zeroNum = 0;
	while (u(bs, 1) == 0 && !eof(bs) && zeroNum < 32) 
	{
		zeroNum++;
	}

	return (UINT)((1 << zeroNum) - 1 + u(bs, zeroNum));
}

/**
读取有符号哥伦布编码值(SE)
#(-1)^(k+1) * Ceil(k/2)

@param bs sps_bit_stream数据
@return value
*/
static INT se(sps_bit_stream *bs)
{
	INT ueVal = (INT)ue(bs);
	double k = ueVal;

	INT seVal = (INT)ceil(k / 2);     //ceil:返回大于或者等于指定表达式的最小整数
	if (ueVal % 2 == 0) 
	{       //偶数取反，即(-1)^(k+1)
		seVal = -seVal;
	}

	return seVal;
}

/**
视频可用性信息(Video usability information)解析
@param bs sps_bit_stream数据
@param info sps解析之后的信息数据及结构体
@see E.1.1 VUI parameters syntax
*/
static void h264_vui_para_parse(sps_bit_stream *bs, sps_info_struct *info)
{
	UINT aspect_ratio_info_present_flag = u(bs, 1);
	if (aspect_ratio_info_present_flag)
	{
		UINT aspect_ratio_idc = u(bs, 8);
		if (aspect_ratio_idc == 255)
		{      //Extended_SAR
			u(bs, 16);      //sar_width
			u(bs, 16);      //sar_height
		}
	}

	UINT overscan_info_present_flag = u(bs, 1);
	if (overscan_info_present_flag) 
	{
		u(bs, 1);       //overscan_appropriate_flag
	}

	UINT video_signal_type_present_flag = u(bs, 1);
	if (video_signal_type_present_flag)
	{
		u(bs, 3);       //video_format
		u(bs, 1);       //video_full_range_flag
		UINT colour_description_present_flag = u(bs, 1);
		if (colour_description_present_flag) 
		{
			u(bs, 8);       //colour_primaries
			u(bs, 8);       //transfer_characteristics
			u(bs, 8);       //matrix_coefficients
		}
	}

	UINT chroma_loc_info_present_flag = u(bs, 1);
	if (chroma_loc_info_present_flag) 
	{
		ue(bs);     //chroma_sample_loc_type_top_field
		ue(bs);     //chroma_sample_loc_type_bottom_field
	}

	UINT timing_info_present_flag = u(bs, 1);
	if (timing_info_present_flag) 
	{
		UINT num_units_in_tick = u(bs, 32);
		UINT time_scale = u(bs, 32);
		UINT fixed_frame_rate_flag = u(bs, 1);

		info->fps = (UINT)((float)time_scale / (float)num_units_in_tick);
		//if (fixed_frame_rate_flag) 
		{
			info->fps = info->fps / 2;
		}
	}

	UINT nal_hrd_parameters_present_flag = u(bs, 1);
	if (nal_hrd_parameters_present_flag)
	{
		//hrd_parameters()  //see E.1.2 HRD parameters syntax
	}

	//后面代码需要hrd_parameters()函数接口实现才有用
	UINT vcl_hrd_parameters_present_flag = u(bs, 1);
	if (vcl_hrd_parameters_present_flag)
	{
		//hrd_parameters()
	}
	if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) 
	{
		u(bs, 1);   //low_delay_hrd_flag
	}

	u(bs, 1);       //pic_struct_present_flag
	UINT bitstream_restriction_flag = u(bs, 1);
	if (bitstream_restriction_flag) 
	{
		u(bs, 1);   //motion_vectors_over_pic_boundaries_flag
		ue(bs);     //max_bytes_per_pic_denom
		ue(bs);     //max_bits_per_mb_denom
		ue(bs);     //log2_max_mv_length_horizontal
		ue(bs);     //log2_max_mv_length_vertical
		ue(bs);     //max_num_reorder_frames
		ue(bs);     //max_dec_frame_buffering
	}
}

//See 7.3.1 NAL unit syntax
//See 7.3.2.1.1 Sequence parameter set data syntax
int h264_parse_sps(const unsigned char *data, unsigned int dataSize, sps_info_struct *info)
{
	if (!data || dataSize <= 0 || !info) 
		return 0;
	INT ret = 0;
	BYTE *dataBuf = NULL;

	if (dataSize > 3 && data[0] == 0 && data[1] == 0 && data[2] == 1)
	{
		dataSize -= 3;
		data += 3;
	}
	else if (dataSize > 4 && data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 1)
	{
		dataSize -= 4;
		data += 4;
	}

	dataBuf = (BYTE*)malloc(dataSize);
	memcpy(dataBuf, data, dataSize);        //重新拷贝一份数据，防止移除竞争码时对原数据造成影响
	del_emulation_prevention(dataBuf, &dataSize);

	sps_bit_stream bs = { 0 };
	sps_bs_init(&bs, dataBuf, dataSize);   //初始化SPS数据流结构体

	u(&bs, 1);      //forbidden_zero_bit
	u(&bs, 2);      //nal_ref_idc
	UINT nal_unit_type = u(&bs, 5);

	if (nal_unit_type == 0x7)
	{     //Nal SPS Flag
		info->profile_idc = u(&bs, 8);
		u(&bs, 1);      //constraint_set0_flag
		u(&bs, 1);      //constraint_set1_flag
		u(&bs, 1);      //constraint_set2_flag
		u(&bs, 1);      //constraint_set3_flag
		u(&bs, 1);      //constraint_set4_flag
		u(&bs, 1);      //constraint_set4_flag
		u(&bs, 2);      //reserved_zero_2bits
		info->level_idc = u(&bs, 8);

		ue(&bs);    //seq_parameter_set_id

		UINT chroma_format_idc = 1;     //摄像机出图大部分格式是4:2:0
		if (info->profile_idc == 100 || info->profile_idc == 110 || info->profile_idc == 122 ||
			info->profile_idc == 244 || info->profile_idc == 44 || info->profile_idc == 83 ||
			info->profile_idc == 86 || info->profile_idc == 118 || info->profile_idc == 128 ||
			info->profile_idc == 138 || info->profile_idc == 139 || info->profile_idc == 134 || info->profile_idc == 135) 
		{
			chroma_format_idc = ue(&bs);
			if (chroma_format_idc == 3) 
			{
				u(&bs, 1);      //separate_colour_plane_flag
			}

			ue(&bs);        //bit_depth_luma_minus8
			ue(&bs);        //bit_depth_chroma_minus8
			u(&bs, 1);      //qpprime_y_zero_transform_bypass_flag
			UINT seq_scaling_matrix_present_flag = u(&bs, 1);
			if (seq_scaling_matrix_present_flag) 
			{
				UINT seq_scaling_list_present_flag[8] = { 0 };
				for (INT i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++)
				{
					seq_scaling_list_present_flag[i] = u(&bs, 1);
					if (seq_scaling_list_present_flag[i]) 
					{
						if (i < 6) 
						{    //scaling_list(ScalingList4x4[i], 16, UseDefaultScalingMatrix4x4Flag[i])
						}
						else 
						{    //scaling_list(ScalingList8x8[i − 6], 64, UseDefaultScalingMatrix8x8Flag[i − 6] )
						}
					}
				}
			}
		}

		ue(&bs);        //log2_max_frame_num_minus4
		UINT pic_order_cnt_type = ue(&bs);
		if (pic_order_cnt_type == 0)
		{
			ue(&bs);        //log2_max_pic_order_cnt_lsb_minus4
		}
		else if (pic_order_cnt_type == 1) 
		{
			u(&bs, 1);      //delta_pic_order_always_zero_flag
			se(&bs);        //offset_for_non_ref_pic
			se(&bs);        //offset_for_top_to_bottom_field

			UINT num_ref_frames_in_pic_order_cnt_cycle = ue(&bs);
			INT *offset_for_ref_frame = (INT *)malloc((UINT)num_ref_frames_in_pic_order_cnt_cycle * sizeof(INT));
			for (INT i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
			{
				offset_for_ref_frame[i] = se(&bs);
			}
			free(offset_for_ref_frame);
		}

		ue(&bs);      //max_num_ref_frames
		u(&bs, 1);      //gaps_in_frame_num_value_allowed_flag

		UINT pic_width_in_mbs_minus1 = ue(&bs);     //第36位开始
		UINT pic_height_in_map_units_minus1 = ue(&bs);      //47
		UINT frame_mbs_only_flag = u(&bs, 1);

		info->width = (INT)(pic_width_in_mbs_minus1 + 1) * 16;
		info->height = (INT)(2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16;

		if (!frame_mbs_only_flag) 
		{
			u(&bs, 1);      //mb_adaptive_frame_field_flag
		}

		u(&bs, 1);     //direct_8x8_inference_flag
		UINT frame_cropping_flag = u(&bs, 1);
		if (frame_cropping_flag) 
		{
			UINT frame_crop_left_offset = ue(&bs);
			UINT frame_crop_right_offset = ue(&bs);
			UINT frame_crop_top_offset = ue(&bs);
			UINT frame_crop_bottom_offset = ue(&bs);

			//See 6.2 Source, decoded, and output picture formats
			INT crop_unit_x = 1;
			INT crop_unit_y = 2 - frame_mbs_only_flag;      //monochrome or 4:4:4
			if (chroma_format_idc == 1) 
			{   //4:2:0
				crop_unit_x = 2;
				crop_unit_y = 2 * (2 - frame_mbs_only_flag);
			}
			else if (chroma_format_idc == 2)
			{    //4:2:2
				crop_unit_x = 2;
				crop_unit_y = 2 - frame_mbs_only_flag;
			}

			info->width -= crop_unit_x * (frame_crop_left_offset + frame_crop_right_offset);
			info->height -= crop_unit_y * (frame_crop_top_offset + frame_crop_bottom_offset);
		}

		UINT vui_parameters_present_flag = u(&bs, 1);
		if (vui_parameters_present_flag)
		{
			h264_vui_para_parse(&bs, info);
		}

		ret = 1;
	}
	free(dataBuf);

	return ret;
}

static void h265_vui_para_parse(sps_bit_stream *bs, sps_info_struct *info)
{
	if (u(bs, 1)) // aspect_ratio_info_present_flag
	{
		if (u(bs, 8) == 255) // aspect_ratio_idc
		{      //Extended_SAR
			u(bs, 16);      //sar_width
			u(bs, 16);      //sar_height
		}
	}

	if (u(bs, 1))  //overscan_info_present_flag
	{
		u(bs, 1);       //overscan_appropriate_flag
	}

	if (u(bs, 1)) // video_signal_type_present_flag
	{
		u(bs, 3);       //video_format
		u(bs, 1);       //video_full_range_flag
		if (u(bs, 1)) // colour_description_present_flag
		{
			u(bs, 8);       //colour_primaries
			u(bs, 8);       //transfer_characteristics
			u(bs, 8);       //matrix_coefficients
		}
	}

	if (u(bs, 1)) // chroma_loc_info_present_flag
	{
		ue(bs);     //chroma_sample_loc_type_top_field
		ue(bs);     //chroma_sample_loc_type_bottom_field
	}

	/*
	* neutral_chroma_indication_flag u(1)
	* field_seq_flag                 u(1)
	* frame_field_info_present_flag  u(1)
	*/
	u(bs, 3);

	if (u(bs, 1)) 
	{        // default_display_window_flag
		ue(bs); // def_disp_win_left_offset
		ue(bs); // def_disp_win_right_offset
		ue(bs); // def_disp_win_top_offset
		ue(bs); // def_disp_win_bottom_offset
	}

	if (u(bs, 1))
	{ // vui_timing_info_present_flag
		UINT num_units_in_tick = u(bs, 32); // num_units_in_tick
		UINT time_scale = u(bs, 32); // time_scale

		info->fps = (UINT)((float)time_scale / (float)num_units_in_tick);

		if (u(bs, 1))          // poc_proportional_to_timing_flag
			ue(bs); // num_ticks_poc_diff_one_minus1

		if (u(bs, 1)) // vui_hrd_parameters_present_flag
		{
			//skip_hrd_parameters(bs, 1, max_sub_layers_minus1);
		}
	}

}


int h265_parse_sps(const unsigned char *data, unsigned int dataSize, sps_info_struct *info)
{
	if (!data || dataSize <= 0 || !info)
		return 0;
	INT ret = 0;
	BYTE *dataBuf = NULL;

	dataBuf = (BYTE*)malloc(dataSize);
	memcpy(dataBuf, data, dataSize);        //重新拷贝一份数据，防止移除竞争码时对原数据造成影响
	del_emulation_prevention(dataBuf, &dataSize);

	BYTE* end = dataBuf + dataSize;
	BYTE* r = NULL;

	r = ff_avc_find_startcode(dataBuf, end);
	while (r < end)
	{
		BYTE* pNextStartcode = NULL;
		while (!*(r++));
		pNextStartcode = ff_avc_find_startcode(r, end);
		{
			const unsigned char* pNalu = r;
			int nalSize = pNextStartcode - r;

			sps_bit_stream bs = { 0 };
			sps_bs_init(&bs, pNalu, nalSize);   //初始化SPS数据流结构体

			u(&bs, 1);      //forbidden_zero_bit
			UINT nal_unit_type = u(&bs, 6);

			if (nal_unit_type == 33)
			{     //Nal SPS Flag
				u(&bs, 9);

				u(&bs, 4);  // sps_video_parameter_set_id

				int sps_max_sub_layers_minus1 = u(&bs, 3);  // sps_max_sub_layers_minus1
				if (sps_max_sub_layers_minus1 > 6)
				{
					break;
				}

				u(&bs, 1);  // sps_temporal_id_nesting_flag
				{
					u(&bs, 2);  // profile_space
					u(&bs, 1);  // tier_flag
					info->profile_idc = u(&bs, 5);  // profile_idc
					u(&bs, 32);  // profile_compatibility_flags
					u(&bs, 48);  // constraint_indicator_flags
					info->level_idc = u(&bs, 8);  // level_idc

					int i = 0;
					BYTE sub_layer_profile_present_flag[7];
					BYTE sub_layer_level_present_flag[7];
					for (int i = 0; i < sps_max_sub_layers_minus1; i++)
					{
						sub_layer_profile_present_flag[i] = u(&bs, 1);  // sub_layer_profile_present_flag
						sub_layer_level_present_flag[i] = u(&bs, 1);  // sub_layer_level_present_flag
					}

					if (sps_max_sub_layers_minus1 > 0)
					{
						for (i = sps_max_sub_layers_minus1; i < 8; i++)
							u(&bs, 2); // reserved_zero_2bits[i]
					}

					for (i = 0; i < sps_max_sub_layers_minus1; i++)
					{
						if (sub_layer_profile_present_flag[i])
						{
							/*
							* sub_layer_profile_space[i]                     u(2)
							* sub_layer_tier_flag[i]                         u(1)
							* sub_layer_profile_idc[i]                       u(5)
							* sub_layer_profile_compatibility_flag[i][0..31] u(32)
							* sub_layer_progressive_source_flag[i]           u(1)
							* sub_layer_interlaced_source_flag[i]            u(1)
							* sub_layer_non_packed_constraint_flag[i]        u(1)
							* sub_layer_frame_only_constraint_flag[i]        u(1)
							* sub_layer_reserved_zero_44bits[i]              u(44)
							*/
							u(&bs, 32);
							u(&bs, 32);
							u(&bs, 24);
						}

						if (sub_layer_level_present_flag[i])
							u(&bs, 8);
					}
				}

				ue(&bs); // sps_seq_parameter_set_id

				if (ue(&bs) == 3)
					u(&bs, 1); // separate_colour_plane_flag

				info->width = ue(&bs); // pic_width_in_luma_samples
				info->height = ue(&bs); // pic_height_in_luma_samples

				if (u(&bs, 1)) // conformance_window_flag
				{
					ue(&bs); // conf_win_left_offset
					ue(&bs); // conf_win_right_offset
					ue(&bs); // conf_win_top_offset
					ue(&bs); // conf_win_bottom_offset
				}

				ue(&bs); // bit_depth_luma_minus8
				ue(&bs); // bit_depth_chroma_minus8
				int log2_max_pic_order_cnt_lsb_minus4 = ue(&bs); // log2_max_pic_order_cnt_lsb_minus4

				/* sps_sub_layer_ordering_info_present_flag */
				int i = u(&bs, 1) ? 0 : sps_max_sub_layers_minus1;
				for (; i <= sps_max_sub_layers_minus1; i++)
				{
					ue(&bs); // max_dec_pic_buffering_minus1
					ue(&bs); // max_num_reorder_pics
					ue(&bs); // max_latency_increase_plus1
				}

				ue(&bs); // log2_min_luma_coding_block_size_minus3
				ue(&bs); // log2_diff_max_min_luma_coding_block_size
				ue(&bs); // log2_min_transform_block_size_minus2
				ue(&bs); // log2_diff_max_min_transform_block_size
				ue(&bs); // max_transform_hierarchy_depth_inter
				ue(&bs); // max_transform_hierarchy_depth_intra

				if (u(&bs, 1) && // scaling_list_enabled_flag
					u(&bs, 1))   // sps_scaling_list_data_present_flag
				{
					int i, j, k, num_coeffs;

					for (i = 0; i < 4; i++)
						for (j = 0; j < (i == 3 ? 2 : 6); j++)
							if (!u(&bs, 1))         // scaling_list_pred_mode_flag[i][j]
								ue(&bs); // scaling_list_pred_matrix_id_delta[i][j]
							else
							{
								num_coeffs = std::min(64, 1 << (4 + (i << 1)));

								if (i > 1)
									ue(&bs); // scaling_list_dc_coef_minus8[i-2][j]

								for (k = 0; k < num_coeffs; k++)
									ue(&bs); // scaling_list_delta_coef
							}
				}

				u(&bs, 1); // amp_enabled_flag
				u(&bs, 1); // sample_adaptive_offset_enabled_flag

				if (u(&bs, 1))
				{           // pcm_enabled_flag
					u(&bs, 4); // pcm_sample_bit_depth_luma_minus1
					u(&bs, 4); // pcm_sample_bit_depth_chroma_minus1
					ue(&bs);    // log2_min_pcm_luma_coding_block_size_minus3
					ue(&bs);    // log2_diff_max_min_pcm_luma_coding_block_size
					u(&bs, 1);    // pcm_loop_filter_disabled_flag
				}

				int num_short_term_ref_pic_sets = ue(&bs);
				if (num_short_term_ref_pic_sets > 64)
					break;

				unsigned int num_delta_pocs[64] = { 0 };
				for (i = 0; i < num_short_term_ref_pic_sets; i++)
				{
					if (i && u(&bs, 1))
					{ // inter_ref_pic_set_prediction_flag
					/* this should only happen for slice headers, and this isn't one */
						if (i >= num_short_term_ref_pic_sets)
							break;

						u(&bs, 1); // delta_rps_sign
						ue(&bs); // abs_delta_rps_minus1

						num_delta_pocs[i] = 0;

						for (i = 0; i <= num_delta_pocs[i - 1]; i++)
						{
							BYTE use_delta_flag = 0;
							BYTE used_by_curr_pic_flag = u(&bs, 1);
							if (!used_by_curr_pic_flag)
								use_delta_flag = u(&bs, 1);

							if (used_by_curr_pic_flag || use_delta_flag)
								num_delta_pocs[i]++;
						}
					}
					else
					{
						unsigned int num_negative_pics = ue(&bs);
						unsigned int num_positive_pics = ue(&bs);

						if ((num_positive_pics + (long)num_negative_pics) * 2 > bs.size * 8 - bs.index)
							break;

						num_delta_pocs[i] = num_negative_pics + num_positive_pics;

						int j;
						for (j = 0; j < num_negative_pics; j++)
						{
							ue(&bs); // delta_poc_s0_minus1[rps_idx]
							u(&bs, 1); // used_by_curr_pic_s0_flag[rps_idx]
						}

						for (j = 0; j < num_positive_pics; j++)
						{
							ue(&bs); // delta_poc_s1_minus1[rps_idx]
							u(&bs, 1); // used_by_curr_pic_s1_flag[rps_idx]
						}
					}
				}

				if (u(&bs, 1))
				{   // long_term_ref_pics_present_flag
					unsigned num_long_term_ref_pics_sps = ue(&bs);
					if (num_long_term_ref_pics_sps > 31U)
						return -1;
					for (i = 0; i < num_long_term_ref_pics_sps; i++)
					{ // num_long_term_ref_pics_sps
						int len = std::min(log2_max_pic_order_cnt_lsb_minus4 + 4, 16);
						u(&bs, len); // lt_ref_pic_poc_lsb_sps[i]
						u(&bs, 1);      // used_by_curr_pic_lt_sps_flag[i]
					}
				}

				u(&bs, 1); // sps_temporal_mvp_enabled_flag
				u(&bs, 1); // strong_intra_smoothing_enabled_flag

				if (u(&bs, 1)) // vui_parameters_present_flag
				{
					h265_vui_para_parse(&bs, info);
				}

			}
		}
		r = pNextStartcode;
	}
	free(dataBuf);

	return ret;
}