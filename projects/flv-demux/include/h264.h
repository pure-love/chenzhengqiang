/*
@author:internet
@modified author:chenzhengqiang
@start date:2015/9/9
@modified date:
*/

#ifndef _CZQ_H264_H_
#define _CZQ_H264_H_
#include "flv.h"

static const int ONE_VIDEO_FRAME_SIZE = 1024 * 1024;
static const int MAX_SPS_FRAME_SIZE = 1024;
static const int MAX_PPS_FRAME_SIZE = 1024;

typedef struct Tag_Video_AvcC
{
	unsigned char configurationVersion;
	unsigned char AVCProfileIndication;
	unsigned char profile_compatibility;
	unsigned char AVCLevelIndication;
	unsigned char reserved_1;
	unsigned char lengthSizeMinusOne;
	unsigned char reserved_2;
	unsigned char numOfSequenceParameterSets;  //一般都是一个
	unsigned int sequenceParameterSetLength;
	unsigned char sequenceParameterSetNALUnit[MAX_SPS_FRAME_SIZE];
	unsigned char numOfPictureParameterSets;   //一般都是一个
	unsigned int  pictureParameterSetLength; 
	unsigned char pictureParameterSetNALUnit[MAX_PPS_FRAME_SIZE];
	unsigned char reserved_3;
	unsigned char chroma_format;
	unsigned char reserved_4;
	unsigned char bit_depth_luma_minus8;
	unsigned char reserved_5;
	unsigned char bit_depth_chroma_minus8;
	unsigned char numOfSequenceParameterSetExt;
	unsigned int sequenceParameterSetExtLength;
	unsigned char * sequenceParameterSetExtNALUnit;
}Video_AvcC;


//包含tag，header和tag，data
typedef struct Tag_Video_Tag                           
{
	unsigned char Type ;                       //音频（0x08）、视频（0x09）和script data（0x12）其它保留
	unsigned int  DataSize;                    //不包含tagheader 的长度
	unsigned int  Timestamp;                   //第5-7字节为UI24类型的值，表示该Tag的时间戳（单位为ms），第一个Tag的时间戳总是0。
	unsigned char TimestampExtended;           //第8个字节为时间戳的扩展字节，当24位数值不够时，该字节作为最高位将时间戳扩展为32位值。
	unsigned int  StreamID;                    //第9-11字节为UI24类型的值，表示stream id，总是0。
	//1: keyframe (for AVC, a seekable frame)
	//2: inter frame (for AVC, a nonseekable frame)
    //3: disposable inter frame (H.263 only)
    //4: generated keyframe (reserved for server use only)
	//5: video info/command frame
	unsigned char FrameType;                   //帧类型
	//1: JPEG (currently unused)
	//2: Sorenson H.263
	//3: Screen video
	//4: On2 VP6
	//5: On2 VP6 with alpha channel
	//6: Screen video version 2
	//7: AVC
	unsigned char CodecID ;                    //CodecID
	//VideoData
	//	If CodecID == 2
	//  H263VIDEOPACKET
	//  If CodecID == 3
	//	SCREENVIDEOPACKET
	//	If CodecID == 4
	//	VP6FLVVIDEOPACKET
	//	If CodecID == 5
	//	VP6FLVALPHAVIDEOPACKET
	//	If CodecID == 6
	//	SCREENV2VIDEOPACKET
	//	if CodecID == 7
	//	AVCVIDEOPACKET
	//	Video frame payload or UI8
	//	(see note following table)
	//AVC sequence header
	//1: AVC NALU
	//2: AVC end of sequence (lower level NALU sequence ender is not required or supported)
	unsigned char AVCPacketType;               //packettype
	//if AVCPacketType == 1
	//Composition time offset
	//else
	//0
	int CompositionTime;                      //AVC时，全0，无意义
	//if AVCPacketType == 0
	//AVCDecoderConfigurationRecord
	//else if AVCPacketType == 1
	//One or more NALUs (can be individual
	//slices per FLV packets; that is, full frames
	//are not strictly required)
	//else if AVCPacketType == 2
	//Empty
	Video_AvcC  video_avcc;
	unsigned char Data[ONE_VIDEO_FRAME_SIZE];                     
}FLV_H264_TAG;

int read_flv_h264_tag(unsigned char * video_tag_buffer , unsigned int length ,FLV_H264_TAG & h264_tag);
#endif
