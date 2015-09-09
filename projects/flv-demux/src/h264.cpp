#include "h264.h"

int AllocStruct_H264_Tag(Video_Tag ** videotag)
{
	Video_Tag * videotag_t = * videotag;
	if ((videotag_t = (Video_Tag *)calloc(1,sizeof(Video_Tag))) == NULL)
	{
		printf ("Error: Allocate Meory To AllocStruct_H264_Tag Buffer Failed ");
		return getchar();
	} 
	if ((videotag_t->Data = (unsigned char * )calloc(ONE_VIDEO_FRAME_SIZE,sizeof(unsigned char))) == NULL)
	{
		printf ("Error: Allocate Meory To videotag_t->Data Buffer Failed ");
		return getchar();
	}
	if ((videotag_t->video_avcc = (Video_AvcC *)calloc(1,sizeof(Video_AvcC))) == NULL)
	{
		printf ("Error: Allocate Meory To videotag_t->video_avcc Buffer Failed ");
		return getchar();
	} 
	if ((videotag_t->video_avcc->sequenceParameterSetNALUnit = (unsigned char * )calloc(MAX_SPS_FRAME_SIZE,sizeof(unsigned char))) == NULL)
	{
		printf ("Error: Allocate Meory To videotag_t->video_avcc->sequenceParameterSetNALUnit  Buffer Failed ");
		return getchar();
	}
	if ((videotag_t->video_avcc->pictureParameterSetNALUnit = (unsigned char * )calloc(MAX_PPS_FRAME_SIZE,sizeof(unsigned char))) == NULL)
	{
		printf ("Error: Allocate Meory To videotag_t->video_avcc->pictureParameterSetNALUnit  Buffer Failed ");
		return getchar();
	}
	* videotag = videotag_t;
	return 1;
}

int FreeStruct_H264_Tag(Video_Tag * videotag)
{
	if (videotag)
	{
		if (videotag->Data)
		{
			free(videotag->Data);
			videotag->Data = NULL;
		}
		if (videotag->video_avcc)
		{
			if (videotag->video_avcc->pictureParameterSetNALUnit)
			{
				free(videotag->video_avcc->pictureParameterSetNALUnit);
				videotag->video_avcc->pictureParameterSetNALUnit = NULL;
			}
			if (videotag->video_avcc->sequenceParameterSetNALUnit)
			{
				free(videotag->video_avcc->sequenceParameterSetNALUnit);
				videotag->video_avcc->sequenceParameterSetNALUnit = NULL;
			}
			free(videotag->video_avcc);
			videotag->video_avcc = NULL;
		}
		free(videotag);
		videotag = NULL;
	}
	return 1;
}

int ReadStruct_H264_Tag(unsigned char * Buf , unsigned int length ,Video_Tag * tag)
{
	int H264_Tag_pos = 0;
	tag->Type = Buf[0];
	tag->DataSize = 
		Buf[1]  << 16 |
		Buf[2]  << 8  |
		Buf[3];
	tag->Timestamp = 
		Buf[4]  << 16 |
		Buf[5]  << 8  |
		Buf[6];
	tag->TimestampExtended = Buf[7];
	tag->StreamID = 
		Buf[8]  << 16 |
		Buf[9]  << 8  |
		Buf[10];
	H264_Tag_pos += 11;
	tag->FrameType = 
		Buf[H264_Tag_pos] >> 4;
	tag->CodecID  = 
		Buf[H264_Tag_pos] &0x0F;
	H264_Tag_pos ++;
	if (tag->CodecID == 0x07)
	{
		tag->AVCPacketType = Buf[H264_Tag_pos];
		H264_Tag_pos ++;
		tag->CompositionTime = 
			Buf[H264_Tag_pos]     <<  16  |
			Buf[H264_Tag_pos +1]  <<  8   |
			Buf[H264_Tag_pos +2];
		H264_Tag_pos += 3;
		if (tag->AVCPacketType == 0x00) //AVCDecoderConfigurationRecord
		{
			tag->video_avcc->configurationVersion = Buf[H264_Tag_pos];
			H264_Tag_pos ++;
			tag->video_avcc->AVCProfileIndication = Buf[H264_Tag_pos];
			H264_Tag_pos ++;
			tag->video_avcc->profile_compatibility = Buf[H264_Tag_pos];
			H264_Tag_pos ++;
			tag->video_avcc->AVCLevelIndication = Buf[H264_Tag_pos];
			H264_Tag_pos ++;
			tag->video_avcc->reserved_1 = Buf[H264_Tag_pos] >> 2;
			tag->video_avcc->lengthSizeMinusOne = Buf[H264_Tag_pos] & 0x03;
			H264_Tag_pos ++;
			tag->video_avcc->reserved_2 = Buf[H264_Tag_pos] >> 5;
			tag->video_avcc->numOfSequenceParameterSets = Buf[H264_Tag_pos] & 0x1F;
			H264_Tag_pos ++;
            tag->video_avcc->sequenceParameterSetLength = 
				Buf[H264_Tag_pos]    << 8 |
				Buf[H264_Tag_pos+1];
			H264_Tag_pos +=2;
			memcpy(tag->video_avcc->sequenceParameterSetNALUnit ,Buf + H264_Tag_pos,tag->video_avcc->sequenceParameterSetLength);
			H264_Tag_pos += tag->video_avcc->sequenceParameterSetLength;
			tag->video_avcc->numOfPictureParameterSets = Buf[H264_Tag_pos];
			H264_Tag_pos ++;
			tag->video_avcc->pictureParameterSetLength = 
				Buf[H264_Tag_pos]    << 8 |
				Buf[H264_Tag_pos+1];
			H264_Tag_pos +=2;
			memcpy(tag->video_avcc->pictureParameterSetNALUnit,Buf + H264_Tag_pos,tag->video_avcc->pictureParameterSetLength);
			tag->video_avcc->reserved_3 = Buf[H264_Tag_pos] >> 2;
			tag->video_avcc->chroma_format = Buf[H264_Tag_pos] & 0x03;
			H264_Tag_pos ++;
			tag->video_avcc->reserved_4 = Buf[H264_Tag_pos] >> 3;
			tag->video_avcc->bit_depth_luma_minus8 = Buf[H264_Tag_pos] & 0x07;
			H264_Tag_pos ++;
			tag->video_avcc->reserved_5 = Buf[H264_Tag_pos] >> 3;
			tag->video_avcc->bit_depth_chroma_minus8 = Buf[H264_Tag_pos] & 0x07;
			H264_Tag_pos ++;
			tag->video_avcc->numOfSequenceParameterSetExt = Buf[H264_Tag_pos];
			H264_Tag_pos ++;
			//暂时用不到
			//unsigned int sequenceParameterSetExtLength;
			//unsigned char * sequenceParameterSetExtNALUnit;
		}
	}
	memcpy(tag->Data,Buf +H264_Tag_pos,length - H264_Tag_pos );
	return length - H264_Tag_pos;
}