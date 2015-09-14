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
	if ((adts_header_buffer[0] == 0xFF)&&((adts_header_buffer[1] & 0xF0) == 0xF0))    //syncword 12¸ö1
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

