/*
@author:chenzhengqiang
@version:1.0
@start date:2015/8/26
@modified date:
@desc:providing the api for handling h264 file
*/

#include "errors.h"
#include "ts_mux_h264.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

NALU_t *allocate_nal_unit(int buffersize)
{
	NALU_t *nal_unit;

	if ((nal_unit = (NALU_t*)calloc (1, sizeof(NALU_t))) == NULL)
	{
		printf("allocate_nal_unit Error: Allocate Meory To NALU_t Failed ");
		getchar();
	}

	nal_unit->max_size = buffersize;									//Assign buffer size 

	if ((nal_unit->buf = (unsigned char*)calloc (buffersize, sizeof (char))) == NULL)
	{
		free (nal_unit);
		printf ("allocate_nal_unit Error: Allocate Meory To NALU_t Buffer Failed ");
		getchar();
	}
	return nal_unit;
}

void free_nal_unit(NALU_t *nal_unit)
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

int is_the_right_nalu_prefix3 (unsigned char *Buf)
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

int is_the_right_nalu_prefix4 (unsigned char *Buf)
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

int read_h264_nal_unit( FILE *fh264_handler, NALU_t * nalu )
{
	int pos = 0;                  //一个nal到下一个nal 数据移动的指针
	int start_code_prefix_found  = 0;      //是否找到下一个nal 的前缀
	int rewind = 0;               //判断 前缀所占字节数 3或 4
	unsigned char * nal_unit_buffer = NULL;
	static int info2 =0 ;
	static int info3 =0 ;

	if ((nal_unit_buffer = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL) 
	{
		printf ("GetAnnexbNALU Error: Could not allocate Buf memory\nal_unit");
	}

	nalu->startcodeprefix_len = 3;      //初始化前缀位三个字节

	if (3 != fread (nal_unit_buffer, 1, 3, fh264_handler))//从文件读取三个字节到buf
	{
		free(nal_unit_buffer);
		return 0;
	}
    
	info2 = is_the_right_nalu_prefix3 (nal_unit_buffer);       //Check whether Buf is 0x000001
	if(info2 != 1) 
	{
		//if the prefix code is not 0x000001,then read one more byte
		if(1 != fread(nal_unit_buffer + 3, 1, 1, fh264_handler))
		{
			free(nal_unit_buffer);
			return 0;
		}
		info3 = is_the_right_nalu_prefix4 (nal_unit_buffer);   //Check whether Buf is 0x00000001
		if (info3 != 1)                 //If not the return -1
		{ 
			free(nal_unit_buffer);
			return -1;
		}
		else 
		{
			//if the prefix code is 0x00000001,then set the prefix length to 4 bytes
			pos = 4;
			nalu->startcodeprefix_len = 4;
		}
	} 
	else
	{
		//if the prefix code is 0x000001,set the prefix length to 3 bytes
		pos = 3;
		nalu->startcodeprefix_len = 3;
	}
	//寻找下一个字符符号位， 即 寻找一个nal 从一个0000001 到下一个00000001
	start_code_prefix_found = 0;
	info2 = 0;
	info3 = 0;
	while (!start_code_prefix_found)
	{
		if (feof (fh264_handler))                                 //如果到了文件结尾
		{
			nalu->len = (pos-1) - nalu->startcodeprefix_len;  //从0 开始
			memcpy (nalu->buf, &nal_unit_buffer[nalu->startcodeprefix_len], nalu->len);     
			nalu->forbidden_bit = nalu->buf[0] & 0x80;      // 1 bit--10000000
			nalu->nal_reference_idc = nalu->buf[0] & 0x60;  // 2 bit--01100000
			nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;    // 5 bit--00011111
			free(nal_unit_buffer);
			return ((info3 == 1)? 4 : 3);
		}
		nal_unit_buffer[pos++] = fgetc (fh264_handler);                       //Read one char to the Buffer 一个字节一个字节从文件向后找
		info3 = is_the_right_nalu_prefix4(&nal_unit_buffer[pos-4]);		        //Check whether Buf is 0x00000001 
		if(info3 != 1)
		{
			info2 = is_the_right_nalu_prefix3(&nal_unit_buffer[pos-3]);            //Check whether Buf is 0x000001
		}
		start_code_prefix_found = (info2 == 1 || info3 == 1);        //如果找到下一个前缀
	}

	rewind = (info3 == 1)? -4 : -3;

	if (0 != fseek (fh264_handler, rewind, SEEK_CUR))			    //将文件内部指针移动到 nal 的末尾
	{
		free(nal_unit_buffer);
		printf("GetAnnexbNALU Error: Cannot fseek in the bit stream file");
	}

	nalu->len = (pos + rewind) -  nalu->startcodeprefix_len; 
	memcpy (nalu->buf, &nal_unit_buffer[nalu->startcodeprefix_len], nalu->len);//拷贝一个nal 数据到数组中
	nalu->forbidden_bit = nalu->buf[0] & 0x80;          
	nalu->nal_reference_idc = nalu->buf[0] & 0x60;                 // 2 bit
	nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;                   // 5 bit
	free(nal_unit_buffer);
	return ((info3 == 1)? 4 : 3);                                               
}


int get_h264_frame_type(NALU_t * nal)
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


/*
@desc:
 read a frame from h264 file 
*/
int read_h264_frame(FILE *fh264_handler,unsigned char * h264_frame,
                             unsigned int & frame_length, unsigned int & frame_type)
{
	NALU_t * nal_unit = NULL;
	//分配nal 资源
	nal_unit = allocate_nal_unit( MAX_VIDEO_TAG_BUF_SIZE ); 

	//读取一帧数据
	int startcodeprefix_size = read_h264_nal_unit( fh264_handler, nal_unit );
	if (startcodeprefix_size == 0)
	{
		return FILE_EOF;
	}

	//判断帧类型
	get_h264_frame_type(nal_unit);
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
		memcpy( h264_frame + 4,nal_unit->buf,nal_unit->len);
	}
	else
	{
		return FILE_FORMAT_ERROR;
	}

	frame_length = nal_unit->startcodeprefix_len + nal_unit->len;
	free_nal_unit(nal_unit);                                                   //释放nal 资源 
	return OK;
}




