#ifndef ParseSPS_H
#define ParseSPS_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
	unsigned int profile_idc;
	unsigned int level_idc;

	unsigned int width;
	unsigned int height;
	unsigned int fps;       //SPS�п��ܲ�����FPS��Ϣ
} sps_info_struct;


/**
����SPS������Ϣ
@param data SPS�������ݣ���ҪNal����Ϊ0x7���ݵĿ�ʼ(���磺67 42 00 28 ab 40 22 01 e3 cb cd c0 80 80 a9 02)
@param dataSize SPS���ݵĳ���
@param info SPS����֮�����Ϣ���ݽṹ��
@return success:1��fail:0

*/
int h264_parse_sps(const unsigned char *data, unsigned int dataSize, sps_info_struct *info);

int h265_parse_sps(const unsigned char *data, unsigned int dataSize, sps_info_struct *info);

#ifdef __cplusplus
}
#endif

#endif  //ParseSPS_H