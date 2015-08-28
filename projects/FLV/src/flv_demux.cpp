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

flv_demux::flv_demux( const char * flv_file, const char* h264_file, const char * aac_file ):_flv_file(flv_file),
_fflv_handler( NULL ),_faac_handler( NULL ),_find_aac_sequence_header(false)

{
    memset(&_aac_adts_header,0,sizeof(_aac_adts_header));
    memset(&_flv_aac_sequence_header,0,sizeof(_flv_aac_sequence_header));

    _fflv_handler = fopen(flv_file,"r" );
    if( _fflv_handler == NULL )
    {
        printf("FOPEN ERROR:%s\n",strerror(errno));
        exit( EXIT_FAILURE );
    }

    _f264_handler = fopen( h264_file, "w");
    if( _f264_handler == NULL )
    {
        fclose( _fflv_handler );
        _fflv_handler = NULL;
        printf("FOPEN ERROR:%s\n",strerror(errno));
        exit( EXIT_FAILURE );
    }
    
    _faac_handler = fopen( aac_file, "w" );
    if( _faac_handler == NULL )
    {
        if( _fflv_handler != NULL )
        fclose( _fflv_handler );
        fclose(_f264_handler);
        printf("FOPEN ERROR:%s\n",strerror(errno));
        exit(EXIT_FAILURE);
    }
}


flv_demux::~flv_demux()
{
    if( _fflv_handler != NULL )
    fclose( _fflv_handler );
    if( _f264_handler != NULL )
    fclose( _f264_handler );    
    if( _faac_handler != NULL )
    fclose( _faac_handler );    
}


