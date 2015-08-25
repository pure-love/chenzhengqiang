/*
@author:chenzhengqiang
@start date:2015/8/25
@modified date:
@desc:h264 media file related
*/

#include "errors.h"
#include "h264.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

unsigned int decode_video_done = 0;

NALU_t *allocate_h264_nal_unit(int buffersize)
{
	NALU_t *nal_unit;

	if ((nal_unit = (NALU_t*)calloc (1, sizeof(NALU_t))) == NULL)
	{
		printf("AllocNALU Error: Allocate Meory To NALU_t Failed ");
		getchar();
	}

	nal_unit->max_size = buffersize;									//Assign buffer size 

	if ((nal_unit->buf = (unsigned char*)calloc (buffersize, sizeof (char))) == NULL)
	{
		free (nal_unit);
		printf ("AllocNALU Error: Allocate Meory To NALU_t Buffer Failed ");
		getchar();
	}
	return nal_unit;
}

void free_h264_nal_unit(NALU_t *nal_unit)
{
	if (nal_unit)
	{
		if (nal_unit->buf)
		{
			free(nal_unit->buf);
			nal_unit->buf=NULL;
		}
		free (nal_unit);
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

int read_h264_nal_unit (FILE *fh264_handler,NALU_t * nalu)
{
	int pos = 0;                  //一个nal到下一个nal 数据移动的指针
	int StartCodeFound  = 0;      //是否找到下一个nal 的前缀
	int rewind = 0;               //判断 前缀所占字节数 3或 4
	unsigned char * Buf = NULL;
	static int info2 =0 ;
	static int info3 =0 ;

	if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL) 
	{
		printf ("GetAnnexbNALU Error: Could not allocate Buf memory\nal_unit");
	}

	nalu->startcodeprefix_len = 3;      //初始化前缀位三个字节

	if (3 != fread (Buf, 1, 3, fh264_handler))//从文件读取三个字节到buf
	{
		free(Buf);
		return 0;
	}
	info2 = FindStartCode2 (Buf);       //Check whether Buf is 0x000001
	if(info2 != 1) 
	{
		//If Buf is not 0x000001,then read one more byte
		if(1 != fread(Buf + 3, 1, 1, fh264_handler))
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
		if (feof (fh264_handler))                                 //如果到了文件结尾
		{
			nalu->len = (pos-1) - nalu->startcodeprefix_len;  //从0 开始
			memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);     
			nalu->forbidden_bit = nalu->buf[0] & 0x80;      // 1 bit--10000000
			nalu->nal_reference_idc = nalu->buf[0] & 0x60;  // 2 bit--01100000
			nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;    // 5 bit--00011111
			free(Buf);
			return ((info3 == 1)? 4 : 3);
		}
		Buf[pos++] = fgetc (fh264_handler);                       //Read one char to the Buffer 一个字节一个字节从文件向后找
		info3 = FindStartCode3(&Buf[pos-4]);		        //Check whether Buf is 0x00000001 
		if(info3 != 1)
		{
			info2 = FindStartCode2(&Buf[pos-3]);            //Check whether Buf is 0x000001
		}
		StartCodeFound = (info2 == 1 || info3 == 1);        //如果找到下一个前缀
	}

	rewind = (info3 == 1)? -4 : -3;

	if (0 != fseek (fh264_handler, rewind, SEEK_CUR))			    //将文件内部指针移动到 nal 的末尾
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
	return OK;                                        
}

int get_frame_type_from_nal(NALU_t * nal)
{
	bs_t s;
	int frame_type = 0; 
	unsigned char * OneFrameBuf_H264 = NULL ;
	if ((OneFrameBuf_H264 = (unsigned char *)calloc(nal->len + 4,sizeof(unsigned char))) == NULL)
	{
		printf("Error malloc OneFrameBuf_H264\nal_unit");
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
		printf("H264读取错误！\nal_unit");
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

int read_h264_frame(FILE *fh264_handler,unsigned char * h264_frame,unsigned int & frame_length, unsigned int & frame_type)
{
	NALU_t * nal_unit = NULL;
	
	//分配nal 资源
	nal_unit = allocate_h264_nal_unit(MAX_VIDEO_TAG_BUF_SIZE); 

	//读取一帧数据
	int ret = read_h264_nal_unit( fh264_handler, nal_unit );
	if ( ret == OK )
	{
	     return ret;
	}

	//判断帧类型
	get_frame_type_from_nal(nal_unit);
	frame_type = nal_unit->Frametype;

	if (nal_unit->startcodeprefix_len == 3)
	{
		h264_frame[0] = 0x00;
		h264_frame[1] = 0x00;
		h264_frame[2] = 0x01;
		memcpy(h264_frame + 3,nal_unit->buf,nal_unit->len);
	}
	else if (nal_unit->startcodeprefix_len == 4)
	{
		h264_frame[0] = 0x00;
		h264_frame[1] = 0x00;
		h264_frame[2] = 0x00;
		h264_frame[3] = 0x01;
		memcpy(h264_frame + 4,nal_unit->buf,nal_unit->len);
	}
	else
	{
		return FILE_FORMAT_ERROR;
	}

	frame_length = nal_unit->startcodeprefix_len + nal_unit->len;

	free_h264_nal_unit(nal_unit);                                                   //释放nal 资源 
	return OK;
}


