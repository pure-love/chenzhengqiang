/*
@author:internet
@improved by:chenzhenqiang
@start date:2015/9/9
*/

#include "common.h"
#include "flv_demux.h"

File_Header * m_fileheadr  = NULL;
Audio_Tag   * m_audio_tag  = NULL;
Video_Tag   * m_video_tag  = NULL;
Script_Tag  * m_script_tag = NULL;
static const int NO = 0;
int AllocStruct()
{
	//扩展下层
	AllocStruct_File_Header(&m_fileheadr);
	AllocStruct_Aac_Tag(&m_audio_tag);
	AllocStruct_H264_Tag(&m_video_tag);
	AllocStruct_Script_Tag(&m_script_tag);
	return 1;
}

int FreeStruct()
{
	//扩展下层
	FreeStruct_File_Header(m_fileheadr);
	FreeStruct_Aac_Tag(m_audio_tag);
	FreeStruct_H264_Tag(m_video_tag);
	FreeStruct_Script_Tag(m_script_tag);
	return 1;
}

int And_Head_H264( FILE *f264_handler,unsigned char * buf, unsigned int size,unsigned char *spsbuffer,unsigned int spslength,unsigned char * ppsbuffer,unsigned int  ppslength ,unsigned int IsVideo_I_Frame)
{
	int write_size = 0;
	unsigned char * buffer = NULL;
	unsigned char strcode[4];                //前缀标志
	unsigned int Sei_length_1 = 0;           //sei帧的长度
	unsigned int Sei_length_2 = 0;
	buffer = (unsigned char * )calloc(size + 1024,sizeof(char));

	//////////////////////////////////////////////////////////////////////////
	if (buf[4] == 0x06)  //SEI
	{
		Sei_length_1 = 
			buf[2]  << 8  |
			buf[3];
		memcpy(buffer,buf,size);
		buffer[4 + Sei_length_1] =    0x00;
		buffer[4 + Sei_length_1 +1] = 0x00;
		buffer[4 + Sei_length_1 +2] = 0x00;
		buffer[4 + Sei_length_1 +3] = 0x01;
	}
	else
	{
		memcpy(buffer,buf,size);
	}
	if (IsVideo_I_Frame == 1) //如果是关键帧
	{
		strcode[0] = 0x00;
		strcode[1] = 0x00;
		strcode[2] = 0x00;
		strcode[3] = 0x01;
		write_size = fwrite((char *)strcode,1,4,f264_handler);
		write_size = fwrite((char *)spsbuffer,1,spslength, f264_handler );
		write_size = fwrite( (char *)strcode,1,4,f264_handler );
		write_size = fwrite((char *)ppsbuffer,1,ppslength, f264_handler);
	}
	buffer[0] = 0x00;
	buffer[1] = 0x00;
	buffer[2] = 0x00;
	buffer[3] = 0x01;
	//////////////////////////////////////////////////////////////////////////
	write_size = fwrite((char *)buffer,1,size,f264_handler );
	if (buffer)
	{
		free(buffer);
		buffer == NULL;
	}
	return  write_size;
}

