#include "ScriptTag.h"

double char2double(unsigned char * buf,unsigned int size)
{
	double scr = 0.0;
	unsigned char buf_1[8];
	unsigned char buf_2[8];
	memcpy(buf_1,buf,size);
	//大小端问题
	buf_2[0] = buf_1[7];
	buf_2[1] = buf_1[6];
	buf_2[2] = buf_1[5];
	buf_2[3] = buf_1[4];
	buf_2[4] = buf_1[3];
	buf_2[5] = buf_1[2];
	buf_2[6] = buf_1[1];
	buf_2[7] = buf_1[0];
	scr = *(double *)buf_2;
	return scr;
}

void double2char(unsigned char * buf,double val)
{
	*(double *)buf = val;
	unsigned char buf_1[8];
	memcpy(buf_1,buf,8);
	buf[0] = buf_1[7];
	buf[1] = buf_1[6];
	buf[2] = buf_1[5];
	buf[3] = buf_1[4];
	buf[4] = buf_1[3];
	buf[5] = buf_1[2];
	buf[6] = buf_1[1];
	buf[7] = buf_1[0];
}


int WriteStruct_Script_Tag(unsigned char * buf,double duration,double width,double height,double framerate,double audiosamplerate,int stereo,double filesize,
						   unsigned int filepositions_times_length ,double * filepositions,double *times)
{
	Script_Tag scripttag;
	unsigned int scrip_pos = 0;

	//Tag Header
	scripttag.Type = 0x12;
	scripttag.DataSize = 0x00;  //填写完毕再更改:
	scripttag.Timestamp = 0x00;
	scripttag.TimestampExtended = 0x00;
	scripttag.StreamID = 0x00;
	scrip_pos += 11;
	
	//第一个AMF
	scripttag.Type_1 = 0x02;
	scrip_pos ++;
	scripttag.StringLength = 0x0A; //onMetaData”长度
	//固定为0x6F 0x6E 0x4D 0x65 0x74 0x61 0x44 0x61 0x74 0x61，表示字符串onMetaData,下面填写到buf 中  
	scrip_pos += 2;
	scrip_pos += 10;

	//第二个AMF
	scripttag.Type_2 = 0x08;
	scrip_pos ++;
	scripttag.ECMAArrayLength = 0x0C;  //duration,width,height,videodatarate,framerate,videocodecid,audiosamplerate,audiosamplesize,stereo,audiocodecid,filesize,keyframe 共12个
	scrip_pos += 4;
	
	//下面是各个数组的值
	scripttag.duration = duration;
	scripttag.width = width;
	scripttag.height = height;
	scripttag.videodatarate = 0.0;
	scripttag.framerate = framerate;
	scripttag.videocodecid = 7.0;  //avc
	scripttag.audiosamplerate = audiosamplerate; //44100
	scripttag.audiosamplesize = 16.0;
	scripttag.stereo = stereo;
	scripttag.audiocodecid = 10.0;
	scripttag.filesize = filesize;
	for (int i = 0 ; i< filepositions_times_length; i++)
	{
		scripttag.filepositions[i] = filepositions[i];
		scripttag.times[i] = times[i];
	}

	//前面2bytes表示，第N个数组的名字所占的bytes
	//duration:8
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x08;
	scrip_pos += 2;
	//拷贝数组名称
	buf[scrip_pos] = 0x64;
	buf[scrip_pos +1] = 0x75;
	buf[scrip_pos +2] = 0x72;
	buf[scrip_pos +3] = 0x61;
	buf[scrip_pos +4] = 0x74;
	buf[scrip_pos +5] = 0x69;
	buf[scrip_pos +6] = 0x6F;
	buf[scrip_pos +7] = 0x6E;
	scrip_pos += 8;
	//跟着下去的1bytes表示这个数组的属性信息
	buf[scrip_pos] = 0x00;
	scrip_pos ++;
	//8个字节的值
	double2char(buf + scrip_pos,scripttag.duration);
	scrip_pos += 8;

	//width:5
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x05;
	scrip_pos += 2;
	buf[scrip_pos] = 0x77;
	buf[scrip_pos +1] = 0x69;
	buf[scrip_pos +2] = 0x64;
	buf[scrip_pos +3] = 0x74;
	buf[scrip_pos +4] = 0x68;
	scrip_pos += 5;
	buf[scrip_pos] = 0x00;
	scrip_pos ++;
	//8个字节的值
	double2char(buf + scrip_pos,scripttag.width);
	scrip_pos += 8;

	//height:6
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x06;
	scrip_pos += 2;
	buf[scrip_pos] = 0x68;
	buf[scrip_pos +1] = 0x65;
	buf[scrip_pos +2] = 0x69;
	buf[scrip_pos +3] = 0x67;
	buf[scrip_pos +4] = 0x68;
	buf[scrip_pos +5] = 0x74;
	scrip_pos += 6;
	buf[scrip_pos] = 0x00;
	scrip_pos ++;
	//8个字节的值
	double2char(buf + scrip_pos,scripttag.height);
	scrip_pos += 8;

	//videodatarate:13
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x0D;
	scrip_pos += 2;
	buf[scrip_pos] = 0x76;
	buf[scrip_pos +1] = 0x69;
	buf[scrip_pos +2] = 0x64;
	buf[scrip_pos +3] = 0x65;
	buf[scrip_pos +4] = 0x6F;
	buf[scrip_pos +5] = 0x64;
	buf[scrip_pos +6] = 0x61;
	buf[scrip_pos +7] = 0x74;
	buf[scrip_pos +8] = 0x61;
	buf[scrip_pos +9] = 0x72;
	buf[scrip_pos +10] = 0x61;
	buf[scrip_pos +11] = 0x74;
	buf[scrip_pos +12] = 0x65;
	scrip_pos += 13;
	buf[scrip_pos] = 0x00;
	scrip_pos ++;
	//8个字节的值
	double2char(buf + scrip_pos,scripttag.videodatarate);
	scrip_pos += 8;

	//framerate:9
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x09;
	scrip_pos += 2;
	buf[scrip_pos] = 0x66;
	buf[scrip_pos +1] = 0x72;
	buf[scrip_pos +2] = 0x61;
	buf[scrip_pos +3] = 0x6D;
	buf[scrip_pos +4] = 0x65;
	buf[scrip_pos +5] = 0x72;
	buf[scrip_pos +6] = 0x61;
	buf[scrip_pos +7] = 0x74;
	buf[scrip_pos +8] = 0x65;
	scrip_pos += 9;
	buf[scrip_pos] = 0x00;
	scrip_pos ++;
	//8个字节的值
	double2char(buf + scrip_pos,scripttag.framerate);
	scrip_pos += 8;

	//videocodecid:12
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x0C;
	scrip_pos += 2;
	buf[scrip_pos] = 0x76;
	buf[scrip_pos +1] = 0x69;
	buf[scrip_pos +2] = 0x64;
	buf[scrip_pos +3] = 0x65;
	buf[scrip_pos +4] = 0x6F;
	buf[scrip_pos +5] = 0x63;
	buf[scrip_pos +6] = 0x6F;
	buf[scrip_pos +7] = 0x64;
	buf[scrip_pos +8] = 0x65;
	buf[scrip_pos +9] = 0x63;
	buf[scrip_pos +10] = 0x69;
	buf[scrip_pos +11] = 0x64;
	scrip_pos += 12;
	buf[scrip_pos] = 0x00;
	scrip_pos ++;
	//8个字节的值
	double2char(buf + scrip_pos,scripttag.videocodecid);
	scrip_pos += 8;

	//audiosamplerate :15
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x0F;
	scrip_pos += 2;
	buf[scrip_pos] = 0x61;
	buf[scrip_pos +1] = 0x75;
	buf[scrip_pos +2] = 0x64;
	buf[scrip_pos +3] = 0x69;
	buf[scrip_pos +4] = 0x6F;
	buf[scrip_pos +5] = 0x73;
	buf[scrip_pos +6] = 0x61;
	buf[scrip_pos +7] = 0x6D;
	buf[scrip_pos +8] = 0x70;
	buf[scrip_pos +9] = 0x6C;
	buf[scrip_pos +10] = 0x65;
	buf[scrip_pos +11] = 0x72;
	buf[scrip_pos +12] = 0x61;
	buf[scrip_pos +13] = 0x74;
	buf[scrip_pos +14] = 0x65;
	scrip_pos += 15;
	buf[scrip_pos] = 0x00;
	scrip_pos ++;
	//8个字节的值
	double2char(buf + scrip_pos,scripttag.audiosamplerate);
	scrip_pos += 8;

	//audiosamplesize:15
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x0F;
	scrip_pos += 2;
	buf[scrip_pos] = 0x61;
	buf[scrip_pos +1] = 0x75;
	buf[scrip_pos +2] = 0x64;
	buf[scrip_pos +3] = 0x69;
	buf[scrip_pos +4] = 0x6F;
	buf[scrip_pos +5] = 0x73;
	buf[scrip_pos +6] = 0x61;
	buf[scrip_pos +7] = 0x6D;
	buf[scrip_pos +8] = 0x70;
	buf[scrip_pos +9] = 0x6C;
	buf[scrip_pos +10] = 0x65;
	buf[scrip_pos +11] = 0x73;
	buf[scrip_pos +12] = 0x69;
	buf[scrip_pos +13] = 0x7A;
	buf[scrip_pos +14] = 0x65;
	scrip_pos += 15;
	buf[scrip_pos] = 0x00;
	scrip_pos ++;
	//8个字节的值
	double2char(buf + scrip_pos,scripttag.audiosamplesize);
	scrip_pos += 8;

	//stereo:6
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x06;
	scrip_pos += 2;
	buf[scrip_pos] = 0x73;
	buf[scrip_pos +1] = 0x74;
	buf[scrip_pos +2] = 0x65;
	buf[scrip_pos +3] = 0x72;
	buf[scrip_pos +4] = 0x65;
	buf[scrip_pos +5] = 0x6F;
	scrip_pos += 6;
	buf[scrip_pos] = 0x01;
	scrip_pos ++;
	//1个字节的值
	buf[scrip_pos] = scripttag.stereo;
	scrip_pos ++;

	//audiocodecid:12
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x0C;
	scrip_pos += 2;
	buf[scrip_pos] = 0x61;
	buf[scrip_pos +1] = 0x75;
	buf[scrip_pos +2] = 0x64;
	buf[scrip_pos +3] = 0x69;
	buf[scrip_pos +4] = 0x6F;
	buf[scrip_pos +5] = 0x63;
	buf[scrip_pos +6] = 0x6F;
	buf[scrip_pos +7] = 0x64;
	buf[scrip_pos +8] = 0x65;
	buf[scrip_pos +9] = 0x63;
	buf[scrip_pos +10] = 0x69;
	buf[scrip_pos +11] = 0x64;
	scrip_pos += 12;
	buf[scrip_pos] = 0x00;
	scrip_pos ++;
	//8个字节的值
	double2char(buf + scrip_pos,scripttag.audiocodecid);
	scrip_pos += 8;

	//filesize:8
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x08;
	scrip_pos += 2;
	buf[scrip_pos] = 0x66;
	buf[scrip_pos +1] = 0x69;
	buf[scrip_pos +2] = 0x6c;
	buf[scrip_pos +3] = 0x65;
	buf[scrip_pos +4] = 0x73;
	buf[scrip_pos +5] = 0x69;
	buf[scrip_pos +6] = 0x7A;
	buf[scrip_pos +7] = 0x65;
	scrip_pos += 8;
	buf[scrip_pos] = 0x00;
	scrip_pos ++;
	//8个字节的值
	double2char(buf + scrip_pos,scripttag.filesize);
	scrip_pos += 8;

	//keyframes:9 
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x09;
	scrip_pos += 2;
	buf[scrip_pos] = 0x6B;
	buf[scrip_pos +1] = 0x65;
	buf[scrip_pos +2] = 0x79;
	buf[scrip_pos +3] = 0x66;
	buf[scrip_pos +4] = 0x72;
	buf[scrip_pos +5] = 0x61;
	buf[scrip_pos +6] = 0x6D;
	buf[scrip_pos +7] = 0x65;
	buf[scrip_pos +8] = 0x73;
	scrip_pos += 9;
	buf[scrip_pos] = 0x03;
	scrip_pos ++;

	//filepositions:13 
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x0D;
	scrip_pos += 2;
	buf[scrip_pos] = 0x66;
	buf[scrip_pos +1] = 0x69;
	buf[scrip_pos +2] = 0x6C;
	buf[scrip_pos +3] = 0x65;
	buf[scrip_pos +4] = 0x70;
	buf[scrip_pos +5] = 0x6F;
	buf[scrip_pos +6] = 0x73;
	buf[scrip_pos +7] = 0x69;
	buf[scrip_pos +8] = 0x74;
	buf[scrip_pos +9] = 0x69;
	buf[scrip_pos +10] = 0x6F;
	buf[scrip_pos +11] = 0x6E;
	buf[scrip_pos +12] = 0x73;
	scrip_pos += 13;
	buf[scrip_pos] = 0x0A;
	scrip_pos ++;
	//Arry_InFomation == 0x0A;  这个数组是在：keyframe中的
	//数组长度 4bytes 
	buf[scrip_pos] = filepositions_times_length >> 24;
	buf[scrip_pos +1] = (filepositions_times_length) >> 16 & 0xFF;
	buf[scrip_pos +2] = (filepositions_times_length) >> 8 & 0xFF;
	buf[scrip_pos +3] = filepositions_times_length & 0xFF;
	scrip_pos +=4;
	//将值考入数组
	for (int i = 0 ; i< filepositions_times_length ; i++)
	{
		buf[scrip_pos] = 0x00;
		scrip_pos ++;
		//8个字节的值
		double2char(buf + scrip_pos,scripttag.filepositions[i]);
		scrip_pos += 8;
	}
	
	//times :5
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x05;
	scrip_pos += 2;
	buf[scrip_pos] = 0x74;
	buf[scrip_pos +1] = 0x69;
	buf[scrip_pos +2] = 0x6D;
	buf[scrip_pos +3] = 0x65;
	buf[scrip_pos +4] = 0x73;
	scrip_pos += 5;
	buf[scrip_pos] = 0x0A;
	scrip_pos ++;
	//Arry_InFomation == 0x0A;  这个数组是在：keyframe中的
	//数组长度 4bytes 
	buf[scrip_pos] = filepositions_times_length >> 24;
	buf[scrip_pos +1] = (filepositions_times_length) >> 16 & 0xFF;
	buf[scrip_pos +2] = (filepositions_times_length) >> 8 & 0xFF;
	buf[scrip_pos +3] = filepositions_times_length & 0xFF;
	scrip_pos +=4;
	//将值考入数组
	for (int i = 0 ; i< filepositions_times_length ; i++)
	{
		buf[scrip_pos] = 0x00;
		scrip_pos ++;
		//8个字节的值
		double2char(buf + scrip_pos,scripttag.times[i]);
		scrip_pos += 8;
	}
	
	//写入tag末尾字符 00 00 00 09
	buf[scrip_pos] = 0x00;
	buf[scrip_pos + 1] = 0x00;
	buf[scrip_pos + 2] = 0x00;
	buf[scrip_pos + 3] = 0x09;
	scrip_pos +=4;

	scripttag.DataSize = scrip_pos - SCRIPT_TAG_HEADER_LENGTH;  //这里对于长度做更改

	//填写文件头buf
	buf[0] = scripttag.Type ;
	buf[1] = (scripttag.DataSize) >> 16;
	buf[2] = ((scripttag.DataSize) >> 8) & 0xFF;
	buf[3] = scripttag.DataSize & 0xFF;         
	buf[4] = (scripttag.Timestamp) >> 16;
	buf[5] = ((scripttag.Timestamp) >> 8) & 0xFF;
	buf[6] = scripttag.Timestamp & 0xFF; 
	buf[7] = scripttag.TimestampExtended;
	buf[8] = (scripttag.StreamID) >> 16;
	buf[9] = ((scripttag.StreamID) >> 8) & 0xFF;
	buf[10] = scripttag.StreamID & 0xFF; 
	buf[11] = scripttag.Type_1;
	buf[12] = scripttag.StringLength >> 8;
	buf[13] = scripttag.StringLength & 0xFF;
	buf[14] = 0x6F;  
	buf[15] = 0x6E;
	buf[16] = 0x4D;
	buf[17] = 0x65;
	buf[18] = 0x74;
	buf[19] = 0x61;
	buf[20] = 0x44;
	buf[21] = 0x61;
	buf[22] = 0x74;
	buf[23] = 0x61;
	buf[24] = scripttag.Type_2;
	buf[25] = scripttag.ECMAArrayLength >> 24;
	buf[26] = (scripttag.ECMAArrayLength >> 16) & 0xFF;
	buf[27] = (scripttag.ECMAArrayLength >> 8) & 0xFF;
	buf[28] = scripttag.ECMAArrayLength & 0xFF;

	return scripttag.DataSize + SCRIPT_TAG_HEADER_LENGTH ;
}