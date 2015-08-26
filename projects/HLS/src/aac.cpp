/*
@author:chenzhengqiang
@version:1.0
@start date:2015/8/26
@modified date:
@desc:providing the api of handling aac:obtain the adts header,read a frame from aac file
*/

#include "errors.h"
#include "aac.h"
#include <cstdio>
#include <cstring>


/*
@returns:0 indicates ok,others see the errors.h
@desc:read the adts header from aac file,and then saving the adts data to ADTS_HEADER
*/
int obtain_aac_adts_header( FILE *faac_handler, ADTS_HEADER & adts_header, unsigned char *adts_header_buffer )
{
       if( faac_handler == NULL  )
       return ARGUMENT_ERROR;

	size_t read_bytes = 0;
	read_bytes = fread(adts_header_buffer,sizeof(unsigned char),ADTS_HEADER_LENGTH,faac_handler);
	if (read_bytes == 0)
	{
	      return FILE_EOF;
	}
	
	if ((adts_header_buffer[0] == 0xFF)&&((adts_header_buffer[1] & 0xF0) == 0xF0))    //syncword 12个1
	{
		adts_header.syncword = (adts_header_buffer[0] << 4 )  | (adts_header_buffer[1]  >> 4);
		adts_header.id = ((unsigned int) adts_header_buffer[1] & 0x08) >> 3;
		adts_header.layer = ((unsigned int) adts_header_buffer[1] & 0x06) >> 1;
		adts_header.protection_absent = (unsigned int) adts_header_buffer[1] & 0x01;
		adts_header.profile = ((unsigned int) adts_header_buffer[2] & 0xc0) >> 6;
		adts_header.sf_index = ((unsigned int) adts_header_buffer[2] & 0x3c) >> 2;
		adts_header.private_bit = ((unsigned int) adts_header_buffer[2] & 0x02) >> 1;
		adts_header.channel_configuration = ((((unsigned int) adts_header_buffer[2] & 0x01) << 2) | (((unsigned int) adts_header_buffer[3] & 0xc0) >> 6));
		adts_header.original = ((unsigned int) adts_header_buffer[3] & 0x20) >> 5;
		adts_header.home = ((unsigned int) adts_header_buffer[3] & 0x10) >> 4;
		adts_header.copyright_identification_bit = ((unsigned int) adts_header_buffer[3] & 0x08) >> 3;
		adts_header.copyright_identification_start = (unsigned int) adts_header_buffer[3] & 0x04 >> 2;		
		adts_header.aac_frame_length = (((((unsigned int) adts_header_buffer[3]) & 0x03) << 11) | (((unsigned int) adts_header_buffer[4] & 0xFF) << 3)| ((unsigned int) adts_header_buffer[5] & 0xE0) >> 5) ;
		adts_header.adts_buffer_fullness = (((unsigned int) adts_header_buffer[5] & 0x1f) << 6 | ((unsigned int) adts_header_buffer[6] & 0xfc) >> 2);
		adts_header.no_raw_data_blocks_in_frame = ((unsigned int) adts_header_buffer[6] & 0x03);
	}
	else 
	{
		return FILE_FORMAT_ERROR;
	}
	return OK;
}


/*
@returns:0 indicates ok,others see the errors.h
@desc:read a frame from aac file,then save the data to aac frame buffer
*/
int read_aac_frame( FILE *faac_handler,unsigned char * aac_frame ,unsigned int & frame_length )
{
	ADTS_HEADER  adts_header ;
	unsigned int read_bytes = 0;

	//read the adts header,then we know the frame length
	int ret = obtain_aac_adts_header( faac_handler,adts_header,aac_frame );
       if( ret != OK )
	{
		return ret;
	}
       
	//fill the data into aac_frame
	read_bytes = fread(aac_frame+ADTS_HEADER_LENGTH,sizeof(unsigned char),
	                             adts_header.aac_frame_length - ADTS_HEADER_LENGTH, faac_handler );
    
	if (read_bytes != adts_header.aac_frame_length - ADTS_HEADER_LENGTH)
	{
		return FILE_LENGTH_ERROR;
	}

       frame_length = adts_header.aac_frame_length;
	return OK;
}



