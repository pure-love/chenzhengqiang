/*
@author:internet
@modified author:chenzhengqiang
@start date:2015/9/9
*/

#ifndef _CZQ_FLV_H_
#define _CZQ_FLV_H_

#include "common.h"
typedef struct _FLV_HEADER
{
	unsigned char Signature_1;                 //always 'F' (0x46)
	unsigned char Signature_2;                 //always 'L' (0x4C)
	unsigned char Signature_3;                 //always 'V' (0x56)
	unsigned char version  ;                   //版本号 现在是0x01
	unsigned char TypeFlagsReserved_1;         //第5个字节的前5位保留，必须为0。
	unsigned char TypeFlagsAudio;              //第5个字节的第6位表示是否存在音频Tag。
	unsigned char TypeFlagsReserved_2;         //第5个字节的第7位保留，必须为0。
	unsigned char TypeFlagsVideo;              //第5个字节的第8位表示是否存在视频Tag。
	unsigned int   DataOffset;                  //第6-9个字节为UI32类型的值，表示从File Header开始到File Body开始的字节数，版本1中总为9
	
}FLV_HEADER;

//包含tag，header和tag，data
typedef struct _FLV_TAG                    
{
    unsigned char Type ;                       //音频（0x08）、视频（0x09）和script data（0x12）其它保留
    unsigned int  DataSize;                    //不包含tagheader 的长度
    unsigned int  Timestamp;                   //第5-7字节为UI24类型的值，表示该Tag的时间戳（单位为ms），第一个Tag的时间戳总是0。
    unsigned char TimestampExtended;           //第8个字节为时间戳的扩展字节，当24位数值不够时，该字节作为最高位将时间戳扩展为32位值。
    unsigned int  StreamID;                    //第9-11字节为UI24类型的值，表示stream id，总是0。
    unsigned char * Data;                      //Tag Data数据
}FLV_TAG;

int read_flv_header(unsigned char * flv_stream, unsigned int stream_length ,FLV_HEADER & flv_header );
#endif