/*
@author:chenzhengqiang
@start date:2015/8/25
@modified date:
@desc:handle the aac file,providing apis like the following:
 obtain the adts header either read a aac frame from aac file 
*/

#include "errors.h"
#include "aac.h"
#include<cstdio>
#include<cstring>

/*
@args:faac_handler[in],adts_header[out]
@returns:0 indicates ok,others see the errors.h
@desc:read the adts header from aac file
*/
int obtain_aac_adts_header( FILE *faac_handler, ADTS_HEADER & adts_header )
{
       unsigned char adts_header_buffer[ADTS_HEADER_LENGTH];
       if( faac_handler == NULL )
       return ARGUMENT_ERROR;
       
	unsigned int readsize = 0;
	readsize = fread(adts_header_buffer,sizeof(unsigned char),ADTS_HEADER_LENGTH,faac_handler);
	
	if (readsize == 0)
	{
		return FILE_EOF;
	}
	if ((adts_header_buffer[0] == 0xFF)&&((adts_header_buffer[1] & 0xF0) == 0xF0))    //syncword 12¸ö1
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
@args:faac_handler[in],frame_buffer[out]
@returns:0 indicates ok,others see the erros.h
@desc:read the adts header from aac file ,then we know the aac frame's length.
 as we alrerady know the frame's length,then just call the fread
*/
int read_aac_frame( FILE *faac_handler,unsigned char * aac_frame, unsigned int & frame_length )
{
       if( faac_handler == NULL || aac_frame == NULL )
       return ARGUMENT_ERROR;
       
	ADTS_HEADER  adts_header ;
	size_t read_bytes = 0;

	int ret = obtain_aac_adts_header(faac_handler,adts_header);
	if (  ret != OK )
	{
	    return ret;
	}
    
	read_bytes= fread(aac_frame+ADTS_HEADER_LENGTH,sizeof(unsigned char),adts_header.aac_frame_length - ADTS_HEADER_LENGTH,faac_handler);
	if ( read_bytes != adts_header.aac_frame_length - ADTS_HEADER_LENGTH )
	{
	     return FILE_LENGTH_ERROR;
	}

       frame_length = adts_header.aac_frame_length;
	return OK;
}