/*
@desc:
 encapsulate the h264 frame to pes packet
*/
int h264_frame_2_pes(unsigned char *h264_frame,unsigned int frame_length,unsigned long h264_pts,TsPes & h264_pes)
{
	unsigned int h264pes_pos = 0;
	h264pes_pos += frame_length;

       memcpy(h264_pes.Es,h264_frame,frame_length);
	h264_pes.packet_start_code_prefix = 0x000001;
	h264_pes.stream_id = TS_H264_STREAM_ID;                               //E0~EF表示是视频的,C0~DF是音频,H264-- E0
	h264_pes.PES_packet_length = 0 ;                                      //一帧数据的长度 不包含 PES包头 ,这个8 是 自适应的长度,填0 可以自动查找
	h264_pes.Pes_Packet_Length_Beyond = frame_length;

	if ( frame_length> 0xFFFF)                                          //如果一帧数据的大小超出界限
	{
		h264_pes.PES_packet_length = 0x00;
		h264_pes.Pes_Packet_Length_Beyond = frame_length;
		h264pes_pos += 16;
	}
	else
	{
		h264_pes.PES_packet_length = 0x00;
		h264_pes.Pes_Packet_Length_Beyond = frame_length;
		h264pes_pos += 14;
	}
	h264_pes.marker_bit = 0x02;
	h264_pes.PES_scrambling_control = 0x00;                               //人选字段 存在，不加扰
	h264_pes.PES_priority = 0x00;
	h264_pes.data_alignment_indicator = 0x00;
	h264_pes.copyright = 0x00;
	h264_pes.original_or_copy = 0x00;
	h264_pes.PTS_DTS_flags = 0x02;                                         //10'：PTS字段存在,DTS不存在
	h264_pes.ESCR_flag = 0x00;
	h264_pes.ES_rate_flag = 0x00;
	h264_pes.DSM_trick_mode_flag = 0x00;
	h264_pes.additional_copy_info_flag = 0x00;
	h264_pes.PES_CRC_flag = 0x00;
	h264_pes.PES_extension_flag = 0x00;
	h264_pes.PES_header_data_length = 0x05;                                //后面的数据 包括了PTS所占的字节数

	//清 0 
	h264_pes.tsptsdts.pts_32_30  = 0;
	h264_pes.tsptsdts.pts_29_15 = 0;
	h264_pes.tsptsdts.pts_14_0 = 0;

	h264_pes.tsptsdts.reserved_1 = 0x0003;                                 //填写 pts信息
	// Videopts大于30bit，使用最高三位 
	if(h264_pts > 0x7FFFFFFF)
	{
		h264_pes.tsptsdts.pts_32_30 = (h264_pts >> 30) & 0x07;                 
		h264_pes.tsptsdts.marker_bit1 = 0x01;
	}
	else 
	{
		h264_pes.tsptsdts.marker_bit1 = 0;
	}
	// Videopts大于15bit，使用更多的位来存储
	if(h264_pts > 0x7FFF)
	{
		h264_pes.tsptsdts.pts_29_15 = (h264_pts >> 15) & 0x007FFF ;
		h264_pes.tsptsdts.marker_bit2 = 0x01;
	}
	else
	{
		h264_pes.tsptsdts.marker_bit2 = 0;
	}
	//使用最后15位
	h264_pes.tsptsdts.pts_14_0 = h264_pts & 0x007FFF;
	h264_pes.tsptsdts.marker_bit3 = 0x01;

	return h264pes_pos;
}



