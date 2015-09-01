/*
@file name:flv_demux.cpp
@author:chenzhengqiang
@version:1.0
@start date:2015/8/27
@modified date:
@desc:the implementation of flv demux for aac,h264
      all rights reserved!
*/


#include "ts_muxer.h"
#include "flv.h"
#include "bits_io.h"
#include "flv_demux.h"
#include <cstring>
#include <cstdlib>
#include <errno.h>
extern int errno;

flv_demux::flv_demux( const char * flv_file ):_flv_file(flv_file),
                                   _fflv_handler( NULL ),_find_aac_sequence_header(false)

{
    memset(&_aac_adts_header,0,sizeof(_aac_adts_header));
    memset(&_flv_aac_sequence_header,0,sizeof(_flv_aac_sequence_header));

    _fflv_handler = fopen(flv_file,"r" );
    if( _fflv_handler == NULL )
    {
        printf("FOPEN ERROR:%s\n",strerror(errno));
        exit( EXIT_FAILURE );
    }
}


flv_demux::~flv_demux()
{
    if( _fflv_handler != NULL )
    fclose( _fflv_handler );
}


void flv_demux::demux_for_aac( const char * aac_file )
{
    FILE *faac_handler = fopen( aac_file, "wb");
    if( faac_handler == NULL )
    return;
    
    if( !verify_format_ok( _flv_file ) )
    return;
   
    if(fseek( _fflv_handler ,_flv_header_length+4,SEEK_SET) != 0 )
    {
         return;
    }

    FLV_TAG_HEADER_INFO tag_header_info;
    while( true )
    {
         memset( &tag_header_info,0, sizeof(tag_header_info));
         if( ! get_flv_tag_header_info( tag_header_info ) )
         break;   
         //we didn't care about the stream ID,so just skip 3 bytes
	  fseek( _fflv_handler,3,SEEK_CUR );
        long cur_pos = ftell( _fflv_handler );
        
	  if( tag_header_info.tag_type == flv::TAG_AAC )
        {
            if( !_find_aac_sequence_header )
            {
                get_flv_aac_sequence_header();
            }
            else
            {   
                
                uint8_t *flv_aac_packet = new uint8_t[tag_header_info.tag_data_size];
                if( flv_aac_packet == NULL  )
                return;

                int read_bytes = fread( flv_aac_packet,sizeof(uint8_t),tag_header_info.tag_data_size,_fflv_handler );
                if( read_bytes != tag_header_info.tag_data_size )
                {
                    delete [] flv_aac_packet;
                    flv_aac_packet = NULL;
                    return;
                }

                uint8_t *aac_adts_frame = new uint8_t[(tag_header_info.tag_data_size-2)+sizeof(AAC_ADTS_HEADER )];
                if( aac_adts_frame == NULL )
                {
                    delete [] flv_aac_packet;
                    flv_aac_packet = NULL;
                    return;
                }

                //get the aac adts frame
                get_aac_adts_frame( flv_aac_packet,aac_adts_frame,tag_header_info.tag_data_size);
                //and then just write the aac adts frame to aac file
                fwrite( aac_adts_frame,sizeof(uint8_t),(tag_header_info.tag_data_size-2)+sizeof(AAC_ADTS_HEADER ),faac_handler );

                delete [] flv_aac_packet;
                flv_aac_packet = NULL;
                delete [] aac_adts_frame;
                aac_adts_frame = NULL;
            }
        }
        else
        {
             ;
        }
	 fseek( _fflv_handler, cur_pos+tag_header_info.tag_data_size+4,SEEK_SET);
    }
    if( faac_handler != NULL )
    fclose(faac_handler);
    faac_handler = NULL;
}


