/*
@author:chenzhengqiang
@start date:2015/9/10
@modified date:
@desc:
*/

#include "h264.h"
int read_flv_h264_tag(unsigned char * video_tag_buffer , unsigned int length ,FLV_H264_TAG & h264_tag )
{
	int H264_Tag_pos = 0;
	h264_tag.Type = video_tag_buffer[0];
	h264_tag.DataSize = 
		video_tag_buffer[1]  << 16 |
		video_tag_buffer[2]  << 8  |
		video_tag_buffer[3];
	h264_tag.Timestamp = 
		video_tag_buffer[4]  << 16 |
		video_tag_buffer[5]  << 8  |
		video_tag_buffer[6];
	h264_tag.TimestampExtended = video_tag_buffer[7];
	h264_tag.StreamID = 
		video_tag_buffer[8]  << 16 |
		video_tag_buffer[9]  << 8  |
		video_tag_buffer[10];
	H264_Tag_pos += 11;
	h264_tag.FrameType = 
		video_tag_buffer[H264_Tag_pos] >> 4;
	h264_tag.CodecID  = 
		video_tag_buffer[H264_Tag_pos] &0x0F;
	H264_Tag_pos ++;
	if (h264_tag.CodecID == 0x07)
	{
		h264_tag.AVCPacketType = video_tag_buffer[H264_Tag_pos];
		H264_Tag_pos ++;
		h264_tag.CompositionTime = 
			video_tag_buffer[H264_Tag_pos]     <<  16  |
			video_tag_buffer[H264_Tag_pos +1]  <<  8   |
			video_tag_buffer[H264_Tag_pos +2];
		H264_Tag_pos += 3;
		if (h264_tag.AVCPacketType == 0x00) //AVCDecoderConfigurationRecord
		{
			h264_tag.video_avcc.configurationVersion = video_tag_buffer[H264_Tag_pos];
			H264_Tag_pos ++;
			h264_tag.video_avcc.AVCProfileIndication = video_tag_buffer[H264_Tag_pos];
			H264_Tag_pos ++;
			h264_tag.video_avcc.profile_compatibility = video_tag_buffer[H264_Tag_pos];
			H264_Tag_pos ++;
			h264_tag.video_avcc.AVCLevelIndication = video_tag_buffer[H264_Tag_pos];
			H264_Tag_pos ++;
			h264_tag.video_avcc.reserved_1 = video_tag_buffer[H264_Tag_pos] >> 2;
			h264_tag.video_avcc.lengthSizeMinusOne = video_tag_buffer[H264_Tag_pos] & 0x03;
			H264_Tag_pos ++;
			h264_tag.video_avcc.reserved_2 = video_tag_buffer[H264_Tag_pos] >> 5;
			h264_tag.video_avcc.numOfSequenceParameterSets = video_tag_buffer[H264_Tag_pos] & 0x1F;
			H264_Tag_pos ++;
            h264_tag.video_avcc.sequenceParameterSetLength = 
				video_tag_buffer[H264_Tag_pos]    << 8 |
				video_tag_buffer[H264_Tag_pos+1];
			H264_Tag_pos +=2;
			memcpy(h264_tag.video_avcc.sequenceParameterSetNALUnit ,video_tag_buffer + H264_Tag_pos,h264_tag.video_avcc.sequenceParameterSetLength);
			H264_Tag_pos += h264_tag.video_avcc.sequenceParameterSetLength;
			h264_tag.video_avcc.numOfPictureParameterSets = video_tag_buffer[H264_Tag_pos];
			H264_Tag_pos ++;
			h264_tag.video_avcc.pictureParameterSetLength = 
				video_tag_buffer[H264_Tag_pos]    << 8 |
				video_tag_buffer[H264_Tag_pos+1];
			H264_Tag_pos +=2;
			memcpy(h264_tag.video_avcc.pictureParameterSetNALUnit,video_tag_buffer + H264_Tag_pos,h264_tag.video_avcc.pictureParameterSetLength);
			h264_tag.video_avcc.reserved_3 = video_tag_buffer[H264_Tag_pos] >> 2;
			h264_tag.video_avcc.chroma_format = video_tag_buffer[H264_Tag_pos] & 0x03;
			H264_Tag_pos ++;
			h264_tag.video_avcc.reserved_4 = video_tag_buffer[H264_Tag_pos] >> 3;
			h264_tag.video_avcc.bit_depth_luma_minus8 = video_tag_buffer[H264_Tag_pos] & 0x07;
			H264_Tag_pos ++;
			h264_tag.video_avcc.reserved_5 = video_tag_buffer[H264_Tag_pos] >> 3;
			h264_tag.video_avcc.bit_depth_chroma_minus8 = video_tag_buffer[H264_Tag_pos] & 0x07;
			H264_Tag_pos ++;
			h264_tag.video_avcc.numOfSequenceParameterSetExt = video_tag_buffer[H264_Tag_pos];
			H264_Tag_pos ++;
			//暂时用不到
			//unsigned int sequenceParameterSetExtLength;
			//unsigned char * sequenceParameterSetExtNALUnit;
		}
	}
	memcpy(h264_tag.Data,video_tag_buffer +H264_Tag_pos,length - H264_Tag_pos );
	return length - H264_Tag_pos;
}