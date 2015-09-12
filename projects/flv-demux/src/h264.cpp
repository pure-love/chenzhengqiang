/*
@author:chenzhengqiang
@start date:2015/9/10
@modified date:
@desc:
*/

#include "h264.h"
int get_flv_h264_tag( unsigned char *flv_tag_header,unsigned char * flv_tag_data , unsigned int tag_header_size ,FLV_H264_TAG & h264_tag )
{
	
	h264_tag.Type = flv_tag_header[0];
	h264_tag.DataSize = 
		flv_tag_header[1]  << 16 |
		flv_tag_header[2]  << 8  |
		flv_tag_header[3];
	h264_tag.Timestamp = 
		flv_tag_header[4]  << 16 |
		flv_tag_header[5]  << 8  |
		flv_tag_header[6];
	h264_tag.TimestampExtended = flv_tag_header[7];
	h264_tag.StreamID = 
		flv_tag_header[8]  << 16 |
		flv_tag_header[9]  << 8  |
		flv_tag_header[10];
    
	int H264_Tag_pos = 0;
	h264_tag.FrameType = 
		flv_tag_data[H264_Tag_pos] >> 4;
	h264_tag.CodecID  = 
		flv_tag_data[H264_Tag_pos] &0x0F;
	H264_Tag_pos ++;
	if (h264_tag.CodecID == 0x07)
	{
		h264_tag.AVCPacketType = flv_tag_data[H264_Tag_pos];
		H264_Tag_pos ++;
		h264_tag.CompositionTime = 
			flv_tag_data[H264_Tag_pos]     <<  16  |
			flv_tag_data[H264_Tag_pos +1]  <<  8   |
			flv_tag_data[H264_Tag_pos +2];
		H264_Tag_pos += 3;
		if (h264_tag.AVCPacketType == 0x00) //AVCDecoderConfigurationRecord
		{
			h264_tag.video_avcc.configurationVersion = flv_tag_data[H264_Tag_pos];
			H264_Tag_pos ++;
			h264_tag.video_avcc.AVCProfileIndication = flv_tag_data[H264_Tag_pos];
			H264_Tag_pos ++;
			h264_tag.video_avcc.profile_compatibility = flv_tag_data[H264_Tag_pos];
			H264_Tag_pos ++;
			h264_tag.video_avcc.AVCLevelIndication = flv_tag_data[H264_Tag_pos];
			H264_Tag_pos ++;
			h264_tag.video_avcc.reserved_1 = flv_tag_data[H264_Tag_pos] >> 2;
			h264_tag.video_avcc.lengthSizeMinusOne = flv_tag_data[H264_Tag_pos] & 0x03;
			H264_Tag_pos ++;
			h264_tag.video_avcc.reserved_2 = flv_tag_data[H264_Tag_pos] >> 5;
			h264_tag.video_avcc.numOfSequenceParameterSets = flv_tag_data[H264_Tag_pos] & 0x1F;
			H264_Tag_pos ++;
            h264_tag.video_avcc.sequenceParameterSetLength = 
				flv_tag_data[H264_Tag_pos]    << 8 |
				flv_tag_data[H264_Tag_pos+1];
			H264_Tag_pos +=2;
			memcpy(h264_tag.video_avcc.sequenceParameterSetNALUnit ,flv_tag_data + H264_Tag_pos,h264_tag.video_avcc.sequenceParameterSetLength);
			H264_Tag_pos += h264_tag.video_avcc.sequenceParameterSetLength;
			h264_tag.video_avcc.numOfPictureParameterSets = flv_tag_data[H264_Tag_pos];
			H264_Tag_pos ++;
			h264_tag.video_avcc.pictureParameterSetLength = 
				flv_tag_data[H264_Tag_pos]    << 8 |
				flv_tag_data[H264_Tag_pos+1];
			H264_Tag_pos +=2;
			memcpy(h264_tag.video_avcc.pictureParameterSetNALUnit,flv_tag_data + H264_Tag_pos,h264_tag.video_avcc.pictureParameterSetLength);
			h264_tag.video_avcc.reserved_3 = flv_tag_data[H264_Tag_pos] >> 2;
			h264_tag.video_avcc.chroma_format = flv_tag_data[H264_Tag_pos] & 0x03;
			H264_Tag_pos ++;
			h264_tag.video_avcc.reserved_4 = flv_tag_data[H264_Tag_pos] >> 3;
			h264_tag.video_avcc.bit_depth_luma_minus8 = flv_tag_data[H264_Tag_pos] & 0x07;
			H264_Tag_pos ++;
			h264_tag.video_avcc.reserved_5 = flv_tag_data[H264_Tag_pos] >> 3;
			h264_tag.video_avcc.bit_depth_chroma_minus8 = flv_tag_data[H264_Tag_pos] & 0x07;
			H264_Tag_pos ++;
			h264_tag.video_avcc.numOfSequenceParameterSetExt = flv_tag_data[H264_Tag_pos];
			H264_Tag_pos ++;
		}
	}
	memcpy(h264_tag.Data,flv_tag_data +H264_Tag_pos,tag_header_size - H264_Tag_pos );
	return tag_header_size - H264_Tag_pos;
}