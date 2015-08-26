#include "VideoTag.h"

unsigned int I_Frame_Num = 0;
unsigned int decode_video_done = 0;

unsigned int  Create_AVCDecoderConfigurationRecord(unsigned char * buf,unsigned char * spsbuf,unsigned int spslength,unsigned char * ppsbuf,unsigned int ppslength)
{
	unsigned int avcc_pos = 0;
	Video_AvcC videoavcc;
	if ((videoavcc.sequenceParameterSetNALUnit = (unsigned char * )calloc(1024,sizeof(unsigned char))) == NULL)
	{
		printf("alloc videoavcc.sequenceParameterSetNALUnit error\n");
		return getchar();
	}
	if ((videoavcc.pictureParameterSetNALUnit = (unsigned char * )calloc(1024,sizeof(unsigned char))) == NULL)
	{
		printf("alloc videoavcc.pictureParameterSetNALUnit error\n");
		return getchar();
	}

	videoavcc.configurationVersion = 0x01;
	videoavcc.AVCProfileIndication = spsbuf[1];
	videoavcc.profile_compatibility = spsbuf[2];
	videoavcc.AVCLevelIndication = spsbuf[3];
	videoavcc.reserved_1 = 0x3F;
	videoavcc.lengthSizeMinusOne = 0x03;
	videoavcc.reserved_2 = 0x07;
	videoavcc.numOfSequenceParameterSets = 0x01;
	videoavcc.sequenceParameterSetLength = spslength;
	memcpy(videoavcc.sequenceParameterSetNALUnit,spsbuf,spslength);
	videoavcc.numOfPictureParameterSets = 0x01;
	videoavcc.pictureParameterSetLength = ppslength;
	memcpy(videoavcc.pictureParameterSetNALUnit,ppsbuf,ppslength);
	//下面的按照官方文档可以扩展，结构体 。h文件有定义

	buf[0] = videoavcc.configurationVersion;
	buf[1] = videoavcc.AVCProfileIndication;
	buf[2] = videoavcc.profile_compatibility;
	buf[3] = videoavcc.AVCLevelIndication;
	buf[4] = ((videoavcc.reserved_1) << 2 ) | videoavcc.lengthSizeMinusOne;
	buf[5] = ((videoavcc.reserved_2) << 5)  | videoavcc.numOfSequenceParameterSets;
	buf[6] = videoavcc.sequenceParameterSetLength >> 8;
	buf[7] = videoavcc.sequenceParameterSetLength & 0xFF;
	avcc_pos += 8;
	memcpy(buf + avcc_pos,videoavcc.sequenceParameterSetNALUnit,spslength);
	avcc_pos += spslength;
	buf[avcc_pos] = videoavcc.numOfPictureParameterSets;
	avcc_pos ++;
	buf[avcc_pos] = videoavcc.pictureParameterSetLength >> 8;
	buf[avcc_pos +1] = videoavcc.pictureParameterSetLength & 0xFF;
	avcc_pos += 2;
	memcpy(buf + avcc_pos,videoavcc.pictureParameterSetNALUnit,ppslength);
	avcc_pos += ppslength;
	if (videoavcc.sequenceParameterSetNALUnit)
	{
		free(videoavcc.sequenceParameterSetNALUnit);
		videoavcc.sequenceParameterSetNALUnit = NULL;
	}
	if (videoavcc.pictureParameterSetNALUnit)
	{
		free(videoavcc.pictureParameterSetNALUnit);
		videoavcc.pictureParameterSetNALUnit = NULL;
	}

	return avcc_pos ;
}

NALU_t *AllocNALU(int buffersize)
{
	NALU_t *n;

	if ((n = (NALU_t*)calloc (1, sizeof(NALU_t))) == NULL)
	{
		printf("AllocNALU Error: Allocate Meory To NALU_t Failed ");
		getchar();
	}

	n->max_size = buffersize;									//Assign buffer size 

	if ((n->buf = (unsigned char*)calloc (buffersize, sizeof (char))) == NULL)
	{
		free (n);
		printf ("AllocNALU Error: Allocate Meory To NALU_t Buffer Failed ");
		getchar();
	}
	return n;
}

void FreeNALU(NALU_t *n)
{
	if (n)
	{
		if (n->buf)
		{
			free(n->buf);
			n->buf=NULL;
		}
		free (n);
	}
}

