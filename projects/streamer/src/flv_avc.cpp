/*
@author:chenzhengqiang
@start date:2015/9/10
@modified date:
@desc:
*/

#include "flv.h"
#include "flv_avc.h"

//get the h264 from flv video tag
//return -1 indicates failed that it's not the h264 tag type
int get_flv_h264_tag( unsigned char *flv_tag_header,unsigned char * flv_tag_data , unsigned int tag_header_size ,FLV_H264_TAG & h264_tag )
{
	
	h264_tag.Type = flv_tag_header[0];
	h264_tag.DataSize = flv_tag_header[1]  << 16 |flv_tag_header[2]  << 8  |flv_tag_header[3];
	h264_tag.Timestamp = flv_tag_header[4]  << 16 |flv_tag_header[5]  << 8  |flv_tag_header[6];
	h264_tag.TimestampExtended = flv_tag_header[7];
	h264_tag.StreamID = flv_tag_header[8]  << 16 |flv_tag_header[9]  << 8  | flv_tag_header[10];
    
	int pos = 0;
	h264_tag.FrameType = 
		flv_tag_data[pos] >> 4;
	h264_tag.CodecID  = 
		flv_tag_data[pos] &0x0F;
	pos ++;
	if (h264_tag.CodecID == MEDIA_TYPE_AVC )
	{
		h264_tag.AVCPacketType = flv_tag_data[pos];
		pos ++;
		h264_tag.CompositionTime = flv_tag_data[pos]     <<  16  |flv_tag_data[pos +1]  <<  8   |flv_tag_data[pos +2];
		pos += 3;
		if (h264_tag.AVCPacketType == 0x00) //AVCDecoderConfigurationRecord
		{
			h264_tag.adcr.configurationVersion = flv_tag_data[pos];
			pos ++;
			h264_tag.adcr.AVCProfileIndication = flv_tag_data[pos];
			pos ++;
			h264_tag.adcr.profile_compatibility = flv_tag_data[pos];
			pos ++;
			h264_tag.adcr.AVCLevelIndication = flv_tag_data[pos];
			pos ++;
			h264_tag.adcr.reserved_1 = flv_tag_data[pos] >> 2;
			h264_tag.adcr.lengthSizeMinusOne = flv_tag_data[pos] & 0x03;
			pos ++;
			h264_tag.adcr.reserved_2 = flv_tag_data[pos] >> 5;
			h264_tag.adcr.numOfSequenceParameterSets = flv_tag_data[pos] & 0x1F;
			pos ++;
                    h264_tag.adcr.sequenceParameterSetLength = flv_tag_data[pos]    << 8 |flv_tag_data[pos+1];
			pos +=2;
			memcpy(h264_tag.adcr.sequenceParameterSetNALUnit ,flv_tag_data + pos,h264_tag.adcr.sequenceParameterSetLength);
			pos += h264_tag.adcr.sequenceParameterSetLength;
			h264_tag.adcr.numOfPictureParameterSets = flv_tag_data[pos];
			pos ++;
			h264_tag.adcr.pictureParameterSetLength = flv_tag_data[pos]    << 8 | flv_tag_data[pos+1];
			pos +=2;
			memcpy(h264_tag.adcr.pictureParameterSetNALUnit,flv_tag_data + pos,h264_tag.adcr.pictureParameterSetLength);
			h264_tag.adcr.reserved_3 = flv_tag_data[pos] >> 2;
			h264_tag.adcr.chroma_format = flv_tag_data[pos] & 0x03;
			pos ++;
			h264_tag.adcr.reserved_4 = flv_tag_data[pos] >> 3;
			h264_tag.adcr.bit_depth_luma_minus8 = flv_tag_data[pos] & 0x07;
			pos ++;
			h264_tag.adcr.reserved_5 = flv_tag_data[pos] >> 3;
			h264_tag.adcr.bit_depth_chroma_minus8 = flv_tag_data[pos] & 0x07;
			pos ++;
			h264_tag.adcr.numOfSequenceParameterSetExt = flv_tag_data[pos];
			pos ++;
		}
	}
       else
       {
            return -1;
       }
	memcpy(h264_tag.Payload,flv_tag_data +pos,tag_header_size - pos );
	return tag_header_size - pos;
}