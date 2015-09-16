/*
@author:chenzhengqiang
@start date:2015/9/9
@desc:
*/

#include "flv.h"
#include "flv_aac.h"
#include "flv_avc.h"
#include "flv_script.h"
#include "flv_demux.h"
#include "common.h"
#include <queue> 
#include <time.h>
using std::queue;
static const int NO = 0;
static const int AAC_SAMPLERATES[13]={96000,88200,
64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350};
static const int H264_FRAME_RATE = 30;

int read_flv_h264_frame( FILE *f264_handler,unsigned char * h264_nalu_buffer, unsigned int size,unsigned char *sps_buffer,unsigned int sps_length,unsigned char * pps_buffer,unsigned int  pps_length ,unsigned int IsVideo_I_Frame)
{
	int write_size = 0;
	unsigned char * buffer = NULL;
	unsigned char strcode[4];             
	unsigned int sei_length = 0; 
	buffer = (unsigned char * )calloc(size + 1024,sizeof(char));

	if ( h264_nalu_buffer[4] == 0x06 ) 
	{
		sei_length = h264_nalu_buffer[2]  << 8  | h264_nalu_buffer[3];
		memcpy(buffer,h264_nalu_buffer,size);
		buffer[4 + sei_length]      =  0x00;
		buffer[4 + sei_length +1] = 0x00;
		buffer[4 + sei_length +2] = 0x00;
		buffer[4 + sei_length +3] = 0x01;
	}
	else
	{
		memcpy(buffer,h264_nalu_buffer,size);
	}
    
	if ( IsVideo_I_Frame == FLV_KEY_FRAME ) 
	{
		strcode[0] = 0x00;
		strcode[1] = 0x00;
		strcode[2] = 0x00;
		strcode[3] = 0x01;
		write_size = fwrite((char *)strcode,1,4,f264_handler);
		write_size = fwrite((char *)sps_buffer,1,sps_length, f264_handler );
		write_size = fwrite((char *)strcode,1,4,f264_handler );
		write_size = fwrite((char *)pps_buffer,1,pps_length, f264_handler);
	}
    
	buffer[0] = 0x00;
	buffer[1] = 0x00;
	buffer[2] = 0x00;
	buffer[3] = 0x01;
	
	write_size = fwrite((char *)buffer,1,size,f264_handler );
	if (buffer)
	{
		free(buffer);
		buffer = NULL;
	}
	return  write_size;
}