/*
@returns:void
@desc:encapsulate pes with aac frame buffer and the display timestamp
*/
void aac_frame_2_pes( unsigned char *aac_frame, unsigned int frame_length, unsigned long aac_pts,TsPes  & aac_pes )
{
       
       memcpy(aac_pes.Es,aac_frame,frame_length );
	unsigned int aacpes_pos = 0;
	aacpes_pos += frame_length ;

	aac_pes.packet_start_code_prefix = 0x000001;
	aac_pes.stream_id = TS_AAC_STREAM_ID;                                //E0~EF表示是视频的,C0~DF是音频,H264-- E0
	aac_pes.PES_packet_length = 0 ; // frame_length + 8 ;             //一帧数据的长度 不包含 PES包头 ,8自适应段的长度
	aac_pes.Pes_Packet_Length_Beyond = frame_length;                  //= OneFrameLen_aac;     //这里读错了一帧  
	if (frame_length > 0xFFFF)                                          //如果一帧数据的大小超出界限
	{
		aac_pes.PES_packet_length = 0x00;
		aac_pes.Pes_Packet_Length_Beyond = frame_length;  
		aacpes_pos += 16;
	}
	else
	{
		aac_pes.PES_packet_length = 0x00;
		aac_pes.Pes_Packet_Length_Beyond = frame_length;  
		aacpes_pos += 14;
	}
	aac_pes.marker_bit = 0x02;
	aac_pes.PES_scrambling_control = 0x00;                               //人选字段 存在，不加扰
	aac_pes.PES_priority = 0x00;
	aac_pes.data_alignment_indicator = 0x00;
	aac_pes.copyright = 0x00;
	aac_pes.original_or_copy = 0x00;
	aac_pes.PTS_DTS_flags = 0x02;                                        //10'：PTS字段存在,DTS不存在
	aac_pes.ESCR_flag = 0x00;
	aac_pes.ES_rate_flag = 0x00;
	aac_pes.DSM_trick_mode_flag = 0x00;
	aac_pes.additional_copy_info_flag = 0x00;
	aac_pes.PES_CRC_flag = 0x00;
	aac_pes.PES_extension_flag = 0x00;
	aac_pes.PES_header_data_length = 0x05;                               //后面的数据 包括了PTS所占的字节数

	aac_pes.tsptsdts.pts_32_30  = 0;
	aac_pes.tsptsdts.pts_29_15 = 0;
	aac_pes.tsptsdts.pts_14_0 = 0;

	aac_pes.tsptsdts.reserved_1 = 0x03;                                 //填写 pts信息
	
	
	//if aac frame's pts greater than 30 bit,then use the higest 3 bit
	if( aac_pts > 0x7FFFFFFF )
	{
		aac_pes.tsptsdts.pts_32_30 = (aac_pts >> 30) & 0x07;                 
		aac_pes.tsptsdts.marker_bit1 = 0x01;
	}
	else 
	{
		aac_pes.tsptsdts.marker_bit1 = 0;
	}
	// if greater than 15bit,then use more bit to save the pts
	if( aac_pts > 0x7FFF)
	{
		aac_pes.tsptsdts.pts_29_15 = (aac_pts >> 15) & 0x007FFF ;
		aac_pes.tsptsdts.marker_bit2 = 0x01;
	}
	else
	{
		aac_pes.tsptsdts.marker_bit2 = 0;
	}
    
	//use the last 15 bit
	aac_pes.tsptsdts.pts_14_0 = aac_pts & 0x007FFF;
	aac_pes.tsptsdts.marker_bit3 = 0x01;
}


//@desc:as the function name described
//obtain the aac's samplerate
//@returns:the aac file's sample rate
int   obtain_aac_file_samplerate( const char * aac_file )
{
       if( aac_file == NULL  )
       return ARGUMENT_ERROR;

       unsigned char adts_header_buffer[ADTS_HEADER_LENGTH];
	size_t read_bytes = 0;

       FILE * faac_handler = fopen( aac_file, "r");
       if( faac_handler == NULL )
       {
           return ARGUMENT_ERROR;
       }
       
	read_bytes = fread(adts_header_buffer,sizeof(unsigned char),ADTS_HEADER_LENGTH,faac_handler);
	if (read_bytes == 0)
	{
	      return FILE_EOF;
	}

       int sampling_frequency_index;
	if ((adts_header_buffer[0] == 0xFF)&&((adts_header_buffer[1] & 0xF0) == 0xF0))    //syncword 12个1
	{
		
		sampling_frequency_index = (adts_header_buffer[2] & 0x3c) >> 2;
             if( sampling_frequency_index < 0 || sampling_frequency_index > MAX_AAC_SAMPLERATE_INDEX )
             return FILE_FORMAT_ERROR;   
	}
	else 
	{
		return FILE_FORMAT_ERROR;
	}

       if( faac_handler )
       fclose(faac_handler);
       faac_handler = NULL;
	return AAC_SAMPLERATES[sampling_frequency_index];
}