void flv_demux::demux_for_aac()
{

    if( _fflv_handler == NULL || _faac_handler == NULL )
    return;
    
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


void flv_demux::get_aac_adts_frame( uint8_t *flv_aac_packet,uint8_t *aac_adts_frame, const int & data_size)
{
    
        unsigned int size=data_size-2+7;
        _aac_adts_header.length1=(size>>11)&0x03;
        _aac_adts_header.length2=(size>>3)&0xff;
        _aac_adts_header.length3=size&0x07;
        memcpy(aac_adts_frame,(uint8_t*)&_aac_adts_header,sizeof( AAC_ADTS_HEADER ));
        memcpy(aac_adts_frame+sizeof( AAC_ADTS_HEADER ),flv_aac_packet+2,data_size-2); 
}


void flv_demux::demux_for_h264()
{
     if( _fflv_handler == NULL || _f264_handler == NULL )
     return;
    
     if( !verify_format_ok( _flv_file ) )
     return;
   
     if(fseek( _fflv_handler , _flv_header_length+4,SEEK_SET) != 0 )
     {
         printf("FSEEK ERROR:%s\n",strerror(errno));
         return;
     }

     while( true )
     {
          
            int tag_type=0;
            int timestamp =0;
            int tmp_timestamp = 0;
            int data_size =0;
            
            if (!bits_io::read_8_bit( tag_type,_fflv_handler ))
            break;
            
            if (!bits_io::read_24_bit( data_size , _fflv_handler ))
            break;
            
            if(!bits_io::read_32_bit( timestamp, _fflv_handler ))
            break;

            data_size = hton24( data_size );
            tmp_timestamp = timestamp;
            timestamp = hton24( tmp_timestamp);
            timestamp |=( tmp_timestamp & 0xff000000 );
            
            //we didn't care the stream ID,so just skip it which holds 3 bits
            fseek(_fflv_handler, 3, SEEK_CUR );
            int cur_pos= ftell( _fflv_handler );
            
            if( tag_type == flv::TAG_H264 )
            {
                int frametype_codecid=0;
	         bits_io::read_8_bit( frametype_codecid, _fflv_handler );
                if( ((frametype_codecid & 0xf0) >>4) == 1 )
                //printf("This Is Key Frame\n");
                ;
                else if( ((frametype_codecid & 0xf0) >>4) == 2)
                //printf("This Is Inter Frame\n"); 
                ;
	         int avc_packet_type=0;
	         bits_io::read_8_bit( avc_packet_type, _fflv_handler );

                //we didn;t care about the composition time,so just skip it
	         fseek( _fflv_handler, 3, SEEK_CUR );

                
	         int temp_length=0;
	         char * tempbuff = NULL; 
	         if ( avc_packet_type == 0)
	        {
	             
		      fseek( _fflv_handler,6,SEEK_CUR );
                   bits_io::read_16_bit(temp_length, _fflv_handler );
		      temp_length = hton16( temp_length );
                   printf("sssize:%d\n",temp_length);
                   
                   tempbuff=(char*)malloc( temp_length );
		      fread(tempbuff,1,temp_length, _fflv_handler );
		      fwrite(&H264_SPACE,1,3, _f264_handler );
		      fwrite(tempbuff,1,temp_length, _f264_handler );
		      free(tempbuff);

                   bits_io::read_8_bit(temp_length, _fflv_handler );//ppsn
                   bits_io::read_16_bit(temp_length, _fflv_handler );//ppssize
                   temp_length = hton16(temp_length);
		      printf("ppsize:%d\n",temp_length);
                   tempbuff=(char*)malloc(temp_length);
		      fread(tempbuff,1,temp_length, _fflv_handler );
		      fwrite(&H264_SPACE,1,3, _f264_handler );
		      fwrite(tempbuff,1,temp_length, _f264_handler );
		      free(tempbuff);

                      /*
		         int configure_version=0;
                      bits_io::read_8_bit( configure_version, _fflv_handler );
                      printf("configure version:0x%x\n",configure_version);
                      //we didn't care about the sps[1] sps[2] sps[3],so just skip 3 bytes
                      fseek( _fflv_handler, 3, SEEK_CUR );

                      int reserved_6bit_and_nal_unit_len = 0;
                      bits_io::read_8_bit( reserved_6bit_and_nal_unit_len , _fflv_handler );
                      printf("reserved_6bit_and_nal_unit_len:0x%x\n", reserved_6bit_and_nal_unit_len );

                      int reserved_and_sps_size = 0;
                      bits_io::read_8_bit( reserved_and_sps_size, _fflv_handler );
                      printf("reserved_and_sps_size:0x%x\n",reserved_and_sps_size);
                      int sps_size = reserved_and_sps_size & 0x1f;
                      printf("sps size:0x%x\n", sps_size);

                      
                      char *sps_buffer=(char*)malloc( sps_size );
                      fread(sps_buffer,1,sps_size, _fflv_handler );
                      fwrite(&H264_SPACE,1,3, _f264_handler );
                      //fwrite(&sps_size,1,1,_f264_handler);
		         fwrite(sps_buffer,1, sps_size, _f264_handler );
		         free(sps_buffer);

                      int pps_size = 0;
                      bits_io::read_8_bit( pps_size, _fflv_handler );
                      printf("pps size:0x%x\n", pps_size);
                      char *pps_buffer = (char *) malloc( pps_size );
                      fread(pps_buffer,1,pps_size, _fflv_handler );
                      fwrite(&H264_SPACE,1,3, _f264_handler );
                      //fwrite(&pps_size,1,1,_f264_handler );
		         fwrite(pps_buffer,1,pps_size, _f264_handler );
		         free(pps_buffer);*/
	        }
              else if( avc_packet_type == 1)
	       {
	              //this is AVC NALU
                    int countsize=2+3;
                    while(countsize< data_size )
                    {
                    	bits_io::read_32_bit(temp_length, _fflv_handler );
                           temp_length = hton32( temp_length );
                    	tempbuff=(char*)malloc( temp_length );
                    	fread(tempbuff,1,temp_length, _fflv_handler );
                    	fwrite(&H264_SPACE,1,3, _f264_handler );
                    	fwrite(tempbuff,1,temp_length, _f264_handler );
                    	free(tempbuff);
                    	countsize+=(temp_length+4);
                    }
	       }
             else
             {
                 ;//not supported
             }
          }
          fseek( _fflv_handler ,cur_pos+data_size+4, SEEK_SET);
     }
}

