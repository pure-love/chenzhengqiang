/*
@author:chenzhengqaing
@start date:2015/9/10
@modified date:
@desc:
*/


#include "common.h"
#include "aac.h"
int read_flv_aac_tag(unsigned char * audio_tag_buffer , unsigned int length ,FLV_AAC_TAG & aac_tag )
{
	int Aac_Tag_pos = 0;
	aac_tag.Type = audio_tag_buffer[0];
	aac_tag.DataSize = 
		audio_tag_buffer[1]  << 16 |
		audio_tag_buffer[2]  << 8  |
		audio_tag_buffer[3];
	aac_tag.Timestamp = 
		audio_tag_buffer[4]  << 16 |
		audio_tag_buffer[5]  << 8  |
		audio_tag_buffer[6];
	aac_tag.TimestampExtended = audio_tag_buffer[7];
	aac_tag.StreamID = 
		audio_tag_buffer[8]  << 16 |
		audio_tag_buffer[9]  << 8  |
		audio_tag_buffer[10];
	Aac_Tag_pos += 11;
	aac_tag.SoundFormat = 
		audio_tag_buffer[Aac_Tag_pos] >> 4;
	aac_tag.SoundRate = 
        (audio_tag_buffer[Aac_Tag_pos] >> 2) & 0x03;
	aac_tag.SoundSize = 
		(audio_tag_buffer[Aac_Tag_pos] >> 1) & 0x01;
	aac_tag.SoundType = 
		 audio_tag_buffer[Aac_Tag_pos] & 0x01;
	Aac_Tag_pos ++;
	if (aac_tag.SoundFormat == 0x0A )       //AACAUDIODATA
	{
		aac_tag.AACPacketType = audio_tag_buffer[Aac_Tag_pos];
		Aac_Tag_pos ++;
	}
	if(aac_tag.AACPacketType == 0x00)   //如果是AudioSpecificConfig
	{
		//获取编解码信息，声道，采样率等等
		aac_tag.audioasc.audioObjectType = (audio_tag_buffer[Aac_Tag_pos] >> 3);
		aac_tag.audioasc.samplingFrequencyIndex = ((audio_tag_buffer[Aac_Tag_pos] & 0x07)  << 1)  | ((audio_tag_buffer[Aac_Tag_pos + 1]) >> 7 );
		Aac_Tag_pos ++;
		aac_tag.audioasc.channelConfiguration = (audio_tag_buffer[Aac_Tag_pos] >> 3)  & 0x0F;
		//下面的根据官方文档自己扩展，这里只需要这几种
	}
	memcpy(aac_tag.Data,audio_tag_buffer + Aac_Tag_pos,length - Aac_Tag_pos );
	return length - Aac_Tag_pos;
}