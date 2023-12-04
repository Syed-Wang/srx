#ifndef ParseSPS_H
#define ParseSPS_H
#ifdef __cplusplus
extern "C"
{
#endif

//typedef void (*get_video_info_callback)(int with, int height, int fps);
typedef void (*get_video_info_cb)(int with, int height, int fps);


/**
����RTP����SPS������Ϣ
@param ����SPS��RTP��
@param dataSize RTP���ݵĳ���
@param cb video�����ص�
@return success:0��fail:<0
*/
int parse_rtp(const unsigned char *pData, unsigned int dataSize, get_video_info_cb cb);


//typedef void ( get_video_info_callback)(int with, int height, int fps);
//int parse_rtp(const unsigned char *pData, unsigned int dataSize, get_video_info_callback *cb);


#ifdef __cplusplus
}
#endif

#endif  //ParseSPS_H