int read_flv_aac_frame( FILE *faac_handler,unsigned char * buf, unsigned int size,unsigned char samplerate,unsigned char channelcount )
{
	int write_size = 0;
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
	int    aac_tag_payload_size = 0;
	int    avc_tag_payload_size = 0;
	
	
	unsigned char  sps_buffer[MAX_FRAME_HEAD_SIZE];
	unsigned char  pps_buffer[MAX_FRAME_HEAD_SIZE];
	unsigned int    sps_length = 0;
       unsigned int    pps_length = 0;
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
			aac_tag_payload_size = get_flv_aac_tag(flv_tag_header,flv_tag_data,tag_data_size,aac_tag);
			if ( aac_tag_payload_size != -1) 
			{
			       printf("aac tag payload size:%d\n",aac_tag_payload_size);
				if( aac_tag.AACPacketType == FLV_AAC_RAW_TYPE )
				{
	                           unsigned char  adts_headerbuf[ADTS_HEADER_SIZE] ;
	                           unsigned int  aac_frame_length = aac_tag_payload_size + ADTS_HEADER_SIZE;
	                           adts_headerbuf[0] = 0xFF;
	                           adts_headerbuf[1] = 0xF1;
	                           //这里要用0x01 vlc 播放器才能播放
	                           //adts_headerbuf[2] = (audioObjectType << 6) /*如01 Low Complexity(LC)--- AACLC*/ | (samplerate << 2)  /* 采样率 44100，下标 */ | (channelcount >> 7) /*声道 = 2*/;
	                           adts_headerbuf[2] = (0x01 << 6) /*如01 Low Complexity(LC)--- AACLC*/ | (aac_tag.adts.samplingFrequencyIndex<< 2)  /* 采样率 44100，下标 */ | (aac_tag.adts.channelConfiguration >> 7) /*声道 = 2*/;
	                           adts_headerbuf[3] = (aac_tag.adts.channelConfiguration << 6) |  0x00 | 0x00 | 0x00 |0x00 | ((aac_frame_length &  0x1800) >> 11);
	                           adts_headerbuf[4] = (aac_frame_length & 0x7F8) >> 3;
	                           adts_headerbuf[5] = (aac_frame_length & 0x7) << 5  |  0x1F;
	                           adts_headerbuf[6] = 0xFC  | 0x00;
	                           fwrite((char*)adts_headerbuf, sizeof(unsigned char) ,ADTS_HEADER_SIZE, faac_handler ); 
	                           fwrite((char *)aac_tag.Payload,sizeof(unsigned char),aac_tag_payload_size, faac_handler );
				}
			}
			break;
		    case TAG_VIDEO:
			avc_tag_payload_size = get_flv_h264_tag( flv_tag_header,flv_tag_data,tag_data_size,h264_tag);
			if ( avc_tag_payload_size != -1 ) 
			{
				if ( h264_tag.AVCPacketType == AVC_SEQUENCE_HEADER_TYPE )
				{
					sps_length = h264_tag.adcr.sequenceParameterSetLength;
					pps_length = h264_tag.adcr.pictureParameterSetLength;
					memcpy(sps_buffer,h264_tag.adcr.sequenceParameterSetNALUnit,sps_length);
					memcpy(pps_buffer,h264_tag.adcr.pictureParameterSetNALUnit,pps_length);
				}
				else if( h264_tag.AVCPacketType == FLV_AVC_NALU_TYPE ) 
				{
					//read_flv_h264_frame(f264_handler,h264_tag.Payload,avc_tag_payload_size,sps_buffer,sps_length,pps_buffer,pps_length,h264_tag.FrameType);
					int write_size = 0;
                                 unsigned char * h264_stream_buffer = (unsigned char * )calloc(avc_tag_payload_size+ 1024,sizeof(char));
	                           unsigned char strcode[4];             
	                           unsigned int sei_length = 0;  
                                 unsigned char h264_frame[65535];
                                 if ( h264_tag.Payload[4] == 0x06 ) 
	                          {
		                        sei_length = h264_tag.Payload[2]  << 8  | h264_tag.Payload[3];
                                     memcpy(h264_frame,h264_tag.Payload,avc_tag_payload_size);
		                        h264_stream_buffer[4 + sei_length]      =    0x00;
		                        h264_stream_buffer[4 + sei_length +1] = 0x00;
		                        h264_stream_buffer[4 + sei_length +2] = 0x00;
		                        h264_stream_buffer[4 + sei_length +3] = 0x01;
	                          }
	                          else
	                          {
		                        memcpy(h264_stream_buffer,h264_tag.Payload,avc_tag_payload_size);
	                          }
    
	                          if ( h264_tag.FrameType == FLV_KEY_FRAME )
	                          {
		                        strcode[0] = 0x00;
		                        strcode[1] = 0x00;
		                        strcode[2] = 0x00;
		                        strcode[3] = 0x01;
		                        write_size = fwrite((char *)strcode,1,4,f264_handler);
		                        write_size = fwrite((char *)sps_buffer,1,sps_length, f264_handler );
		                        write_size = fwrite((char *)strcode,1,4,f264_handler );
		                        write_size = fwrite((char *)pps_buffer,1,pps_length, f264_handler);
                         
	                          }
    
	                          h264_stream_buffer[0] = 0x00;
	                          h264_stream_buffer[1] = 0x00;
	                          h264_stream_buffer[2] = 0x00;
	                          h264_stream_buffer[3] = 0x01;
	                          write_size = fwrite((char *)h264_stream_buffer,1,avc_tag_payload_size,f264_handler );
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
