#include "aac.h"

int AllocStruct_Aac_Tag(Audio_Tag ** audiotag)
{
	Audio_Tag * audiotag_t = * audiotag;
	if ((audiotag_t = (Audio_Tag *)calloc(1,sizeof(Audio_Tag))) == NULL)
	{
		printf ("Error: Allocate Meory To AllocStruct_Aac_Tag Buffer Failed ");
		return getchar();
	} 
	if ((audiotag_t->Data = (unsigned char * )calloc(ONE_AUDIO_FRAME_SIZE,sizeof(unsigned char))) == NULL)
	{
		printf ("Error: Allocate Meory To audiotag_t->Data Buffer Failed ");
		return getchar();
	}
	if ((audiotag_t->audioasc = (Audio_ASC *)calloc(1,sizeof(Audio_ASC))) == NULL)
	{
		printf ("Error: Allocate Meory To audiotag_t->audioasc  Buffer Failed ");
		return getchar();
	} 
	* audiotag = audiotag_t;
	return 1;
}

int FreeStruct_Aac_Tag(Audio_Tag * audiotag)
{
	if (audiotag)
	{
		if (audiotag->Data)
		{
			free(audiotag->Data);
			audiotag->Data = NULL;
		}
		if (audiotag->audioasc)
		{
			free(audiotag->audioasc);
			audiotag->audioasc = NULL;
		}
		free(audiotag);
		audiotag = NULL;
	}
	return 1;
}

int ReadStruct_Aac_Tag(unsigned char * Buf , unsigned int length ,Audio_Tag * tag)
{
	int Aac_Tag_pos = 0;
	tag->Type = Buf[0];
	tag->DataSize = 
		Buf[1]  << 16 |
		Buf[2]  << 8  |
		Buf[3];
	tag->Timestamp = 
		Buf[4]  << 16 |
		Buf[5]  << 8  |
		Buf[6];
	tag->TimestampExtended = Buf[7];
	tag->StreamID = 
		Buf[8]  << 16 |
		Buf[9]  << 8  |
		Buf[10];
	Aac_Tag_pos += 11;
	tag->SoundFormat = 
		Buf[Aac_Tag_pos] >> 4;
	tag->SoundRate = 
        (Buf[Aac_Tag_pos] >> 2) & 0x03;
	tag->SoundSize = 
		(Buf[Aac_Tag_pos] >> 1) & 0x01;
	tag->SoundType = 
		 Buf[Aac_Tag_pos] & 0x01;
	Aac_Tag_pos ++;
	if (tag->SoundFormat == 0x0A)       //AACAUDIODATA
	{
		tag->AACPacketType = 
			Buf[Aac_Tag_pos];
		Aac_Tag_pos ++;
	}
	if(tag->AACPacketType == 0x00)   //如果是AudioSpecificConfig
	{
		//获取编解码信息，声道，采样率等等
		tag->audioasc->audioObjectType = (Buf[Aac_Tag_pos] >> 3);
		////if ( samplingFrequencyIndex == 0xf ) 
		//{
		//	samplingFrequency; 24 uimsbf
        //}
		tag->audioasc->samplingFrequencyIndex = ((Buf[Aac_Tag_pos] & 0x07)  << 1)  | ((Buf[Aac_Tag_pos + 1]) >> 7 );
		Aac_Tag_pos ++;
		tag->audioasc->channelConfiguration = (Buf[Aac_Tag_pos] >> 3)  & 0x0F;
		//下面的根据官方文档自己扩展，这里只需要这几种

	}
	memcpy(tag->Data,Buf + Aac_Tag_pos,length - Aac_Tag_pos );
	return length - Aac_Tag_pos;
}