#ifndef _CZQ_VIDEO_TAG_H_
#define _CZQ_VIDEO_TAG_H_
#include "Flv.h"
#include "Mybs.h"

#define  MAX_VIDEO_TAG_BUF_SIZE   1024 * 1024
#define  VIDEO_TAG_HEADER_LENGTH  11

extern unsigned int I_Frame_Num ; 
extern unsigned int decode_video_done;

typedef struct Tag_Video_AvcC
{
	unsigned char configurationVersion;  //8；= 0x01
	unsigned char AVCProfileIndication;  //sps即sps的第2字节,所谓的AVCProfileIndication
	unsigned char profile_compatibility; //sps即sps的第3字节,所谓的profile_compatibility
	unsigned char AVCLevelIndication;    //sps即sps的第4字节,所谓的AVCLevelIndication
	unsigned char reserved_1;            //‘111111’b;
	unsigned char lengthSizeMinusOne;    //NALUnitLength 的长度 -1 一般为0x03
	unsigned char reserved_2;            //‘111’b;
	unsigned char numOfSequenceParameterSets;  //一般都是一个
	unsigned int sequenceParameterSetLength;   //sps长度
	unsigned char * sequenceParameterSetNALUnit; //sps数据
	unsigned char numOfPictureParameterSets;   //一般都是一个
	unsigned int  pictureParameterSetLength;   //pps长度
	unsigned char * pictureParameterSetNALUnit;//pps数据
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
	unsigned int CompositionTime;              //AVC时，全0，无意义,产生tag的时间，设为0
	//if AVCPacketType == 0
	//AVCDecoderConfigurationRecord
	//else if AVCPacketType == 1
	//One or more NALUs (can be individual
	//slices per FLV packets; that is, full frames
	//are not strictly required)
	//else if AVCPacketType == 2
	//Empty
	Video_AvcC  * video_avcc;
	unsigned char * Data;                     
}Video_Tag;

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


unsigned int Create_AVCDecoderConfigurationRecord(unsigned char * buf,unsigned char * spsbuf,unsigned int spslength,unsigned char * ppsbuf,unsigned int ppslength);
NALU_t *AllocNALU(int buffersize);   //分配nal 资源
void FreeNALU(NALU_t * n);           //释放nal 资源 
int FindStartCode2 (unsigned char *Buf);         //判断nal 前缀是否为3个字节
int FindStartCode3 (unsigned char *Buf);         //判断nal 前缀是否为4个字节
int GetAnnexbNALU (NALU_t *nalu);                //填写nal 数据和头
int GetFrameType(NALU_t * n);                    //获取帧类型
int TraverseH264File();                          //遍历H264文件用于查找I帧数目
int WriteStruct_H264_Tag(unsigned char * Buf,unsigned int  Timestamp,unsigned char AACPacketType/*AAC序列头部*/,unsigned int * video_frame_type);
#endif
