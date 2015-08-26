#include "Mux.h"

unsigned char m_File_Header[FILE_HEADER_LENGTH];
unsigned char Video_Tag_Buf[MAX_VIDEO_TAG_BUF_SIZE];
unsigned char Audio_Tag_Buf[MAX_AUDIO_TAG_BUF_SIZE];
unsigned char Script_Tag_Buf[MAX_SCRIPT_TAG_BUF_SIZE];


int Write_File_Header(unsigned char * buf)
{
	WriteStruct_File_Header(buf , FILE_HEADER_LENGTH);
	return WriteFile(pVideo_Audio_Flv_File,(char *)buf,FILE_HEADER_LENGTH);
}

int Write_Audio_Tag(unsigned char * Buf,unsigned int  Timestamp,unsigned char AACPacketType/*AAC序列头部*/)
{
	unsigned int writelength = 0;
	writelength = WriteStruct_Aac_Tag(Buf,Timestamp,AACPacketType);
	return WriteFile(pVideo_Audio_Flv_File,(char *)Buf,writelength);
}

int Wtire_Video_Tag(unsigned char * buf,unsigned int  Timestamp,unsigned char AACPacketType/*AAC序列头部*/,unsigned int * video_frame_type)
{
	unsigned int writelength = 0;
    writelength =  WriteStruct_H264_Tag(buf,Timestamp,AACPacketType,video_frame_type);
	return WriteFile(pVideo_Audio_Flv_File,(char *)buf,writelength);
}

int Write_Script_Tag(unsigned char * buf,double duration,double width,double height,double framerate,double audiosamplerate,int stereo,double filesize,
					 unsigned int filepositions_times_length ,double * filepositions,double * times)
{
	unsigned int writelength = 0;
	writelength = WriteStruct_Script_Tag(buf,duration,width,height,framerate,audiosamplerate,stereo,filesize,filepositions_times_length,filepositions,times);
	return WriteFile(pVideo_Audio_Flv_File,(char *)buf,writelength);
}

