/*
@author:chenzhengqiang
@start date:2015/9/9
@desc:
*/

#include "aac.h"
#include "ts_muxer.h"
#include "common.h"
#include "flv_demux.h"
 
static const int NO = 0;
FILE *fts_handler = fopen("./test.ts","wb");
static unsigned long  h264_pts = 0; 
static unsigned long  aac_pts = 0;

int read_flv_h264_frame( FILE *f264_handler,unsigned char * buf, unsigned int size,unsigned char *sps_buffer,unsigned int sps_length,unsigned char * pps_buffer,unsigned int  pps_length ,unsigned int IsVideo_I_Frame)
{
	int write_size = 0;
	unsigned char * buffer = NULL;
	unsigned char strcode[4];             
	unsigned int Sei_length_1 = 0; 
	buffer = (unsigned char * )calloc(size + 1024,sizeof(char));

	unsigned int   aac_frame_samplerate = 0;   
	unsigned int   h264_frame_type =  0; 
	Ts_Adaptation_field  ts_adaptation_field_head ; 
	Ts_Adaptation_field  ts_adaptation_field_tail ;
    
       TsPes h264_pes;
       TsPes aac_pes;

       unsigned int frame_length = 0;
       unsigned char h264_frame[65535];
       
	if ( buf[4] == 0x06 ) 
	{
		Sei_length_1 = buf[2]  << 8  | buf[3];
		memcpy(buffer,buf,size);
		buffer[4 + Sei_length_1]      =    0x00;
		buffer[4 + Sei_length_1 +1] = 0x00;
		buffer[4 + Sei_length_1 +2] = 0x00;
		buffer[4 + Sei_length_1 +3] = 0x01;
	}
	else
	{
		memcpy(buffer,buf,size);
	}
    
	if ( IsVideo_I_Frame == 1 ) //如果是关键帧
	{
		strcode[0] = 0x00;
		strcode[1] = 0x00;
		strcode[2] = 0x00;
		strcode[3] = 0x01;
		write_size = fwrite((char *)strcode,1,4,f264_handler);
		write_size = fwrite((char *)sps_buffer,1,sps_length, f264_handler );
             memcpy(h264_frame,strcode,4); 
             memcpy(h264_frame+4,sps_buffer,sps_length);
             h264_frame_2_pes(h264_frame,sps_length+4,h264_pts,h264_pes); 
		if ( h264_pes.Pes_Packet_Length_Beyond != 0 )
		{
			printf("PES_VIDEO  :  SIZE = %d\n",h264_pes.Pes_Packet_Length_Beyond);
			write_adaptive_head_fields(&ts_adaptation_field_head,h264_pts); 
		       write_adaptive_tail_fields(&ts_adaptation_field_tail); 
			pes_2_ts(fts_handler,&h264_pes,TS_H264_PID ,&ts_adaptation_field_head ,&ts_adaptation_field_tail,h264_pts,aac_pts);
			h264_pts += 1000* 90 /30;   //90khz
		}
		write_size = fwrite((char *)strcode,1,4,f264_handler );
		write_size = fwrite((char *)pps_buffer,1,pps_length, f264_handler);
              memcpy(h264_frame+4,pps_buffer,pps_length);
              h264_frame_2_pes(h264_frame,pps_length+4,h264_pts,h264_pes); 
		if ( h264_pes.Pes_Packet_Length_Beyond != 0 )
		{
			printf("PES_VIDEO  :  SIZE = %d\n",h264_pes.Pes_Packet_Length_Beyond);
			write_adaptive_head_fields(&ts_adaptation_field_head,h264_pts); 
		       write_adaptive_tail_fields(&ts_adaptation_field_tail); 
			pes_2_ts(fts_handler,&h264_pes,TS_H264_PID ,&ts_adaptation_field_head ,&ts_adaptation_field_tail,h264_pts,aac_pts);
			h264_pts += 1000* 90 /30;   //90khz
		}
	}
    
	buffer[0] = 0x00;
	buffer[1] = 0x00;
	buffer[2] = 0x00;
	buffer[3] = 0x01;
	
	write_size = fwrite((char *)buffer,1,size,f264_handler );
       h264_frame_2_pes(buffer,size,h264_pts,h264_pes); 
       
	if ( h264_pes.Pes_Packet_Length_Beyond != 0 )
	{
		printf("PES_VIDEO  :  SIZE = %d\n",h264_pes.Pes_Packet_Length_Beyond);
		write_adaptive_head_fields(&ts_adaptation_field_head,h264_pts);
		write_adaptive_tail_fields(&ts_adaptation_field_tail); 
		pes_2_ts(fts_handler,&h264_pes,TS_H264_PID ,&ts_adaptation_field_head ,&ts_adaptation_field_tail,h264_pts,aac_pts);
		h264_pts += 1000* 90 /30;   //90khz
	}
    
	if (buffer)
	{
		free(buffer);
		buffer = NULL;
	}
	return  write_size;
}

