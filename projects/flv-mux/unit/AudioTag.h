#ifndef _CZQ_AUDIO_TAG_H_
#define _CZQ_AUDIO_TAG_H_
#include "Flv.h"

#define AUDIO_TAG_HEADER_LENGTH    11
#define ADTS_HEADER_LENGTH         7
#define MAX_AUDIO_TAG_BUF_SIZE     1024 * 100

extern unsigned int decode_audio_done;

//ADTS 头中相对有用的信息 采样率、声道数、帧长度
//adts头
typedef struct
{
	unsigned int syncword;  //12 bslbf 同步字The bit string ‘1111 1111 1111’，说明一个ADTS帧的开始
	unsigned int id;        //1 bslbf   MPEG 标示符, 设置为1
	unsigned int layer;     //2 uimsbf Indicates which layer is used. Set to ‘00’
	unsigned int protection_absent;  //1 bslbf  表示是否误码校验
	unsigned int profile;            //2 uimsbf  表示使用哪个级别的AAC，如01 Low Complexity(LC)--- AACLC
	unsigned int sf_index;           //4 uimsbf  表示使用的采样率下标
	unsigned int private_bit;        //1 bslbf 
	unsigned int channel_configuration;  //3 uimsbf  表示声道数
	unsigned int original;               //1 bslbf 
	unsigned int home;                   //1 bslbf 
	/*下面的为改变的参数即每一帧都不同*/
	unsigned int copyright_identification_bit;   //1 bslbf 
	unsigned int copyright_identification_start; //1 bslbf
	unsigned int aac_frame_length;               // 13 bslbf  一个ADTS帧的长度包括ADTS头和raw data block
	unsigned int adts_buffer_fullness;           //11 bslbf     0x7FF 说明是码率可变的码流
	/*no_raw_data_blocks_in_frame 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧.
	所以说number_of_raw_data_blocks_in_frame == 0 
	表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
    */
	unsigned int no_raw_data_blocks_in_frame;    //2 uimsfb
} ADTS_HEADER;


typedef struct Tag_Audio_ASC
{
	unsigned char audioObjectType;              //5;编解码类型：AAC-LC = 0x02
	unsigned char samplingFrequencyIndex;       //4;采样率 44100 = 0x04
	unsigned char channelConfiguration;         //4;声道 = 2
	unsigned char framelengthFlag;              //1;标志位，位于表明IMDCT窗口长度 = 0
	unsigned char dependsOnCoreCoder;           //1;标志位，表明是否依赖于corecoder = 0
	unsigned char extensionFlag;                //1;选择了AAC-LC = 0
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

int   Detach_Head_Aac(ADTS_HEADER * adtsheader);                                 //读取ADTS头信息
void  Create_AudioSpecificConfig(unsigned char * buf,
							   unsigned char profile/*表示使用哪个级别的AAC，如01 Low Complexity(LC)--- AACLC*/,
							   unsigned char SoundRate/*采样率*/,
							   unsigned char SoundType/*声道*/ );
int WriteStruct_Aac_Tag(unsigned char * Buf,unsigned int  Timestamp,unsigned char AACPacketType/*AAC序列头部*/);
#endif