int FindStartCode2 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1)               //Check whether buf is 0x000001
	{
		return 0;
	}
	else 
	{
		return 1;
	}
}

int FindStartCode3 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1)  //Check whether buf is 0x00000001
	{
		return 0;
	}
	else 
	{
		return 1;
	}
}

int GetAnnexbNALU (NALU_t * nalu)
{
	int pos = 0;                  //一个nal到下一个nal 数据移动的指针
	int StartCodeFound  = 0;      //是否找到下一个nal 的前缀
	int rewind = 0;               //判断 前缀所占字节数 3或 4
	unsigned char * Buf = NULL;
	static int info2 =0 ;
	static int info3 =0 ;

	if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL) 
	{
		printf ("GetAnnexbNALU Error: Could not allocate Buf memory\n");
	}

	nalu->startcodeprefix_len = 3;      //初始化前缀位三个字节

	if (3 != fread (Buf, 1, 3, pVideo_H264_File))//从文件读取三个字节到buf
	{
		free(Buf);
		return 0;
	}
	info2 = FindStartCode2 (Buf);       //Check whether Buf is 0x000001
	if(info2 != 1) 
	{
		//If Buf is not 0x000001,then read one more byte
		if(1 != fread(Buf + 3, 1, 1, pVideo_H264_File))
		{
			free(Buf);
			return 0;
		}
		info3 = FindStartCode3 (Buf);   //Check whether Buf is 0x00000001
		if (info3 != 1)                 //If not the return -1
		{ 
			free(Buf);
			return -1;
		}
		else 
		{
			//If Buf is 0x00000001,set the prefix length to 4 bytes
			pos = 4;
			nalu->startcodeprefix_len = 4;
		}
	} 
	else
	{
		//If Buf is 0x000001,set the prefix length to 3 bytes
		pos = 3;
		nalu->startcodeprefix_len = 3;
	}
	//寻找下一个字符符号位， 即 寻找一个nal 从一个0000001 到下一个00000001
	StartCodeFound = 0;
	info2 = 0;
	info3 = 0;
	while (!StartCodeFound)
	{
		if (feof (pVideo_H264_File))                                 //如果到了文件结尾
		{
			nalu->len = (pos-1) - nalu->startcodeprefix_len;  //从0 开始
			memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);     
			nalu->forbidden_bit = nalu->buf[0] & 0x80;      // 1 bit--10000000
			nalu->nal_reference_idc = nalu->buf[0] & 0x60;  // 2 bit--01100000
			nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;    // 5 bit--00011111
			free(Buf);
			return ((info3 == 1)? 4 : 3);
		}
		Buf[pos++] = fgetc (pVideo_H264_File);                       //Read one char to the Buffer 一个字节一个字节从文件向后找
		info3 = FindStartCode3(&Buf[pos-4]);		        //Check whether Buf is 0x00000001 
		if(info3 != 1)
		{
			info2 = FindStartCode2(&Buf[pos-3]);            //Check whether Buf is 0x000001
		}
		StartCodeFound = (info2 == 1 || info3 == 1);        //如果找到下一个前缀
	}

	rewind = (info3 == 1)? -4 : -3;

	if (0 != fseek (pVideo_H264_File, rewind, SEEK_CUR))			    //将文件内部指针移动到 nal 的末尾
	{
		free(Buf);
		printf("GetAnnexbNALU Error: Cannot fseek in the bit stream file");
	}

	nalu->len = (pos + rewind) -  nalu->startcodeprefix_len;       //设置包含nal 头的数据长度
	memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);//拷贝一个nal 数据到数组中
	nalu->forbidden_bit = nalu->buf[0] & 0x80;                     //1 bit  设置nal 头
	nalu->nal_reference_idc = nalu->buf[0] & 0x60;                 // 2 bit
	nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;                   // 5 bit
	free(Buf);
	return ((info3 == 1)? 4 : 3);                                               
}