void flv_demux::get_flv_aac_sequence_header()
{
     int tag_header_info=0;
     int aac_packet_type=0;
     bits_io::read_8_bit( tag_header_info, _fflv_handler );
     bits_io::read_8_bit( aac_packet_type, _fflv_handler );

     //if the aac packet type is 0x00 then we know that it's flv aac's sequence header
     if ( aac_packet_type == 0x00 )
	{
		_find_aac_sequence_header = true;
		fread( &_flv_aac_sequence_header,1, sizeof(_flv_aac_sequence_header), _fflv_handler );
		_aac_adts_header.check1 = 0xff;
		_aac_adts_header.check2 = 0xf;
		_aac_adts_header.check3 = 0x1f;
		_aac_adts_header.check4 = 0x3f;
		_aac_adts_header.protection = 1;
		_aac_adts_header.ObjectType = 0;
		_aac_adts_header.SamplingIndex=_flv_aac_sequence_header.sample_index2 |_flv_aac_sequence_header.sample_index1<<1;
		_aac_adts_header.channel2=_flv_aac_sequence_header.channel;		
		return;
	}
	else
	{
	      return;
	}
}


void flv_demux::get_aac_adts_frame( uint8_t *flv_aac_packet,uint8_t *aac_adts_frame, const int & data_size)
{
    
        unsigned int size=data_size-2+7;
        _aac_adts_header.length1=(size>>11)&0x03;
        _aac_adts_header.length2=(size>>3)&0xff;
        _aac_adts_header.length3=size&0x07;
        memcpy(aac_adts_frame,(uint8_t*)&_aac_adts_header,sizeof( AAC_ADTS_HEADER ));
        memcpy(aac_adts_frame+sizeof( AAC_ADTS_HEADER ),flv_aac_packet+2,data_size-2); 
}

bool flv_demux::get_flv_tag_header_info( FLV_TAG_HEADER_INFO & tag_header_info )
{

         //read the tag type field,which holds 8 bit
         if( !bits_io::read_8_bit( tag_header_info.tag_type,_fflv_handler ))
         {  
            return false;
         }
         
         //read the data size field,which holds 24 bit
         if( ! bits_io::read_24_bit(tag_header_info.tag_data_size, _fflv_handler ))
         return false;

         //read the timestamp and the timestampextend field, which hold 32 bit total
         if( ! bits_io::read_32_bit(tag_header_info.tag_timestamp, _fflv_handler ) )
         return false;
         
         //now convert the data_size and timestamp to network byte order
         tag_header_info.tag_data_size = hton24(tag_header_info.tag_data_size);
         int tmp_timestamp = tag_header_info.tag_timestamp;
         tag_header_info.tag_timestamp = hton24( tmp_timestamp);
         tag_header_info.tag_timestamp |=( tmp_timestamp & 0xff000000 );

         return true;
}


