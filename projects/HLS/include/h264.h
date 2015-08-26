#ifndef _CZQ_VIDEO_H_
#define _CZQ_VIDEO_H_

#include "ts.h"
#include "my_bs.h"
#include <cstdio>
#define  MAX_VIDEO_TAG_BUF_SIZE   1024 * 1024
#define  VIDEO_TAG_HEADER_LENGTH  11

//H264一帧数据的结构体
typedef struct Tag_NALU_t
{
	unsigned char forbidden_bit;           //! Should always be FALSE
	unsigned char nal_reference_idc;       //! NALU_PRIORITY_xxxx
	unsigned char nal_unit_type;           //! NALU_TYPE_xxxx  
	unsigned int  startcodeprefix_len;      //! 前缀字节数
	unsigned int  len;                     //! 包含nal 头的nal 长度，从第一个00000001到下一个000000001的长度
	unsigned int  max_size;                //! 做多一个nal 的长度
	unsigned char * buf;                   //! 包含nal 头的nal 数据
	unsigned char Frametype;               //! 帧类型
	unsigned int  lost_packets;            //! 预留
} NALU_t;

//nal类型
enum nal_unit_type_e
{
	NAL_UNKNOWN     = 0,
	NAL_SLICE       = 1,
	NAL_SLICE_DPA   = 2,
	NAL_SLICE_DPB   = 3,
	NAL_SLICE_DPC   = 4,
	NAL_SLICE_IDR   = 5,    /* ref_idc != 0 */
	NAL_SEI         = 6,    /* ref_idc == 0 */
	NAL_SPS         = 7,
	NAL_PPS         = 8
	/* ref_idc == 0 for 6,9,10,11,12 */
};

//帧类型
enum Frametype_e
{
	FRAME_I  = 15,
	FRAME_P  = 16,
	FRAME_B  = 17
};


NALU_t *AllocNALU(int buffersize);   //分配nal 资源
void FreeNALU(NALU_t * n);           //释放nal 资源 
int FindStartCode2 (unsigned char *Buf);         //判断nal 前缀是否为3个字节
int FindStartCode3 (unsigned char *Buf);         //判断nal 前缀是否为4个字节
int GetAnnexbNALU (FILE *,NALU_t *nalu);                //填写nal 数据和头
int GetFrameType(NALU_t * n);                    //获取帧类型
int read_h264_frame(FILE *,unsigned char * h264_frame, unsigned int & frame_length, unsigned int & frame_type);
int h264_frame_2_pes(unsigned char *h264_frame,unsigned int frame_length,unsigned long h264_pts,TsPes & h264_pes);

#endif