/*
@desc:read a nal unit from input stream buffer 
*/
int read_h264_nal_unit( uint8_t *stream_buffer, uint8_t buffer_size, NALU_t * nalu )
{
       if( stream_buffer == NULL || buffer_size <= 3 )
       return ARGUMENT_ERROR;
       
	int nalu_pos = 0;
       int stream_pos = 0;
	int start_code_prefix_found  = 0;   
	uint8_t * nal_unit_buffer = NULL;
    
	static int info2 =0 ;
	static int info3 =0 ;

	if ((nal_unit_buffer = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL ) 
	{
		return -1;
	}

	nalu->startcodeprefix_len = 3; 
       memcpy(nal_unit_buffer,stream_buffer+stream_pos,3);
       stream_pos+=3;
	info2 = is_the_right_nalu_prefix3(nal_unit_buffer);       //Check whether nal_unit_buffer is 0x000001
	if( info2 != 1 ) 
	{
		//if nal_unit_buffer is not 0x000001,then read one more byte cause it could be 0x00000001
		if( (stream_pos +1) >= buffer_size )
             return -1;
        
             memcpy(nal_unit_buffer+3,stream_buffer+stream_pos,1);
             stream_pos+=1;
		info3 = is_the_right_nalu_prefix4 ( nal_unit_buffer );   //Check whether nal_unit_buffer is 0x00000001
		if (info3 != 1)                 //If not the return -1
		{ 
			free(nal_unit_buffer);
			return -1;
		}
		else 
		{
			//if nal_unit_buffer is 0x00000001,set the prefix length to 4 bytes
			nalu_pos = 4;
			nalu->startcodeprefix_len = 4;
		}
	} 
	else
	{
		//if nal_unit_buffer is 0x000001,set the prefix length to 3 bytes
		nalu_pos = 3;
		nalu->startcodeprefix_len = 3;
	}
    
	//go to find the next nalu prefix code
	start_code_prefix_found = 0;
	info2 = 0;
	info3 = 0;
    
	while (!start_code_prefix_found )
	{
		if ( stream_pos >= buffer_size )         
		{
			nalu->len = nalu_pos- nalu->startcodeprefix_len;
			memcpy (nalu->buf, &nal_unit_buffer[nalu->startcodeprefix_len], nalu->len);     
			nalu->forbidden_bit = nalu->buf[0] & 0x80;      // 1 bit--10000000
			nalu->nal_reference_idc = nalu->buf[0] & 0x60;  // 2 bit--01100000
			nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;    // 5 bit--00011111
			free(nal_unit_buffer);
			return stream_pos;
		}
        
		nal_unit_buffer[nalu_pos++] = stream_buffer[stream_pos++];
		info3 = is_the_right_nalu_prefix4(&nal_unit_buffer[nalu_pos-4]);		        //Check whether nal_unit_buffer is 0x00000001 
		if(info3 != 1)
		{
			info2 = is_the_right_nalu_prefix3(&nal_unit_buffer[nalu_pos-3]);            //Check whether nal_unit_buffer is 0x000001
		}
		start_code_prefix_found = (info2 == 1 || info3 == 1);   
	}

       int back_pos = (info3 == 1)? -4 : -3;
       stream_pos+=back_pos;
	nalu->len = nalu_pos+back_pos-nalu->startcodeprefix_len; 
	memcpy (nalu->buf, &nal_unit_buffer[nalu->startcodeprefix_len], nalu->len);
	nalu->forbidden_bit = nalu->buf[0] & 0x80;      
	nalu->nal_reference_idc = nalu->buf[0] & 0x60;                 // 2 bit
	nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;                   // 5 bit
	free(nal_unit_buffer);
	return stream_pos;                                    
}



int read_h264_frame( unsigned char * input_stream,unsigned int stream_size, unsigned char * h264_frame,
                                       unsigned int & frame_length, unsigned int & frame_type)
{
	NALU_t * nal_unit = NULL;
	nal_unit = allocate_nal_unit(MAX_VIDEO_TAG_BUF_SIZE); 

	int ret = read_h264_nal_unit( (uint8_t *)input_stream,stream_size,nal_unit );
	if (ret <=0 )
	{
		return ret;
	}

	//判断帧类型
	get_h264_frame_type(nal_unit);
	frame_type = nal_unit->Frametype;

	if ( nal_unit->startcodeprefix_len == 3 )
	{
		h264_frame[0] = 0x00;
		h264_frame[1] = 0x00;
		h264_frame[2] = 0x01;
		memcpy(h264_frame + 3,nal_unit->buf,nal_unit->len);
	}
	else if (nal_unit->startcodeprefix_len == 4 )
	{
		h264_frame[0] = 0x00;
		h264_frame[1] = 0x00;
		h264_frame[2] = 0x00;
		h264_frame[3] = 0x01;
		memcpy( h264_frame + 4,nal_unit->buf,nal_unit->len);
	}
	else
	{
		return STREAM_FORMAT_ERROR;
	}

	frame_length = nal_unit->startcodeprefix_len + nal_unit->len;
	free_nal_unit(nal_unit);                                                   //释放nal 资源 
	return OK;
}


