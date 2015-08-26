#include "AudioTag.h"

unsigned int decode_audio_done = 0;

int Detach_Head_Aac(ADTS_HEADER * adtsheader)
{
	unsigned char Adts_Headr_Buf[ADTS_HEADER_LENGTH];
	unsigned int readsize = 0;
	readsize = ReadFile(pAudio_Aac_File ,Adts_Headr_Buf,ADTS_HEADER_LENGTH);
	if (readsize < 0)
	{
		printf("ReadFile : pAudio_Aac_File ERROR\n");
		return getchar();
	}
	if (readsize == 0)
	{
		return readsize;
	}
	if ((Adts_Headr_Buf[0] == 0xFF)&&((Adts_Headr_Buf[1] & 0xF0) == 0xF0))    //syncword 12个1
	{
		adtsheader->syncword = (Adts_Headr_Buf[0] << 4 )  | (Adts_Headr_Buf[1]  >> 4);
		adtsheader->id = ((unsigned int) Adts_Headr_Buf[1] & 0x08) >> 3;
		adtsheader->layer = ((unsigned int) Adts_Headr_Buf[1] & 0x06) >> 1;
		adtsheader->protection_absent = (unsigned int) Adts_Headr_Buf[1] & 0x01;
		adtsheader->profile = ((unsigned int) Adts_Headr_Buf[2] & 0xc0) >> 6;
		adtsheader->sf_index = ((unsigned int) Adts_Headr_Buf[2] & 0x3c) >> 2;
		adtsheader->private_bit = ((unsigned int) Adts_Headr_Buf[2] & 0x02) >> 1;
		adtsheader->channel_configuration = ((((unsigned int) Adts_Headr_Buf[2] & 0x01) << 2) | (((unsigned int) Adts_Headr_Buf[3] & 0xc0) >> 6));
		adtsheader->original = ((unsigned int) Adts_Headr_Buf[3] & 0x20) >> 5;
		adtsheader->home = ((unsigned int) Adts_Headr_Buf[3] & 0x10) >> 4;
		adtsheader->copyright_identification_bit = ((unsigned int) Adts_Headr_Buf[3] & 0x08) >> 3;
		adtsheader->copyright_identification_start = (unsigned int) Adts_Headr_Buf[3] & 0x04 >> 2;		
		adtsheader->aac_frame_length = (((((unsigned int) Adts_Headr_Buf[3]) & 0x03) << 11) | (((unsigned int) Adts_Headr_Buf[4] & 0xFF) << 3)| ((unsigned int) Adts_Headr_Buf[5] & 0xE0) >> 5) ;
		adtsheader->adts_buffer_fullness = (((unsigned int) Adts_Headr_Buf[5] & 0x1f) << 6 | ((unsigned int) Adts_Headr_Buf[6] & 0xfc) >> 2);
		adtsheader->no_raw_data_blocks_in_frame = ((unsigned int) Adts_Headr_Buf[6] & 0x03);
	}
	else 
	{
		printf("ADTS_HEADER : BUF ERROR\n");
		getchar();
	}
	return readsize;
}

void Create_AudioSpecificConfig(unsigned char * buf,
							   unsigned char profile/*表示使用哪个级别的AAC，如01 Low Complexity(LC)--- AACLC*/,
							   unsigned char SoundRate/*采样率*/,
							   unsigned char SoundType/*声道*/ )
{
	Audio_ASC audioasc;
	audioasc.audioObjectType = profile;               
	audioasc.samplingFrequencyIndex = SoundRate;
	audioasc.channelConfiguration = SoundType;
	audioasc.framelengthFlag = 0x00;
	audioasc.dependsOnCoreCoder = 0x00;
	audioasc.extensionFlag = 0x00;
	buf[0] = (audioasc.audioObjectType << 3)  | (audioasc.samplingFrequencyIndex >> 1);
	buf[1] = (((audioasc.samplingFrequencyIndex) & 0x01) <<  7) | 
		(audioasc.channelConfiguration << 3)  | 
		(audioasc.framelengthFlag << 2) | 
		(audioasc.dependsOnCoreCoder << 1)  | 
		(audioasc.extensionFlag);
}

int WriteStruct_Aac_Tag(unsigned char * Buf,unsigned int  Timestamp,unsigned char AACPacketType/*AAC序列头部*/)
{
	Audio_Tag audiotag;
	unsigned int readsize = 0;
	unsigned int writesize = 0;
	ADTS_HEADER  adts_header ;
	unsigned char SoundRate;                //采样率
	unsigned char SoundSize = 0x01;         //样本	//0 = snd8Bit //1 = snd16Bit
	unsigned char SoundType;                //声道
	unsigned char profile = 0x01;

	//读取ADTS头
	if (!Detach_Head_Aac(&adts_header))
	{
		decode_audio_done = 1;
		return 0;
	}
	if (adts_header.sf_index == 0x04) //采样率44100
	{
		SoundRate = 0x03;
	}
	if (adts_header.channel_configuration == 0x02)  //双声道
	{
		SoundType = 0x01;
	}

	//填写audioTag头
	audiotag.Type = 0x08;
	audiotag.DataSize = adts_header.aac_frame_length - ADTS_HEADER_LENGTH  + 2; // 这个是 Tag_header以外的两个字节
	audiotag.Timestamp = Timestamp;
	audiotag.TimestampExtended = 0x00;
	audiotag.StreamID = 0x00;
	audiotag.SoundFormat = 0x0A;   //aac
	audiotag.SoundRate = SoundRate;
	audiotag.SoundSize = SoundSize;
	audiotag.SoundType = SoundType;
	audiotag.AACPacketType = AACPacketType; 
	if (AACPacketType == 0x00) //AAC sequence header
	{
		//生成AudioSpecificConfig
		Create_AudioSpecificConfig(Buf + 13,profile,adts_header.sf_index,adts_header.channel_configuration);
		audiotag.DataSize = 0x04; 
		//将音频文件移动到开头
		if (fseek(pAudio_Aac_File, 0, 0) < 0) //成功，返回0，失败返回-1
		{
			printf("fseek : pAudio_Aac_File Error\n");
			return getchar();
		}
	}
	else //AAC raw
	{
		//将data填入bufz中
		readsize = ReadFile(pAudio_Aac_File ,Buf + 13,adts_header.aac_frame_length - ADTS_HEADER_LENGTH);
		if (readsize != adts_header.aac_frame_length - ADTS_HEADER_LENGTH)
		{
			printf("READ ADTS_DATA : BUF LENGTH ERROR\n");
			return NULL;
		}
	}
	//填写文件头buf
	Buf[0] = audiotag.Type ;
	Buf[1] = (audiotag.DataSize) >> 16;
	Buf[2] = ((audiotag.DataSize) >> 8) & 0xFF;
	Buf[3] = audiotag.DataSize & 0xFF;         
	Buf[4] = (audiotag.Timestamp) >> 16;
	Buf[5] = ((audiotag.Timestamp) >> 8) & 0xFF;
	Buf[6] = audiotag.Timestamp & 0xFF;         
	Buf[7] = audiotag.TimestampExtended;
	Buf[8] = (audiotag.StreamID) >> 16;
	Buf[9] = ((audiotag.StreamID) >> 8) & 0xFF;
	Buf[10] = audiotag.StreamID & 0xFF; 
	Buf[11] = ((audiotag.SoundFormat) << 4) | ((audiotag.SoundRate) << 2) | (audiotag.SoundSize << 1) | (audiotag.SoundType);
	Buf[12] = audiotag.AACPacketType;  

	return audiotag.DataSize + AUDIO_TAG_HEADER_LENGTH;
}