int And_Head_Aac( FILE *faac_handler,unsigned char * buf, unsigned int size,unsigned int IsAacFrame,unsigned char audioObjectType,unsigned char samplerate,unsigned char channelcount)
{
	int write_size = 0;
	if (IsAacFrame)
	{
		//////////////////////////////////////////////////////////////////////////
		//ADTS 头中相对有用的信息 采样率、声道数、帧长度
		//adts头
		//typedef struct
		//{
		//	unsigned int syncword;  //12 bslbf 同步字The bit string ‘1111 1111 1111’，说明一个ADTS帧的开始
		//	unsigned int id;        //1 bslbf   MPEG 标示符, 设置为1
		//	unsigned int layer;     //2 uimsbf Indicates which layer is used. Set to ‘00’
		//	unsigned int protection_absent;  //1 bslbf  表示是否误码校验


		//	unsigned int profile;            //2 uimsbf  表示使用哪个级别的AAC，如01 Low Complexity(LC)--- AACLC
		//	unsigned int sf_index;           //4 uimsbf  表示使用的采样率下标
		//	unsigned int private_bit;        //1 bslbf 
		//	unsigned int channel_configuration;  //3 uimsbf  表示声道数
		//	unsigned int original;               //1 bslbf 
		//	unsigned int home;                   //1 bslbf 
		//	/*下面的为改变的参数即每一帧都不同*/
		//	unsigned int copyright_identification_bit;   //1 bslbf 
		//	unsigned int copyright_identification_start; //1 bslbf

		//	unsigned int aac_frame_length;               // 13 bslbf  一个ADTS帧的长度包括ADTS头和raw data block
		//	unsigned int adts_buffer_fullness;           //11 bslbf     0x7FF 说明是码率可变的码流
		//
		//	/*no_raw_data_blocks_in_frame 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧.
		//	所以说number_of_raw_data_blocks_in_frame == 0 
		//	表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
		//    */
		//	unsigned int no_raw_data_blocks_in_frame;    //2 uimsfb
		//} ADTS_HEADER;

		//?0: 96000 Hz
		//?1: 88200 Hz
		//?2: 64000 Hz
		//?3: 48000 Hz
		//?4: 44100 Hz
		//?5: 32000 Hz
		//?6: 24000 Hz
		//?7: 22050 Hz
		//?8: 16000 Hz
		//?9: 12000 Hz
		//?10: 11025 Hz
		//?11: 8000 Hz
		//?12: 7350 Hz
		//?13: Reserved
		//?14: Reserved
		//?15: frequency is written explictly

		//?0: Defined in AOT Specifc Config
		//?1: 1 channel: front-center
		//?2: 2 channels: front-left, front-right
		//?3: 3 channels: front-center, front-left, front-right
		//?4: 4 channels: front-center, front-left, front-right, back-center
		//?5: 5 channels: front-center, front-left, front-right, back-left, back-right
		//?6: 6 channels: front-center, front-left, front-right, back-left, back-right, LFE-channel
		//?7: 8 channels: front-center, front-left, front-right, side-left, side-right, back-left, back-right, LFE-channel
		//?8-15: Reserved
		unsigned char  adts_headerbuf[ADTS_HEADER_LENGTH] ;
		unsigned int  aac_frame_length = size + ADTS_HEADER_LENGTH;
		adts_headerbuf[0] = 0xFF;
		adts_headerbuf[1] = 0xF1;
		//这里要用0x01 vlc 播放器才能播放
		//adts_headerbuf[2] = (audioObjectType << 6) /*如01 Low Complexity(LC)--- AACLC*/ | (samplerate << 2)  /* 采样率 44100，下标 */ | (channelcount >> 7) /*声道 = 2*/;
		adts_headerbuf[2] = (0x01 << 6) /*如01 Low Complexity(LC)--- AACLC*/ | (samplerate << 2)  /* 采样率 44100，下标 */ | (channelcount >> 7) /*声道 = 2*/;
		adts_headerbuf[3] = (channelcount << 6) |  0x00 | 0x00 | 0x00 |0x00 | ((aac_frame_length &  0x1800) >> 11);
		adts_headerbuf[4] = (aac_frame_length & 0x7F8) >> 3;
		adts_headerbuf[5] = (aac_frame_length & 0x7) << 5  |  0x1F;
		adts_headerbuf[6] = 0xFC  | 0x00;
		write_size = fwrite((char*)adts_headerbuf, sizeof(unsigned char) ,ADTS_HEADER_LENGTH, faac_handler ); 
	}
	write_size = fwrite((char *)buf,sizeof(unsigned char),size, faac_handler );
	return write_size;
}