int WriteBuf2File(double width ,double height,double framerate)
{
	unsigned int write_size_pos = 0;
	unsigned int write_video_size = 0;
	unsigned int write_audio_size = 0;
	unsigned char Tag_Size[4];
	double Timestamp_video = 0.0;    //一帧视频所用时间
	double Timestamp_audio = 0.0;    //一帧音频所用时间  
	unsigned int filepositions_times_length = 0;  //文件中I帧的数目
	double duration = 0.0;          //文件持续时间
	double audiosamplerate = 0.0;   //采样率
	int stereo = 0;                 //是否是立体声
	double filesize = 0.0;          //文件数据大小
	double * filepositions;         //I帧在文件中的位置
	unsigned int filepositions_count = 0;           //计数
	double * times;                 //I帧的时间
	unsigned int times_count = 0;                   //计数
	unsigned int Script_Size = 0;   //script tag一共的tag数目 
	unsigned int vide_frame_type = 0;  //取出帧类型

	//将文件头写入文件
	write_size_pos += Write_File_Header(m_File_Header);
	if (write_size_pos != FILE_HEADER_LENGTH)
	{
		printf("Write FLV File Header Error\n");
		return getchar();
	}
	//遍历H264视频查找I帧的数量
	I_Frame_Num = 0;
	filepositions_times_length = TraverseH264File();
	I_Frame_Num = 0;
	//查找AAC文件的信息
	ADTS_HEADER adtsheader;
	Detach_Head_Aac(&adtsheader); 
	if (adtsheader.sf_index == 0x04)
	{
		audiosamplerate = 44100.00;
	}
	if (adtsheader.channel_configuration == 2)
	{
		stereo = 1;
	}
	//将音频文件移动到开头

	if (fseek(pAudio_Aac_File, 0, 0) < 0) //成功，返回0，失败返回-1
	{
		printf("fseek : pAudio_Aac_File Error\n");
		return getchar();
	}
	//分配空间
	if ((filepositions = (double *)calloc(filepositions_times_length,sizeof(double))) == NULL)
	{
		printf("alloc filepositions error\n");
		getchar();
	}
	if ((times = (double *)calloc(filepositions_times_length,sizeof(double))) == NULL)
	{
		printf("alloc times error\n");
		getchar();
	}

	//算出 scripttag的长度 
	Script_Size = 11 + //Tag Header*/ 
		          18 + //第一个AMF,//第二个AMF
				  19 + //duration:8
				  16 + //width:5
				  17 + //height:6
				  24 + //videodatarate:13
				  20 + //framerate:9
				  23 + //videocodecid:12
				  26 + //audiosamplerate :15
				  26 + //audiosamplesize:15
				  10 + //stereo:6
				  23 + //audiocodecid:12
				  19 + //filesize:8
				  12 + //keyframes:9 
				  20 +  (1 + 8) * filepositions_times_length + //filepositions:13 
				  12 +  (1 + 8) * filepositions_times_length + //times:5 
				  4  ; //写入tag末尾字符 00 00 00 09

	//填写 first——tag size;
	Tag_Size[0] = 0x00;
	Tag_Size[1] = 0x00;
	Tag_Size[2] = 0x00;
	Tag_Size[3] = 0x00;
	write_size_pos += WriteFile(pVideo_Audio_Flv_File,(char *)Tag_Size,4);

	//将文件内部指针移动Script_Size个字节
	if (fseek(pVideo_Audio_Flv_File, Script_Size, SEEK_CUR) < 0) //成功，返回0，失败返回-1
	{
		printf("fseek : pVideo_Audio_Flv_File Error\n");
		return getchar();
	}

	//填写script tag size
	Tag_Size[0] = Script_Size >> 24;
	Tag_Size[1] = (Script_Size >> 16) & 0xFF;
	Tag_Size[2] = (Script_Size >> 8) & 0xFF;;
	Tag_Size[3] = Script_Size & 0xFF;
	write_size_pos += WriteFile(pVideo_Audio_Flv_File,(char *)Tag_Size,4);

	//开始写入音视频数据TAG
    //1：填写audio，config
	write_audio_size = Write_Audio_Tag(Audio_Tag_Buf,0x00,0x00);
	write_size_pos += write_audio_size;
    //2:填写 audio，config tag size
	Tag_Size[0] = write_audio_size >> 24;
	Tag_Size[1] = (write_audio_size >> 16) & 0xFF;
	Tag_Size[2] = (write_audio_size >> 8) & 0xFF;;
	Tag_Size[3] = write_audio_size & 0xFF;
	write_size_pos += WriteFile(pVideo_Audio_Flv_File,(char *)Tag_Size,4);
	//3:填写video，config
	write_video_size = Wtire_Video_Tag(Video_Tag_Buf,0x00,0x00,&vide_frame_type);
	write_size_pos += write_video_size;
	//4:填写 video，config tag size
	Tag_Size[0] = write_video_size >> 24;
	Tag_Size[1] = (write_video_size >> 16) & 0xFF;
	Tag_Size[2] = (write_video_size >> 8) & 0xFF;;
	Tag_Size[3] = write_video_size & 0xFF;
	write_size_pos += WriteFile(pVideo_Audio_Flv_File,(char *)Tag_Size,4);

	//开始循环写入音视频数据
	for (;;)
	{
		if (/*文件读取完毕*/(decode_video_done && decode_audio_done))
		{
			break;
		}
		//视频文件读取完毕
		if (decode_video_done)
		{
			write_audio_size = Write_Audio_Tag(Audio_Tag_Buf,Timestamp_audio,0x01);
			write_size_pos += write_audio_size;
			Tag_Size[0] = write_audio_size >> 24;
			Tag_Size[1] = (write_audio_size >> 16) & 0xFF;
			Tag_Size[2] = (write_audio_size >> 8) & 0xFF;;
			Tag_Size[3] = write_audio_size & 0xFF;
			write_size_pos += WriteFile(pVideo_Audio_Flv_File,(char *)Tag_Size,4);
			//计算一帧音频所用时间
			Timestamp_audio += 1024*1000/audiosamplerate;
			printf("当前帧是 AUDIO :   AAC  TAG_SIZE : %d\n",write_audio_size);
			continue;
		}
		//音频文件读取完毕
		if (decode_audio_done)
		{
			write_video_size = Wtire_Video_Tag(Video_Tag_Buf,Timestamp_video,0x01,&vide_frame_type);
			if (write_video_size == 0)
			{
				continue;
			}
			write_size_pos += write_video_size ;
			Tag_Size[0] = write_video_size >> 24;
			Tag_Size[1] = (write_video_size >> 16) & 0xFF;
			Tag_Size[2] = (write_video_size >> 8) & 0xFF;;
			Tag_Size[3] = write_video_size & 0xFF;
			write_size_pos += WriteFile(pVideo_Audio_Flv_File,(char *)Tag_Size,4);
			if (vide_frame_type == FRAME_I)
			{
				printf("当前帧是 VIDEO : I 帧！ TAG_SIZE : %d\n",write_video_size);
				filepositions[filepositions_count] = write_size_pos + Script_Size - write_video_size - 4;
				filepositions_count ++;
				times[times_count] = Timestamp_video;
				times_count ++;

			}
			else if (vide_frame_type == FRAME_B )
			{
				printf("当前帧是 VIDEO : B 帧！ TAG_SIZE : %d\n",write_video_size);
			}
			else if (vide_frame_type == FRAME_P)
			{
				printf("当前帧是 VIDEO : P 帧！ TAG_SIZE : %d\n",write_video_size);
			}
			else
			{
				printf("vide_frame_type error\n");
				getchar();
			}
			//计算一帧视频所用时间
			Timestamp_video += 1000/framerate;
			continue;
		}

		/* write interleaved audio and video frames */
		if ( Timestamp_audio > Timestamp_video )
		{
			write_video_size = Wtire_Video_Tag(Video_Tag_Buf,Timestamp_video,0x01,&vide_frame_type);
			if (write_video_size == 0)
			{
				continue;
			}
			write_size_pos += write_video_size ;
			Tag_Size[0] = write_video_size >> 24;
			Tag_Size[1] = (write_video_size >> 16) & 0xFF;
			Tag_Size[2] = (write_video_size >> 8) & 0xFF;;
			Tag_Size[3] = write_video_size & 0xFF;
			write_size_pos += WriteFile(pVideo_Audio_Flv_File,(char *)Tag_Size,4);

			if (vide_frame_type == FRAME_I)
			{
				printf("当前帧是 VIDEO : I 帧！ TAG_SIZE : %d\n",write_video_size);
				filepositions[filepositions_count] = write_size_pos + Script_Size - write_video_size - 4;
				filepositions_count ++;
				times[times_count] = Timestamp_video;
				times_count ++;
			}
			else if (vide_frame_type == FRAME_B )
			{
				printf("当前帧是 VIDEO : B 帧！ TAG_SIZE : %d\n",write_video_size);
			}
			else if (vide_frame_type == FRAME_P)
			{
				printf("当前帧是 VIDEO : P 帧！ TAG_SIZE : %d\n",write_video_size);
			}
			else
			{
				printf("vide_frame_type error\n");
				getchar();
			}
			//计算一帧视频所用时间
			Timestamp_video += 1000/framerate;
		}
		else
		{
			write_audio_size = Write_Audio_Tag(Audio_Tag_Buf,Timestamp_audio,0x01);
			write_size_pos += write_audio_size;
			Tag_Size[0] = write_audio_size >> 24;
			Tag_Size[1] = (write_audio_size >> 16) & 0xFF;
			Tag_Size[2] = (write_audio_size >> 8) & 0xFF;;
			Tag_Size[3] = write_audio_size & 0xFF;
			write_size_pos += WriteFile(pVideo_Audio_Flv_File,(char *)Tag_Size,4);
			//计算一帧音频所用时间
			Timestamp_audio += 1024*1000/audiosamplerate;
			printf("当前帧是 AUDIO :   AAC  TAG_SIZE : %d\n",write_audio_size);
		}
	}
	//将文件内部指针移动到script 前面
	if (fseek(pVideo_Audio_Flv_File, FILE_HEADER_LENGTH  + 4, 0) < 0) //成功，返回0，失败返回-1
	{
		printf("fseek : pVideo_Audio_Flv_File FILE_HEADER_LENGTH + 4 Error\n");
		return getchar();
	}
	//填写 script tag
	if (Timestamp_audio > Timestamp_video )
	{
		duration = Timestamp_audio/1000;
	}
	else
	{
		duration = Timestamp_video/1000 ;
	}
	filesize = write_size_pos + Script_Size;
	write_size_pos += Write_Script_Tag(Script_Tag_Buf, duration, width, height, framerate, audiosamplerate,stereo,
		                               filesize,filepositions_times_length ,filepositions,times);

	//将文件内部指针移动到文件末尾
	if (fseek(pVideo_Audio_Flv_File,0, SEEK_END) < 0) //成功，返回0，失败返回-1
	{
		printf("fseek : pVideo_Audio_Flv_File SEEK_END + 4 Error\n");
		return getchar();
	}
	printf("\n\n");
	return 1;
}