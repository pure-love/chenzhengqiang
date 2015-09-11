/*
@author:internet
@modified author:chenzhengqiang
@start date:2015/9/9
@modified date:
*/

#ifndef _CZQ_SCRIPT_H_
#define _CZQ_SCRIPT_H_
#include "flv.h"

static const int ONE_SCRIPT_FRAME_SIZE = 1024 * 1024;
static const int MAX_ECMAARAY_NAME_LENGH = 100;

//包含tag，header和tag，data
typedef struct Tag_Script_Tag                           
{
	unsigned char Type ;                       //音频（0x08）、视频（0x09）和script data（0x12）其它保留
	unsigned int  DataSize;                    //不包含tagheader 的长度
	unsigned int  Timestamp;                   //第5-7字节为UI24类型的值，表示该Tag的时间戳（单位为ms），第一个Tag的时间戳总是0。
	unsigned char TimestampExtended;           //第8个字节为时间戳的扩展字节，当24位数值不够时，该字节作为最高位将时间戳扩展为32位值。
	unsigned int  StreamID;                    //第9-11字节为UI24类型的值，表示stream id，总是0。
	//Type of the ScriptDataValue.
	//The following types are defined:
	//0 = Number
	//1 = Boolean
	//2 = String
	//3 = Object
	//4 = MovieClip (reserved, not supported)
	//5 = Null
	//6 = Undefined
	//7 = Reference
	//8 = ECMA array
	//9 = Object end marker
	//10 = Strict array
	//11 = Date
	//12 = Long string
	unsigned char Type_1;
	unsigned int  StringLength;                //第2-3个字节为UI16类型值，表示字符串的长度，一般总是0x000A（“onMetaData”长度
	unsigned int  ECMAArrayLength;             //一共有多少组数据，即有多少个类似：宽，高，采样率这样的信息
	double duration;                           //文件持续时间         
	double width;							   //文件    宽度         
	double height;							   //文件    高度         
	double videodatarate;					   //视频数据速率         
	double framerate;						   //帧速率               
	double videocodecid;					   //视频编解码器id       
	double audiosamplerate;					   //音频采样率           
	double audiodatarate;					   //音频数据速率         
	double audiosamplesize;					   //音频样本大小         
	int    stereo;							   //是否是立体声         
	double audiocodecid;					   //音频编解码器id       
	double filesize;						   //文件大小             
	double lasttimetamp;					   //文件最后时间         
	double lastkeyframetimetamp;               //文件最后关键帧时间点 
	double filepositions[1000];                //每一帧数据在文件中的位置
	double times[1000];                        //时间
	unsigned char Data[ONE_SCRIPT_FRAME_SIZE];                      //信息剩下的数据，暂时没有解析
}FLV_SCRIPT_TAG ;

double char2double(unsigned char * buf,unsigned int size);
void   double2char(unsigned char * buf,double val);
int read_flv_script_tag( unsigned char * flv_script_buffer, unsigned int length, FLV_SCRIPT_TAG & script_tag );
#endif