int ReadStruct( FILE * fflv_handler, FILE *f264_handler, FILE * faac_handler )
{
	unsigned char * temporary_buf = NULL;
	unsigned int    temporary_bufsize = 0;
	unsigned int    tag_data_size = 0 ;           //Tag Data部分的大小     
	unsigned char   tag_type;                     //tag类型
	unsigned int    audiowritedatasize = 0;
	unsigned int    videowritedatasize = 0;
	unsigned int    scriptwritedatasize= 0;
	unsigned int    IsAacFrame = 0;
	unsigned char   spsbuffer[MAX_FRAME_HEAD_SIZE];
	unsigned char   ppsbuffer[MAX_FRAME_HEAD_SIZE];
	unsigned int    spslength = 0;
       unsigned int    ppslength = 0;
	unsigned char   audioObjectType;              //编解码类型：AAC-LC = 0x02
	unsigned char   samplingFrequencyIndex;       //采样率 44100 = 0x04
	unsigned char   channelConfiguration;         //声道


	if ((temporary_buf = (unsigned char *)calloc(ONE_FRAME_SIZE,sizeof(char))) == NULL)
	{
		printf ("Error: Allocate Meory To temporary_buf Buffer Failed ");
		return getchar();
	}
	
//////////////////////////////////////////////////////////////////////////
	//读取FILEHEADER
	temporary_bufsize = fread(temporary_buf,sizeof(unsigned char),9, fflv_handler );
	ReadStruct_File_Header(temporary_buf , 9 ,m_fileheadr);
	//读取各个Tag
       while ( feof ( fflv_handler ) == NO )                         //如果未到文件结尾
	{
		temporary_bufsize = fread(temporary_buf,sizeof(unsigned char),4, fflv_handler );     //PreviousTagSize
		temporary_bufsize = fread( temporary_buf, sizeof(unsigned char),11, fflv_handler );    //Tag Header
		if (temporary_bufsize == 0)
		{
			if (feof( fflv_handler ) )   //到文件结尾
			{
				break;
			}
		}
		tag_type = temporary_buf[0];
		tag_data_size = 
			temporary_buf[1]  << 16 |
			temporary_buf[2]  << 8  |
			temporary_buf[3];
		temporary_bufsize = fread( temporary_buf + 11,sizeof(unsigned char),tag_data_size, fflv_handler );
		switch (tag_type)
		{
		    case 0x08:     //音频（0x08）、视频（0x09）和script data（0x12），其它保留
			printf("audio  : tag  size  include tag header : %d\n",(tag_data_size +11));
			audiowritedatasize = ReadStruct_Aac_Tag(temporary_buf ,tag_data_size + 11 ,m_audio_tag);
			if ( m_audio_tag->SoundFormat == 0x0A )       //如果是AAC
			{
				IsAacFrame = 1;
				if(m_audio_tag->AACPacketType != 0x00)   //如果不是AudioSpecificConfig
				{
					And_Head_Aac(faac_handler,m_audio_tag->Data,audiowritedatasize,IsAacFrame,audioObjectType,samplingFrequencyIndex,channelConfiguration);
				}
				else
				{
					audioObjectType = m_audio_tag->audioasc->audioObjectType;
					samplingFrequencyIndex = m_audio_tag->audioasc->samplingFrequencyIndex;
					channelConfiguration = m_audio_tag->audioasc->channelConfiguration;
				}
			}
			else                                         //Sound data—varies by format
			{
				IsAacFrame = 0;
				And_Head_Aac( faac_handler, m_audio_tag->Data,audiowritedatasize,IsAacFrame,audioObjectType,samplingFrequencyIndex,channelConfiguration);
			}
			break;
		    case 0x09:
			printf("video  : tag  size  include tag header : %d\n",(tag_data_size +11));
			videowritedatasize = ReadStruct_H264_Tag(temporary_buf ,tag_data_size + 11 ,m_video_tag);
			if (m_video_tag->CodecID == 0x07 )  //AVCVIDEOPACKET
			{
				if (m_video_tag->AVCPacketType == 0x00)       //如果是AudioSpecificConfig
				{
					//获取sps ，pps ，buffer
					spslength = m_video_tag->video_avcc->sequenceParameterSetLength;
					ppslength = m_video_tag->video_avcc->pictureParameterSetLength;
					memcpy(spsbuffer,m_video_tag->video_avcc->sequenceParameterSetNALUnit,spslength);
					memcpy(ppsbuffer,m_video_tag->video_avcc->pictureParameterSetNALUnit,ppslength);
				}
				else if(m_video_tag->AVCPacketType != 0x02)   //如果不是empty
				{
					And_Head_H264(f264_handler,m_video_tag->Data,videowritedatasize,spsbuffer,spslength,ppsbuffer,ppslength,m_video_tag->FrameType);
				}
			}
			else
			{
				And_Head_H264(f264_handler,m_video_tag->Data,videowritedatasize,spsbuffer,spslength,ppsbuffer,ppslength,m_video_tag->FrameType);
			}
			break;
		    case 0x12:
			printf("script : tag  size  include tag header : %d\n",(tag_data_size +11));
			scriptwritedatasize = ReadStruct_Script_Tag(temporary_buf ,tag_data_size + 11 ,m_script_tag);
			break;
		    default:
			printf("others : tag  reserved\n");
		}
	}
	printf("duration                        : %lf\n",m_script_tag->duration);
	printf("width                           : %lf\n",m_script_tag->width);
	printf("height                          : %lf\n",m_script_tag->height);
	printf("ideodatarate                   : %lf\n",m_script_tag->videodatarate);
	printf("framerate                       : %lf\n",m_script_tag->framerate);
	printf("videocodecid                    : %lf\n",m_script_tag->videodatarate);
	printf("audiosamplerate                 : %lf\n",m_script_tag->audiosamplerate);
	printf("audiodatarate                   : %lf\n",m_script_tag->audiodatarate);
	printf("audiosamplesize                 : %lf\n",m_script_tag->audiosamplesize);
	printf("stereo                          : %d\n",m_script_tag->stereo);
	printf("audiocodecid                    : %lf\n",m_script_tag->audiocodecid);
	printf("filesize                        : %lf\n",m_script_tag->filesize);
	printf("lasttimetamp                    : %lf\n",m_script_tag->lasttimetamp);
	printf("lastkeyframetimetamp            : %lf\n",m_script_tag->lastkeyframetimetamp);
	if (temporary_buf) 
	{
		free(temporary_buf);
		temporary_buf = NULL;
	}
	return 1;
}

void flv_demux_2_h264_aac( const char *flv_file, const char *h264_file, const char * aac_file )
{
    	printf("++++++++++FLV DEMUX START++++++++++\n");
       FILE *fflv_handler = fopen( flv_file, "rb" );
       FILE *f264_handler = fopen( h264_file, "wb" );
       FILE *faac_handler = fopen( aac_file, "wb" );

       if( fflv_handler == NULL || f264_handler == NULL || faac_handler == NULL )
       {
            printf("FILE OPEN FAILLED:%s",strerror(errno));
            printf("++++++++++FLV DEMUX DONE++++++++++\n");
            return;
       }

      AllocStruct();
      ReadStruct( fflv_handler, f264_handler, faac_handler );
      FreeStruct();
      fclose( fflv_handler );
      fclose( f264_handler );
      fclose( faac_handler );
      printf("++++++++++FLV DEMUX DONE++++++++++\n");
}