int GetFrameType(NALU_t * nal)
{
	bs_t s;
	int frame_type = 0; 
	unsigned char * OneFrameBuf_H264 = NULL ;
	if ((OneFrameBuf_H264 = (unsigned char *)calloc(nal->len + 4,sizeof(unsigned char))) == NULL)
	{
		printf("Error malloc OneFrameBuf_H264\n");
		return getchar();
	}
	if (nal->startcodeprefix_len == 3)
	{
		OneFrameBuf_H264[0] = 0x00;
		OneFrameBuf_H264[1] = 0x00;
		OneFrameBuf_H264[2] = 0x01;
		memcpy(OneFrameBuf_H264 + 3,nal->buf,nal->len);
	}
	else if (nal->startcodeprefix_len == 4)
	{
		OneFrameBuf_H264[0] = 0x00;
		OneFrameBuf_H264[1] = 0x00;
		OneFrameBuf_H264[2] = 0x00;
		OneFrameBuf_H264[3] = 0x01;
		memcpy(OneFrameBuf_H264 + 4,nal->buf,nal->len);
	}
	else
	{
		printf("H264读取错误！\n");
	}
	bs_init( &s,OneFrameBuf_H264 + nal->startcodeprefix_len + 1  ,nal->len - 1 );


	if (nal->nal_unit_type == NAL_SLICE || nal->nal_unit_type ==  NAL_SLICE_IDR )
	{
		/* i_first_mb */
		bs_read_ue( &s );
		/* picture type */
		frame_type =  bs_read_ue( &s );
		switch(frame_type)
		{
		case 0: case 5: /* P */
			nal->Frametype = FRAME_P;
			break;
		case 1: case 6: /* B */
			nal->Frametype = FRAME_B;
			break;
		case 3: case 8: /* SP */
			nal->Frametype = FRAME_P;
			break;
		case 2: case 7: /* I */
			nal->Frametype = FRAME_I;
			I_Frame_Num ++;
			break;
		case 4: case 9: /* SI */
			nal->Frametype = FRAME_I;
			break;
		}
	}
	else if (nal->nal_unit_type == NAL_SEI)
	{
		nal->Frametype = NAL_SEI;
	}
	else if(nal->nal_unit_type == NAL_SPS)
	{
		nal->Frametype = NAL_SPS;
	}
	else if(nal->nal_unit_type == NAL_PPS)
	{
		nal->Frametype = NAL_PPS;
	}
	if (OneFrameBuf_H264)
	{
		free(OneFrameBuf_H264);
		OneFrameBuf_H264 = NULL;
	}
	return 1;
}

int TraverseH264File()
{
	NALU_t * n = NULL;
	//分配nal 资源
	n = AllocNALU(MAX_VIDEO_TAG_BUF_SIZE);  
    while (!feof(pVideo_H264_File))    //如果未到文件结尾
	{
		//读取一帧数据
		GetAnnexbNALU(n);
		//判断帧类型
		GetFrameType(n);
	}
	//将文件指针移动到文件开头，重新打开文件
	if (fseek(pVideo_H264_File, 0, 0) < 0) //成功，返回0，失败返回-1
	{
		printf("fseek : pVideo_H264_File Error\n");
		return getchar();
	}
	FreeNALU(n);
	return I_Frame_Num;
}