int read_flv_aac_frame( FILE *faac_handler,unsigned char * buf, unsigned int size,bool is_aac_frame,unsigned char audioObjectType,unsigned char samplerate,unsigned char channelcount)
{
       (void)audioObjectType; 
	int write_size = 0;
	if ( is_aac_frame )
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
		unsigned char  adts_headerbuf[ADTS_HEADER_SIZE] ;
		unsigned int  aac_frame_length = size + ADTS_HEADER_SIZE;
		adts_headerbuf[0] = 0xFF;
		adts_headerbuf[1] = 0xF1;
		//这里要用0x01 vlc 播放器才能播放
		//adts_headerbuf[2] = (audioObjectType << 6) /*如01 Low Complexity(LC)--- AACLC*/ | (samplerate << 2)  /* 采样率 44100，下标 */ | (channelcount >> 7) /*声道 = 2*/;
		adts_headerbuf[2] = (0x01 << 6) /*如01 Low Complexity(LC)--- AACLC*/ | (samplerate << 2)  /* 采样率 44100，下标 */ | (channelcount >> 7) /*声道 = 2*/;
		adts_headerbuf[3] = (channelcount << 6) |  0x00 | 0x00 | 0x00 |0x00 | ((aac_frame_length &  0x1800) >> 11);
		adts_headerbuf[4] = (aac_frame_length & 0x7F8) >> 3;
		adts_headerbuf[5] = (aac_frame_length & 0x7) << 5  |  0x1F;
		adts_headerbuf[6] = 0xFC  | 0x00;
		write_size = fwrite((char*)adts_headerbuf, sizeof(unsigned char) ,ADTS_HEADER_SIZE, faac_handler ); 
	}
	write_size = fwrite((char *)buf,sizeof(unsigned char),size, faac_handler );
	return write_size;
}



