/*
@author:chenzhengqaing
@start date:2015/9/10
@modified date:
@desc:
*/


#include "common.h"
#include "flv_aac.h"
#include "flv.h"
/*
@returns:true indicates the audio tag is aac,false otherwise
@desc:parse the flv tag header,and save the fields related into FLV_AAC_TAG
*/
int get_flv_aac_tag(unsigned char *flv_tag_header_buffer,unsigned char * flv_tag_data,unsigned int tag_data_size ,FLV_AAC_TAG & aac_tag )
{
       //flv tag header,it always hold 11 bytes
	aac_tag.Type = flv_tag_header_buffer[0];
	aac_tag.DataSize = tag_data_size;
	aac_tag.Timestamp = flv_tag_header_buffer[4]  << 16 |flv_tag_header_buffer[5]  << 8  |flv_tag_header_buffer[6];
	aac_tag.TimestampExtended = flv_tag_header_buffer[7];
	aac_tag.StreamID = flv_tag_header_buffer[8]  << 16 |flv_tag_header_buffer[9]  << 8  |flv_tag_header_buffer[10];
    
	int pos = 0;	
       aac_tag.SoundFormat = flv_tag_data[pos] >> 4;
	aac_tag.SoundRate = (flv_tag_data[pos] >> 2) & 0x03;
	aac_tag.SoundSize = (flv_tag_data[pos] >> 1) & 0x01;
	aac_tag.SoundType = flv_tag_data[pos] & 0x01;
	pos ++;
    
	if (aac_tag.SoundFormat == MEDIA_TYPE_AAC ) 
	{
		aac_tag.AACPacketType = flv_tag_data[pos];
		pos ++;
	}
       else
       {
            return -1;
       }
	if(aac_tag.AACPacketType == AAC_SEQUENCE_HEADER_TYPE )
	{
		
		aac_tag.adts.audioObjectType = (flv_tag_data[pos] >> 3);
		aac_tag.adts.samplingFrequencyIndex = ((flv_tag_data[pos] & 0x07)  << 1)  | ((flv_tag_data[pos + 1]) >> 7 );
		pos ++;
		aac_tag.adts.channelConfiguration = (flv_tag_data[pos] >> 3)  & 0x0F;
	}
	memcpy(aac_tag.Payload,flv_tag_data + pos,tag_data_size- pos);
	return tag_data_size - pos;
}