int WriteStruct_H264_Tag(unsigned char * Buf,unsigned int  Timestamp,unsigned char AACPacketType/*AAC序列头部*/,unsigned int * video_frame_type)
{
	Video_Tag videotag;
	unsigned int readsize = 0;
	unsigned int writesize = 0;
	unsigned int CompositionTime = 0x00;  //AVC时，全0，无意义,产生tag的时间，设为0
	NALU_t * n = NULL;
	//分配nal 资源
	n = AllocNALU(MAX_VIDEO_TAG_BUF_SIZE);                                
	if (!feof(pVideo_H264_File))    //如果未到文件结尾
	{     
		//填写videotag头
		videotag.Type = 0x09;
		videotag.DataSize = n->len + 4 + 5;  //nal长度，4个字节的长度，5个字节tag所用字节
		videotag.Timestamp = Timestamp;
		videotag.TimestampExtended = 0x00;
		videotag.StreamID = 0x00;
		videotag.FrameType = 0x02;
		videotag.CodecID = 0x07;
		videotag.AVCPacketType = AACPacketType;
		videotag.CompositionTime = CompositionTime;
		if (AACPacketType == 0x00) //AVC sequence header
		{
			//读取二帧数据
			NALU_t * n_1 = NULL;
			NALU_t * n_2 = NULL;
			unsigned int avcc_pos;
			n_1 = AllocNALU(MAX_VIDEO_TAG_BUF_SIZE); 
			n_2 = AllocNALU(MAX_VIDEO_TAG_BUF_SIZE); 
loop_1_1:
			GetAnnexbNALU(n_1);
			//判断帧类型
			GetFrameType(n_1);
			if (n_1->nal_unit_type == NAL_SPS)
			{
loop_1_2:
				GetAnnexbNALU(n_2);
				//判断帧类型
				GetFrameType(n_2);
				if (n_2->nal_unit_type == NAL_PPS)
				{
					avcc_pos = Create_AVCDecoderConfigurationRecord(Buf + 16,n_1->buf,n_1->len,n_2->buf,n_2->len);  //这里注意 sps，pps 是没有4字节前缀或 4字节的 长度的
					videotag.DataSize = avcc_pos + 5;
					videotag.FrameType = 0x01;
					videotag.CompositionTime = CompositionTime;
				}
				else
				{
					goto loop_1_2;
				}
			}
			else
			{
				goto loop_1_1;
			}
			FreeNALU(n_1);     
			FreeNALU(n_2);     
		}
		else if(AACPacketType == 0x01)                      //AVC NALU
		{
loop_2:
			//读取一帧数据
			int startcodeprefix_size = GetAnnexbNALU(n);
			if (startcodeprefix_size == 0)
			{
				decode_video_done = 1;
				return 0;
			}
			//判断帧类型
			GetFrameType(n);
			*video_frame_type = n->Frametype;
			if (n->nal_unit_type == NAL_SEI)               //如果是SEI帧直接去掉
			{
				goto loop_2;
			}
			if (n->Frametype == FRAME_I)
			{
				//将data填入bufz中
				videotag.DataSize = n->len + 4 + 5; //nal长度，4个字节的长度，5个字节tag所用字节
				videotag.FrameType = 0x01; //标记为I帧
				videotag.CompositionTime = 0x00;  
				Buf[16] = n->len >> 24;
				Buf[17] = (n->len >> 16) & 0xFF;
				Buf[18] = (n->len >> 8) & 0xFF;
				Buf[19] = n->len & 0xFF;
				memcpy(Buf + 20 ,n->buf,n->len);
			}
			else if (n->Frametype == FRAME_B || n->Frametype == FRAME_P)
			{
				videotag.DataSize = n->len + 4 + 5; //nal长度，4个字节的长度，5个字节tag所用字节
				videotag.FrameType = 0x02;  //不是I帧 
				videotag.CompositionTime = CompositionTime;  
				Buf[16] = n->len >> 24;
				Buf[17] = (n->len >> 16) & 0xFF;
				Buf[18] = (n->len >> 8) & 0xFF;
				Buf[19] = n->len & 0xFF;
				memcpy(Buf + 20 ,n->buf,n->len);
			}
			else //其它帧直接去掉
			{
				goto loop_2; 
			}
		}
	}
	else
	{
		decode_video_done = 1;
		FreeNALU(n);
		return 0;
	}
	//填写文件头buf
	Buf[0] = videotag.Type ;
	Buf[1] = (videotag.DataSize) >> 16;
	Buf[2] = ((videotag.DataSize) >> 8) & 0xFF;
	Buf[3] = videotag.DataSize & 0xFF;         
	Buf[4] = (videotag.Timestamp) >> 16;
	Buf[5] = ((videotag.Timestamp) >> 8) & 0xFF;
	Buf[6] = videotag.Timestamp & 0xFF;         
	Buf[7] = videotag.TimestampExtended;
	Buf[8] = (videotag.StreamID) >> 16;
	Buf[9] = ((videotag.StreamID) >> 8) & 0xFF;
	Buf[10] = videotag.StreamID & 0xFF; 
	Buf[11] = (videotag.FrameType << 4)  | (videotag.CodecID);
	Buf[12] = videotag.AVCPacketType;
	Buf[13] = videotag.CompositionTime >> 16;
	Buf[14] = (videotag.CompositionTime >> 8) & 0xFF;
	Buf[15] = videotag.CompositionTime & 0xFF;

	FreeNALU(n);                                                   //释放nal 资源 
	return videotag.DataSize + VIDEO_TAG_HEADER_LENGTH;
}