int read_flv_tag_data( FILE * fflv_handler, FILE *f264_handler, FILE * faac_handler )
{
       #define CHECK_READ(X)\
       if( (X) == 0 )\
       {\
            if(feof(fflv_handler))\
            break;\
            else\
            return SYSTEM_ERROR;\
       }

       FLV_HEADER flv_header;
       FLV_AAC_TAG aac_tag;
       FLV_H264_TAG h264_tag;
       FLV_SCRIPT_TAG script_tag;
       int ret;

       unsigned char  flv_header_buffer[FLV_HEADER_SIZE];
       unsigned char  previous_tag_size[PREVIOUS_TAG_SIZE];
       unsigned char  flv_tag_header[TAG_HEADER_SIZE];
	unsigned char  flv_tag_data[FLV_FRAME_SIZE];
	unsigned int    read_bytes = 0;
	unsigned int    tag_data_size = 0 ;           //Tag Data部分的大小     
	unsigned char  tag_type;                     //tag类型
	unsigned int    audio_tag_data_size = 0;
	unsigned int    video_tag_data_size = 0;
	
	bool    is_aac_frame = false;
	unsigned char  sps_buffer[MAX_FRAME_HEAD_SIZE];
	unsigned char  pps_buffer[MAX_FRAME_HEAD_SIZE];
	unsigned int    sps_length = 0;
       unsigned int    pps_length = 0;
	unsigned char   audioObjectType;              //编解码类型：AAC-LC = 0x02
	unsigned char   samplingFrequencyIndex;       //采样率 44100 = 0x04
	unsigned char   channelConfiguration;         //声道
	
	read_bytes = fread(flv_header_buffer,sizeof(unsigned char),FLV_HEADER_SIZE, fflv_handler );
       if( read_bytes == 0 || read_bytes != FLV_HEADER_SIZE )
       {
            if( feof(fflv_handler) )
            return FILE_EOF;    
            return SYSTEM_ERROR;
       }
       
	ret = read_flv_header(flv_header_buffer,FLV_HEADER_SIZE,flv_header);
       if( ret != OK )
       return ret; 

       while ( feof ( fflv_handler ) == NO )               
	{
	       //flv file consists of PreviousTagSize(4 bytes)+tag()
	       //read the previous tag size first,it holds 4 bytes
		read_bytes = fread(previous_tag_size,sizeof(unsigned char),PREVIOUS_TAG_SIZE, fflv_handler );
		CHECK_READ(read_bytes);
             
             //read the tag header,it always hold 11 bytes
		read_bytes = fread(flv_tag_header, sizeof(unsigned char),TAG_HEADER_SIZE, fflv_handler);
		CHECK_READ(read_bytes);
             
		tag_type = flv_tag_header[0];
		tag_data_size = flv_tag_header[1]  << 16 |flv_tag_header[2]  << 8  | flv_tag_header[3];

             //now read the flv tag data for each media type
		read_bytes = fread( flv_tag_data,sizeof(unsigned char),tag_data_size, fflv_handler );
             CHECK_READ(read_bytes);
             
		switch ( tag_type )
		{
		    case TAG_AUDIO:  
			audio_tag_data_size = get_flv_aac_tag(flv_tag_header,flv_tag_data,tag_data_size,aac_tag);
			if ( audio_tag_data_size != -1) 
			{
				is_aac_frame = true;
				if( aac_tag.AACPacketType == FLV_AAC_RAW )
				{
					read_flv_aac_frame(faac_handler,aac_tag.Data,audio_tag_data_size,is_aac_frame,audioObjectType,samplingFrequencyIndex,channelConfiguration);
				}
				else
				{
					audioObjectType = aac_tag.audioasc.audioObjectType;
					samplingFrequencyIndex = aac_tag.audioasc.samplingFrequencyIndex;
					channelConfiguration = aac_tag.audioasc.channelConfiguration;
				}
			}
			break;
		    case TAG_VIDEO:
			video_tag_data_size = get_flv_h264_tag( flv_tag_header,flv_tag_data ,tag_data_size,h264_tag);
			if ( h264_tag.CodecID == 0x07 ) 
			{
				if ( h264_tag.AVCPacketType == 0x00 )
				{
					sps_length = h264_tag.video_avcc.sequenceParameterSetLength;
					pps_length = h264_tag.video_avcc.pictureParameterSetLength;
					memcpy(sps_buffer,h264_tag.video_avcc.sequenceParameterSetNALUnit,sps_length);
					memcpy(pps_buffer,h264_tag.video_avcc.pictureParameterSetNALUnit,pps_length);
				}
				else if( h264_tag.AVCPacketType == 0x01 ) 
				{
					read_flv_h264_frame(f264_handler,h264_tag.Data,video_tag_data_size,sps_buffer,sps_length,pps_buffer,pps_length,h264_tag.FrameType);
				}
			}
			break;
		    case TAG_SCRIPT:
			get_flv_script_tag( flv_tag_header,flv_tag_data,tag_data_size,script_tag);
			break;
		    default:
			;
		}
	}
       
	printf("duration                        : %lf\n",script_tag.duration);
	printf("width                           : %lf\n",script_tag.width);
	printf("height                          : %lf\n",script_tag.height);
	printf("ideodatarate                   : %lf\n",script_tag.videodatarate);
	printf("framerate                       : %lf\n",script_tag.framerate);
	printf("videocodecid                    : %lf\n",script_tag.videodatarate);
	printf("audiosamplerate                 : %lf\n",script_tag.audiosamplerate);
	printf("audiodatarate                   : %lf\n",script_tag.audiodatarate);
	printf("audiosamplesize                 : %lf\n",script_tag.audiosamplesize);
	printf("stereo                          : %d\n",script_tag.stereo);
	printf("audiocodecid                    : %lf\n",script_tag.audiocodecid);
	printf("filesize                        : %lf\n",script_tag.filesize);
	printf("lasttimetamp                    : %lf\n",script_tag.lasttimetamp);
	printf("lastkeyframetimetamp            : %lf\n",script_tag.lastkeyframetimetamp);
	return 1;
}



void flv_demux_2_h264_aac( const char *flv_file, const char *h264_file, const char * aac_file )
{
    	printf("++++++++++++++++FLV DEMUX START++++++++++++++++\n");
       FILE *fflv_handler = fopen( flv_file, "rb" );
       FILE *f264_handler = fopen( h264_file, "wb" );
       FILE *faac_handler = fopen( aac_file, "wb" );

       if( fflv_handler == NULL || f264_handler == NULL || faac_handler == NULL )
       {
            printf("FILE OPEN FAILLED:%s",strerror(errno));
            printf("++++++++++++++++FLV DEMUX DONE++++++++++++++++\n");
            return;
       }
       
      read_flv_tag_data( fflv_handler, f264_handler, faac_handler );
      fclose( fflv_handler );
      fclose( f264_handler );
      fclose( faac_handler );
      printf("++++++++++++++++FLV DEMUX DONE++++++++++++++++\n");
}