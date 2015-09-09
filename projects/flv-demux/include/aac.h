/*
@author:internet
@modified author:chenzhengqiang
@start date:2015/9/9
*/


#ifndef _CZQ_AAC_H_
#define _CZQ_AAC_H_

#include "flv.h"
#define ONE_AUDIO_FRAME_SIZE 1024*100

typedef struct Tag_Audio_ASC
{
	unsigned char audioObjectType;              //编解码类型：AAC-LC = 0x02
	unsigned char samplingFrequencyIndex;       //采样率 44100 = 0x04
	unsigned char channelConfiguration;         //声道 = 2
	unsigned char framelengthFlag;              //标志位，位于表明IMDCT窗口长度 = 0
	unsigned char dependsOnCoreCoder;           //标志位，表明是否依赖于corecoder = 0
	unsigned char extensionFlag;                //选择了AAC-LC = 0

}Audio_ASC;

//包含tag，header和tag，data
typedef struct Tag_Audio_Tag                           
{
	unsigned char Type ;                       //音频（0x08）、视频（0x09）和script data（0x12）其它保留
	unsigned int  DataSize;                    //不包含tagheader 的长度
	unsigned int  Timestamp;                   //第5-7字节为UI24类型的值，表示该Tag的时间戳（单位为ms），第一个Tag的时间戳总是0。
	unsigned char TimestampExtended;           //第8个字节为时间戳的扩展字节，当24位数值不够时，该字节作为最高位将时间戳扩展为32位值。
	unsigned int  StreamID;                    //第9-11字节为UI24类型的值，表示stream id，总是0。
	//0 = Linear PCM, platform endian
	//1 = ADPCM
	//2 = MP3
	//3 = Linear PCM, little endian
	//4 = Nellymoser 16-kHz mono
	//5 = Nellymoser 8-kHz mono
	//6 = Nellymoser
	//7 = G.711 A-law logarithmic PCM
	//8 = G.711 mu-law logarithmic PCM
	//9 = reserved
	//10 = AAC
	//11 = Speex
	//14 = MP3 8-Khz
	//15 = Device-specific sound
	//Format of SoundData
	//Formats 7, 8, 14, and 15 are
	//reserved for internal use
	//AAC is supported in Flash
	//Player 9,0,115,0 and higher.
	//Speex is supported in Flash
	//Player 10 and highe
	unsigned char SoundFormat ;                //数据类型
	//0 = 5.5-kHz
	//1 = 11-kHz
	//2 = 22-kHz
	//3 = 44-kHz
	//Sampling rate For AAC: always 3
	unsigned char SoundRate ;                  //采样率
	//0 = snd8Bit
	//1 = snd16Bit
	//Size of each sample. This
	//parameter only pertains to
	//uncompressed formats.
	//Compressed formats always
	//decode to 16 bits internally.
	//0 = snd8Bit
	//1 = snd16Bit
	unsigned char SoundSize;                   //样本
	//0 = sndMono
	//1 = sndStereo
	//Mono or stereo sound
	//For Nellymoser: always 0
	//For AAC: always 1
	unsigned char SoundType;                   //声道
	//SoundData UI8[size of sound data] 
	//if SoundFormat == 10
	//AACAUDIODATA
	//else  Sound data—varies by format

	//0: AAC sequence header
	//1: AAC raw
	unsigned char AACPacketType;               //AAC序列头部
	//if AACPacketType == 0        AudioSpecificConfig
	//else if AACPacketType == 1   Raw AAC frame data
	unsigned char * Data; 
	Audio_ASC * audioasc;
}Audio_Tag;

int AllocStruct_Aac_Tag(Audio_Tag ** audiotag);
int FreeStruct_Aac_Tag(Audio_Tag * audiotag);
int ReadStruct_Aac_Tag(unsigned char * Buf , unsigned int length ,Audio_Tag * tag);
#endif
