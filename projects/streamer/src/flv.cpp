/*
@author:chenzhengqiang
@start date:2015/9/10
@modified date:
@desc:
*/
#include "errors.h"
#include "flv.h"
int read_flv_header(unsigned char * flv_header_buffer , unsigned int stream_length ,FLV_HEADER & flv_header )
{
	unsigned int header_pos = 0;
	flv_header.Signature_1 = flv_header_buffer[header_pos];
	flv_header.Signature_2 = flv_header_buffer[header_pos +1];
	flv_header.Signature_3 = flv_header_buffer[header_pos +2];
	flv_header.version = flv_header_buffer[header_pos +3];
	header_pos +=4;
       flv_header.TypeFlagsReserved_1 = flv_header_buffer[header_pos] >> 3;
	flv_header.TypeFlagsAudio = ( flv_header_buffer[header_pos] >> 2) & 0x01;
	flv_header.TypeFlagsReserved_2 = flv_header_buffer[header_pos] & 0x02;
	flv_header.TypeFlagsVideo = flv_header_buffer[header_pos] & 0x01;
	header_pos ++;
       flv_header.DataOffset = flv_header_buffer[header_pos]     << 24  |
		                              flv_header_buffer[header_pos +1]  << 16 |
		                              flv_header_buffer[header_pos +2]  << 8  |
		                              flv_header_buffer[header_pos +3];
	header_pos +=4;
	if (stream_length != header_pos )
	{
		return STREAM_LENGTH_ERROR;
	}
	return OK;
}