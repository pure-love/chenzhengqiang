#ifndef _CZQ_FLV_H_
#define _CZQ_FLV_H_

#include "FileIo.h"

#define FILE_HEADER_LENGTH    9

//文件头结构体9个字节
typedef struct Tag_File_Header
{
	unsigned char Signature_1;                 //always 'F' (0x46)
	unsigned char Signature_2;                 //always 'L' (0x4C)
	unsigned char Signature_3;                 //always 'V' (0x56)
	unsigned char version  ;                   //版本号 现在是0x01
	unsigned char TypeFlagsReserved_1;         //第5个字节的前5位保留，必须为0。
	unsigned char TypeFlagsAudio;              //第5个字节的第6位表示是否存在音频Tag。
	unsigned char TypeFlagsReserved_2;         //第5个字节的第7位保留，必须为0。
	unsigned char TypeFlagsVideo;              //第5个字节的第8位表示是否存在视频Tag。
	unsigned int  DataOffset;                  //第6-9个字节为UI32类型的值，表示从File Header开始到File Body开始的字节数，版本1中总为9

}File_Header;

int WriteStruct_File_Header(unsigned char * Buf , unsigned int length);
#endif