/*
@file name:flv_demux.cpp
@author:chenzhengqiang
@version:1.0
@start date:2015/8/27
@modified date:
@desc:the implementation of flv demux for aac,h264
*/

#include "flv.h"
#include "bits_io.h"
#include "flv_demux.h"
#include <cstring>
#include <cstdlib>
#include <errno.h>
extern int errno;

flv_demux::flv_demux( const char * flv_file, const char * aac_file ):_flv_file(flv_file),
_fflv_handler( NULL ),_faac_handler( NULL ),_find_aac_sequence_header(false)

{
    memset(&_aac_adts_header,0,sizeof(_aac_adts_header));
    memset(&_flv_aac_sequence_header,0,sizeof(_flv_aac_sequence_header));

    _fflv_handler = fopen( _flv_file,"r" );
    if( _fflv_handler == NULL )
    {
        printf("FOPEN ERROR:%s\n",strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    _faac_handler = fopen( aac_file, "w" );
    if( _faac_handler == NULL )
    {
        if( _fflv_handler != NULL )
        fclose( _fflv_handler );    
        printf("FOPEN ERROR:%s\n",strerror(errno));
        exit(EXIT_FAILURE);
    }
}


flv_demux::~flv_demux()
{
    if( _fflv_handler != NULL )
    fclose( _fflv_handler );
    if( _faac_handler != NULL )
    fclose( _faac_handler );    
}


void flv_demux::demux_for_aac()
{
    if( !verify_format_ok( _flv_file ) )
    return;
   
    if(fseek( _fflv_handler ,_flv_header_length+4,SEEK_SET) != 0 )
    {
         printf("FSEEK ERROR:%s\n",strerror(errno));
         return;
    }

    while( true )
    {
         //read the tag type field,which holds 8 bit
         int tag_type=0;
         if( !bits_io::read_8_bit(tag_type,_fflv_handler ))
         {  
            return;
         }
         
         //read the data size field,which holds 24 bit
         int data_size = 0;
         if( ! bits_io::read_24_bit( data_size, _fflv_handler ))
         return;

         //read the timestamp and the timestampextend field, which hold 32 bit total
         int timestamp = 0;
         if( ! bits_io::read_32_bit( timestamp, _fflv_handler ) )
         return;
         
         //now convert the data_size and timestamp to network byte order
         data_size = hton24(data_size);
         int tmp_timestamp = timestamp;
         timestamp = hton24( tmp_timestamp);
         timestamp |=( tmp_timestamp & 0xff000000 );

         //we didn't care about the stream ID,so just skip 3 bytes
	  fseek( _fflv_handler ,3,SEEK_CUR);
         
        long cur_pos = ftell( _fflv_handler );
        
	  if( tag_type == flv::TAG_AAC )
        {
            if( !_find_aac_sequence_header )
            {
                get_flv_aac_sequence_header(data_size);
            }
            else
            {   
                
                uint8_t *flv_aac_packet = new uint8_t[data_size];
                if( flv_aac_packet == NULL  )
                return;

                int read_bytes = fread( flv_aac_packet,sizeof(uint8_t),data_size,_fflv_handler );
                if( read_bytes != data_size )
                {
                    delete [] flv_aac_packet;
                    flv_aac_packet = NULL;
                    return;
                }

                uint8_t *aac_adts_frame = new uint8_t[(data_size-2)+sizeof(AAC_ADTS_HEADER )];
                if( aac_adts_frame == NULL )
                {
                    delete [] flv_aac_packet;
                    flv_aac_packet = NULL;
                    return;
                }

                //get the aac adts frame
                get_aac_adts_frame( flv_aac_packet,aac_adts_frame,data_size);
                //and then just write the aac adts frame to aac file
                fwrite( aac_adts_frame,sizeof(uint8_t),(data_size-2)+sizeof(AAC_ADTS_HEADER ),_faac_handler );

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
	 fseek( _fflv_handler, cur_pos+data_size+4,SEEK_SET);
    }
}


void flv_demux::get_flv_aac_sequence_header( const int & data_size )
{
     int tag_header_info=0;
     int aac_packet_type=0;
     bits_io::read_8_bit( tag_header_info, _fflv_handler );
     bits_io::read_8_bit( aac_packet_type, _fflv_handler );

     //if the aac packet type is 0x00 then we know that it's flv aac's sequence header
     if ( aac_packet_type == 0x00 )
	{
		_find_aac_sequence_header = true;
		fread(&_flv_aac_sequence_header,1,sizeof(_flv_aac_sequence_header),_fflv_handler);
        
		_aac_adts_header.check1=0xff;
		_aac_adts_header.check2=0xf;
		_aac_adts_header.check3=0x1f;
		_aac_adts_header.check4=0x3f;
		_aac_adts_header.protection=1;
		_aac_adts_header.ObjectType=0;
		_aac_adts_header.SamplingIndex=_flv_aac_sequence_header.sample_index2 |_flv_aac_sequence_header.sample_index1<<1;
		_aac_adts_header.channel2=_flv_aac_sequence_header.channel;		
		return;
	}
	else
	{
	      return;
	}
}


void flv_demux::goto_h264_demux( const int & data_size )
{
     (void)data_size;
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
