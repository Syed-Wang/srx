#ifndef ParseSPS_H
#define ParseSPS_H
#ifdef __cplusplus
extern "C"
{
#endif

//typedef void (*get_video_info_callback)(int with, int height, int fps);
typedef void (*get_video_info_cb)(int with, int height, int fps);


/**
解析RTP包中SPS数据信息
@param 包含SPS的RTP包
@param dataSize RTP数据的长度
@param cb video参数回调
@return success:0，fail:<0
*/
int parse_rtp(const unsigned char *pData, unsigned int dataSize, get_video_info_cb cb);


//typedef void ( get_video_info_callback)(int with, int height, int fps);
//int parse_rtp(const unsigned char *pData, unsigned int dataSize, get_video_info_callback *cb);


#ifdef __cplusplus
}
#endif

#endif  //ParseSPS_H