void flv_demux::demux_for_h264( const char * h264_file )
{    
     FILE *fts_handler = fopen("./test.ts","wb");
     FILE *f264_handler = fopen( h264_file, "wb");
     if(f264_handler == NULL )
     return;
     
     if( !verify_format_ok( _flv_file ) )
     return;
   
     if(fseek( _fflv_handler , _flv_header_length+4, SEEK_SET ) != 0 )
     {
         return;
     }

     FLV_TAG_HEADER_INFO tag_header_info;
     TsPes h264_pes;
     unsigned long  h264_pts = 0;
     unsigned int   h264_frame_type =  -1;
     Ts_Adaptation_field  ts_adaptation_field_head ; 
     Ts_Adaptation_field  ts_adaptation_field_tail ;
     while( true )
     {
     
            memset( &tag_header_info,0, sizeof(tag_header_info));
            if( ! get_flv_tag_header_info( tag_header_info ) )
                   break;        
            //we didn't care the stream ID,so just skip it which holds 3 bits
            fseek(_fflv_handler, 3, SEEK_CUR );
            int cur_pos= ftell( _fflv_handler );
            
            if( tag_header_info.tag_type == flv::TAG_H264 )
            {
                int frametype_codecid=0;
	         bits_io::read_8_bit( frametype_codecid, _fflv_handler );
                if( (( frametype_codecid & 0xf0) >>4 ) == 1 )
                //printf("This Is Key Frame\n");
                ;
                else if( ((frametype_codecid & 0xf0) >>4 ) == 2)
                //printf("This Is Inter Frame\n"); 
                ;
	         int avc_packet_type=0;
	         bits_io::read_8_bit( avc_packet_type, _fflv_handler );

                //we didn;t care about the composition time,so just skip it
	         fseek( _fflv_handler, 3, SEEK_CUR );

                
	         int frame_length=0;
	         unsigned char * h264_frame = NULL; 
             
	         if ( avc_packet_type == 0)
	        {

		      fseek( _fflv_handler,6,SEEK_CUR );
                   bits_io::read_16_bit(frame_length, _fflv_handler );
		      frame_length = hton16( frame_length );
                   printf("sps size:%d\n",frame_length);
                   
                   h264_frame=(unsigned char*)malloc( frame_length+4);
                   if( h264_frame == NULL )
                   return;
                   
                   memcpy(h264_frame,&H264_SPACE,4);
		      fread(h264_frame+4,1,frame_length, _fflv_handler );
		      //fwrite(&H264_SPACE,1,3, f264_handler );
		      //h264_frame_2_pes(h264_frame,frame_length+4,h264_pts,h264_pes); 
		      fwrite(h264_frame,1,frame_length+4, f264_handler );
		      free(h264_frame);

                   bits_io::read_8_bit(frame_length, _fflv_handler );//ppsn
                   bits_io::read_16_bit(frame_length, _fflv_handler );//ppssize
                   frame_length = hton16(frame_length);
		      printf("pps size:%d\n",frame_length);
              
                   h264_frame=( unsigned char* )malloc(frame_length+4);
                   if( h264_frame == NULL )
                   return;
                   
                   memcpy(h264_frame,&H264_SPACE,4);
		      fread(h264_frame+4,1,frame_length,  _fflv_handler );
		      fwrite(h264_frame,1,frame_length+4, f264_handler );
		      free(h264_frame);
	        }
              else if( avc_packet_type == 1 )
	       {
	              //this is AVC NALU
                    int countsize=2+4;
                    h264_frame_type = 15;
                    while( countsize < tag_header_info.tag_data_size )
                    {
                    	bits_io::read_32_bit(frame_length, _fflv_handler );
                           frame_length = hton32( frame_length);
                    	h264_frame=(unsigned char*)malloc( frame_length+4);
                           if( h264_frame == NULL )
                           return;
                           memcpy(h264_frame,&H264_SPACE,4);
                    	fread(h264_frame+4,1,frame_length, _fflv_handler );
                           h264_frame_2_pes(h264_frame,frame_length+4,h264_pts,h264_pes); 
			
			if ( h264_pes.Pes_Packet_Length_Beyond != 0 )
			{
				printf("PES_VIDEO  :  SIZE = %d\n",h264_pes.Pes_Packet_Length_Beyond);
				if (h264_frame_type == FRAME_I || h264_frame_type == FRAME_P || h264_frame_type == FRAME_B)
				{
					//填写自适应段标志
					WriteAdaptive_flags_Head(&ts_adaptation_field_head,h264_pts); //填写自适应段标志帧头
					WriteAdaptive_flags_Tail(&ts_adaptation_field_tail); //填写自适应段标志帧尾
					//计算一帧视频所用时间
					PES2TS(fts_handler,&h264_pes,TS_H264_PID ,&ts_adaptation_field_head ,&ts_adaptation_field_tail,h264_pts,0);
					h264_pts += 1000* 90/30;   //90khz
				}
				else
				{
					//填写自适应段标志
					WriteAdaptive_flags_Tail(&ts_adaptation_field_head); //填写自适应段标志  ,这里注意 其它帧类型不要算pcr 所以都用帧尾代替就行
					WriteAdaptive_flags_Tail(&ts_adaptation_field_tail); //填写自适应段标志帧尾
					PES2TS(fts_handler,&h264_pes,TS_H264_PID ,&ts_adaptation_field_head ,&ts_adaptation_field_tail,h264_pts,0);
				}
			} 
                           //fwrite(&H264_SPACE,1,4, f264_handler );
                    	fwrite(h264_frame,1,frame_length+4, f264_handler );
                    	free(h264_frame);
                    	countsize+=(frame_length+5);
                    }
	       }
             else
             {
                 ;//not supported
             }
          }
          fseek( _fflv_handler ,cur_pos+tag_header_info.tag_data_size+4, SEEK_SET);
     }

     if( f264_handler != NULL )
        fclose(f264_handler);
     f264_handler = NULL;
}

