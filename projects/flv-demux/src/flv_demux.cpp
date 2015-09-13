/*
@author:chenzhengqiang
@start date:2015/9/9
@desc:
*/

#include "m3u8.h"
#include "flv.h"
#include "flv_aac.h"
#include "flv_avc.h"
#include "flv_script.h"
#include "flv_demux.h"
#include "ts_muxer.h"
#include "common.h"
#include <queue> 
#include <time.h>
using std::queue;
static const int NO = 0;
static const int AAC_SAMPLERATES[13]={96000,88200,
64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350};
static const int H264_FRAME_RATE = 30;
struct TS_PES_FRAME
{
    unsigned char frame_buffer[65535*10];
    unsigned long frame_length;
};
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

       unsigned long h264_pts = 0;
       unsigned long aac_pts = 0;
	
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

       queue<TS_PES_FRAME> aac_es_queue;
       queue<TS_PES_FRAME> avc_es_queue;
       TS_PES_FRAME es_frame;

       M3U8_CONFIG m3u8_config;
       m3u8_config.path="./";
       m3u8_config.channel="balabala";
       m3u8_config.timestamp=time(NULL);
       m3u8_config.media_prev_sequence = 1;
       m3u8_config.media_cur_sequence = 1;
       m3u8_config.target_duration = 10;
       time_t prev = m3u8_config.timestamp;
       time_t now;

       FILE *fm3u8_handler = create_m3u8_file(  m3u8_config );
       write_m3u8_file_header( fm3u8_handler, m3u8_config );
       char ts_url[99];
       FILE *fts_handler = create_ts_file(ts_url,sizeof(ts_url),m3u8_config);
       add_ts_url_2_m3u8_file(&fm3u8_handler, ts_url, m3u8_config );
       int times = 1;
       while ( feof ( fflv_handler ) == NO )               
	{
	       now=time( NULL );
              if( now - prev == 10 )
              {
                    printf("now:%ld prev:%ld:%ld",now,prev);
                    ++times;
                    prev = now;
                    m3u8_config.timestamp = now;
                    fclose(fts_handler);
                    fts_handler = create_ts_file(ts_url,sizeof(ts_url),m3u8_config);
                    add_ts_url_2_m3u8_file(&fm3u8_handler, ts_url, m3u8_config );
              }

              if( times == 3 )
              {
                    times = 0;
                    m3u8_config.media_cur_sequence+=1;
              }
              
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

                                  es_frame.frame_length = ADTS_HEADER_SIZE+aac_tag_payload_size;
                                  memcpy(es_frame.frame_buffer,adts_headerbuf,ADTS_HEADER_SIZE);
                                  memcpy(es_frame.frame_buffer+ADTS_HEADER_SIZE,(char *)aac_tag.Payload,aac_tag_payload_size);

                                  aac_es_queue.push(es_frame);
                                  if( aac_pts < h264_pts )
                                  {
                                        TsPes aac_pes;
                                        es_frame = aac_es_queue.front();
                                        aac_frame_2_pes(es_frame.frame_buffer,es_frame.frame_length,aac_pts,aac_pes);
                                        aac_es_queue.pop();
                                        Ts_Adaptation_field  ts_adaptation_field_head; 
	                                 Ts_Adaptation_field  ts_adaptation_field_tail;
			                    if (aac_pes.Pes_Packet_Length_Beyond != 0)
			                    {
				                    printf("PES_AUDIO  :  SIZE = %d\n",aac_pes.Pes_Packet_Length_Beyond);
				                    //填写自适应段标志
				                    write_adaptive_tail_fields(&ts_adaptation_field_head); //填写自适应段标志  ,这里注意 音频类型不要算pcr 所以都用帧尾代替就行
				                    write_adaptive_tail_fields(&ts_adaptation_field_tail); //填写自适应段标志帧尾
				                    pes_2_ts(fts_handler,&aac_pes,TS_AAC_PID ,&ts_adaptation_field_head ,&ts_adaptation_field_tail,h264_pts,aac_pts);
				                    //计算一帧音频所用时间
				                    aac_pts += 1024*1000* 90/AAC_SAMPLERATES[aac_tag.adts.samplingFrequencyIndex];
			                    }
                                }
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
	                           Ts_Adaptation_field  ts_adaptation_field_head ; 
	                           Ts_Adaptation_field  ts_adaptation_field_tail ;
                                 TsPes h264_pes;
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
                                      memcpy(es_frame.frame_buffer,strcode,4);
                                      memcpy(es_frame.frame_buffer+4,sps_buffer,sps_length);
                                      es_frame.frame_length = 4+sps_length;
                                      avc_es_queue.push(es_frame);

                                      if( h264_pts <= aac_pts )
                                      {
                                            es_frame = avc_es_queue.front();
                                            h264_frame_2_pes(es_frame.frame_buffer,es_frame.frame_length,h264_pts,h264_pes);
                                            avc_es_queue.pop();
		                               if ( h264_pes.Pes_Packet_Length_Beyond != 0 )
		                               {
			                            printf("PES_VIDEO  :  SIZE = %d\n",h264_pes.Pes_Packet_Length_Beyond);
			                            write_adaptive_head_fields(&ts_adaptation_field_head,h264_pts); 
		                                   write_adaptive_tail_fields(&ts_adaptation_field_tail); 
			                            pes_2_ts(fts_handler,&h264_pes,TS_H264_PID ,&ts_adaptation_field_head ,&ts_adaptation_field_tail,h264_pts,aac_pts);
			                            h264_pts += 1000* 90 /H264_FRAME_RATE;   //90khz
		                               }
                                      }
		                        write_size = fwrite((char *)strcode,1,4,f264_handler );
		                        write_size = fwrite((char *)pps_buffer,1,pps_length, f264_handler);

                                     memcpy(es_frame.frame_buffer,strcode,4);
                                     memcpy(es_frame.frame_buffer+4,pps_buffer,pps_length);
                                     es_frame.frame_length = 4+pps_length;
                                     avc_es_queue.push(es_frame);

                                     if( h264_pts <= aac_pts )
                                     {
                                            es_frame = avc_es_queue.front();
                                            h264_frame_2_pes(es_frame.frame_buffer,es_frame.frame_length,h264_pts,h264_pes);
                                            avc_es_queue.pop();
		                               if ( h264_pes.Pes_Packet_Length_Beyond != 0 )
		                               {
			                               printf("PES_VIDEO  :  SIZE = %d\n",h264_pes.Pes_Packet_Length_Beyond);
			                               write_adaptive_head_fields(&ts_adaptation_field_head,h264_pts); 
		                                      write_adaptive_tail_fields(&ts_adaptation_field_tail); 
			                               pes_2_ts(fts_handler,&h264_pes,TS_H264_PID ,&ts_adaptation_field_head ,&ts_adaptation_field_tail,h264_pts,aac_pts);
			                               h264_pts += 1000* 90 /H264_FRAME_RATE;   //90khz
		                               }
                                      }
	                          }
    
	                          h264_stream_buffer[0] = 0x00;
	                          h264_stream_buffer[1] = 0x00;
	                          h264_stream_buffer[2] = 0x00;
	                          h264_stream_buffer[3] = 0x01;
	                          write_size = fwrite((char *)h264_stream_buffer,1,avc_tag_payload_size,f264_handler );
                                 memcpy(es_frame.frame_buffer,h264_stream_buffer,avc_tag_payload_size);
                                 es_frame.frame_length = avc_tag_payload_size;
                                 avc_es_queue.push(es_frame);

                                if( h264_pts <= aac_pts )
                                {
                                        es_frame = avc_es_queue.front();
                                        h264_frame_2_pes(es_frame.frame_buffer,es_frame.frame_length,h264_pts,h264_pes); 
                                        avc_es_queue.pop();
                                        
	                                 if ( h264_pes.Pes_Packet_Length_Beyond != 0 )
	                                 {
		                                printf("PES_VIDEO  :  SIZE = %d\n",h264_pes.Pes_Packet_Length_Beyond);
		                                write_adaptive_head_fields(&ts_adaptation_field_head,h264_pts);
		                                write_adaptive_tail_fields(&ts_adaptation_field_tail); 
		                                pes_2_ts(fts_handler,&h264_pes,TS_H264_PID ,&ts_adaptation_field_head ,&ts_adaptation_field_tail,h264_pts,aac_pts);
		                                h264_pts += 1000* 90 /H264_FRAME_RATE;   //90khz
	                                }
                                }   
	                         if ( h264_stream_buffer )
	                         {
		                        free(h264_stream_buffer);
		                        h264_stream_buffer= NULL;
	                         }
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
