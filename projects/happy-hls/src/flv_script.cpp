/*
@author:internet
@modified by:chenzhengqiang
@start date:2015/9/10
@modified date:
@desc:
*/

#include "flv_script.h"
#include<cstdlib>
#include<cstring>
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
}


int get_flv_script_tag( unsigned char *flv_tag_header, unsigned char * flv_tag_data , unsigned int tag_data_size ,FLV_SCRIPT_TAG & script_tag)
{
	int Arry_byte_tag_data_size;
	unsigned char Arry_Name[MAX_ECMAARAY_NAME_LENGH];
	unsigned char Arry_InFomation;
	unsigned char Arry_InFomation_framekey;
	unsigned int  Arry_Name_framekey_Arry_tag_data_size;

	//读取头，11字节
	script_tag.Type = flv_tag_header[0];
	script_tag.DataSize = flv_tag_header[1]  << 16 |flv_tag_header[2]  << 8  |flv_tag_header[3];
	script_tag.Timestamp = flv_tag_header[4]  << 16 |flv_tag_header[5]  << 8  |flv_tag_header[6];
	script_tag.TimestampExtended = flv_tag_header[7];
	script_tag.StreamID = flv_tag_header[8]  << 16 |flv_tag_header[9]  << 8  |flv_tag_header[10];
    
	//读取第一个AMF
	int pos = 0;
	script_tag.Type_1 = flv_tag_data[pos];
	pos ++;
	if (script_tag.Type_1 == 0x02)
	{
		script_tag.StringLength = 
			flv_tag_data[pos]   << 8 |
			flv_tag_data[pos+1];
		pos +=2;
		//查找信息，固定为0x6F 0x6E 0x4D 0x65 0x74 0x64 0x44 0x61 0x74 0x61，表示字符串onMetaData

		pos +=script_tag.StringLength;
	}
	//读取第二个AMF包
	script_tag.Type_1 = flv_tag_data[pos];
	pos ++;
	if (script_tag.Type_1 == 0x08)
	{
		script_tag.ECMAArrayLength =                   //表示接下来的metadata array data 中有多少组数据
			flv_tag_data[pos]     << 24 |
			flv_tag_data[pos+1]   << 16 |
			flv_tag_data[pos+2]   << 8  |
			flv_tag_data[pos+3];
		pos += 4;
	}

	for ( unsigned int i = 0 ; i< script_tag.ECMAArrayLength ; i++ )  //一共有多少组数据，即有多少个类似：宽，高，采样率这样的信息
	{
		//首先判断下是不是遇到了Script_Tag的末尾标志，有可能会出现 数组的个数 < script_tag.ECMAArrayLength 的情况
	    if (flv_tag_data[pos]  == 0x00 && flv_tag_data[pos + 1]  == 0x00 && flv_tag_data[pos + 2]  == 0x00 && flv_tag_data[pos + 3]  == 0x09)
		{
			break;
		}
		//前面2bytes表示，第N个数组的名字所占的bytes
loop:	Arry_byte_tag_data_size = 
			flv_tag_data[pos]   << 8  |
			flv_tag_data[pos+1];
		pos +=2;

		memcpy(Arry_Name,flv_tag_data + pos , Arry_byte_tag_data_size);  //拷贝数组名称
		pos += Arry_byte_tag_data_size;

		Arry_InFomation = flv_tag_data[pos];                      //跟着下去的1bytes表示这个数组的属性信息
		pos ++;

		/* //Arry_InFomation的值说明
		If Type == 0
		DOUBLE    //随后的8bytes表示该属性对应的float值
		If Type == 1
		UI8       //随后的1bytes表示boolean，比如是否有视频为01表示“含有”的意思
		If Type == 2
		SCRIPTDATASTRING  //后跟的2bytes表示字符串长度，然后再根据这个长度从后边的bytes中读取出字符串
		If Type == 3
		SCRIPTDATAOBJECT[n]  //数组信息，里面名称 然后循环上面//前面2bytes表示，第N个数组的名字所占的bytes这样的操作
		If Type == 4
		SCRIPTDATASTRING     //暂时不作处理
		defining
		the MovieClip path
		If Type == 7
		UI16                 //接下来的2bytes代表所要的数据
		If Type == 8
		SCRIPTDATAVARIABLE[ECMAArrayLength]  //暂时不作处理
		If Type == 10
		SCRIPTDATAVARIABLE[n]   //数据变量，占4bytes
		If Type == 11
		SCRIPTDATADATE          //占10bytes，表示日期
		If Type == 12
		SCRIPTDATALONGSTRING    //暂时不做处理
		*/

		if (strstr((char *)Arry_Name,"duration") != NULL)           
		{
			//Arry_InFomation == 0
			script_tag.duration= char2double(&flv_tag_data[pos],8);
			pos += 8;
		}
		else if (strstr((char *)Arry_Name,"width") != NULL)
		{
			//Arry_InFomation == 0;
			script_tag.width= char2double(&flv_tag_data[pos],8);
			pos += 8;
		}
		else if (strstr((char *)Arry_Name,"height") != NULL)
		{
			//Arry_InFomation == 0;
			script_tag.height = char2double(&flv_tag_data[pos],8);
			pos += 8;
		}
		else if (strstr((char *)Arry_Name,"videodatarate") != NULL)
		{
			//Arry_InFomation == 0;
			script_tag.videodatarate = char2double(&flv_tag_data[pos],8);
			pos += 8;
		}
		else if (strstr((char *)Arry_Name,"framerate") != NULL)
		{
			//Arry_InFomation == 0;
			script_tag.framerate = char2double(&flv_tag_data[pos],8);	
			pos += 8;
		}
		else if (strstr((char *)Arry_Name,"videocodecid") != NULL)
		{
			//Arry_InFomation == 0;
			script_tag.videocodecid = char2double(&flv_tag_data[pos],8);
			pos += 8;
		}
		else if (strstr((char *)Arry_Name,"audiosamplerate") != NULL)
		{
			//Arry_InFomation == 0;
			script_tag.audiosamplerate = char2double(&flv_tag_data[pos],8);
			pos += 8;
		}
		else if (strstr((char *)Arry_Name,"audiodatarate") != NULL)
		{
			//Arry_InFomation == 0;
			script_tag.audiodatarate = char2double(&flv_tag_data[pos],8);
			pos += 8;
		}
		else if (strstr((char *)Arry_Name,"audiosamplesize") != NULL)
		{
			//Arry_InFomation == 0;
			script_tag.audiosamplesize = char2double(&flv_tag_data[pos ],8);
			pos += 8;
		}
		else if (strstr((char *)Arry_Name,"stereo") != NULL)
		{
			//Arry_InFomation == 1;
			script_tag.stereo = flv_tag_data[pos];
			pos ++;
		}
		else if (strstr((char *)Arry_Name,"audiocodecid") != NULL)
		{
			//Arry_InFomation == 0;
			script_tag.audiocodecid = char2double(&flv_tag_data[pos],8);
			pos += 8;
		}
		else if (strstr((char *)Arry_Name,"filesize") != NULL)
		{
			//Arry_InFomation == 0;
			script_tag.filesize = char2double(&flv_tag_data[pos],8);
			pos += 8;
		}
		else if (strstr((char *)Arry_Name,"lasttime") != NULL)
		{
			//Arry_InFomation == 0;
			script_tag.lasttimetamp = char2double(&flv_tag_data[pos],8);
			pos += 8;
		}
		else if (strstr((char *)Arry_Name,"lastkeyframetime") != NULL)
		{
			//Arry_InFomation == 0;
			script_tag.lastkeyframetimetamp = char2double(&flv_tag_data[pos],8);
			pos += 8;
		}
		else if ((strstr((char *)Arry_Name,"keyframe") != NULL) && Arry_InFomation == 0x03)   //如果是关键帧信息
		{
			//Arry_InFomation == 0x03; 
			goto loop;
		}
		else if ((strstr((char *)Arry_Name,"filepositions") != NULL)&& Arry_InFomation == 0x0A)
		{
			//Arry_InFomation == 0x0A;  这个数组是在：keyframe中的
			//数组长度 4bytes  
			Arry_Name_framekey_Arry_tag_data_size = 
				flv_tag_data[pos]      << 24 |
				flv_tag_data[pos + 1]  << 16 |
				flv_tag_data[pos + 2]  << 8  |
				flv_tag_data[pos + 3];
			pos += 4;
			//将值考入数组
			for (unsigned int k = 0 ; k < Arry_Name_framekey_Arry_tag_data_size ; k ++ )
			{
				Arry_InFomation_framekey =    flv_tag_data[pos];               //类型
				//Arry_InFomation_framekey == 0x00;
				pos ++;
				script_tag.filepositions[i]= char2double(&flv_tag_data[pos],8);      //值
				pos += 8;
			}
			//注意这个不算是 ECMAArrayLength里面的一种
			i --;
		}
		else if ((strstr((char *)Arry_Name,"times") != NULL) && Arry_InFomation == 0x0A)
		{
			//Arry_InFomation == 0x0A;  这个数组是在：keyframe中的
			//数组长度 4bytes  
			Arry_Name_framekey_Arry_tag_data_size = 
				flv_tag_data[pos]      << 24 |
				flv_tag_data[pos + 1]  << 16 |
				flv_tag_data[pos + 2]  << 8  |
				flv_tag_data[pos + 3];
			pos += 4;
			//将值考入数组
			for ( unsigned int k = 0 ; k < Arry_Name_framekey_Arry_tag_data_size ; k ++ )
			{
				Arry_InFomation_framekey = flv_tag_data[pos];          //类型
				//Arry_InFomation_framekey == 0x00;
				pos ++;
				script_tag.times[i]= char2double(&flv_tag_data[pos],8);      //值
				pos += 8;
			}
			//注意这个不算是 ECMAArrayLength里面的一种
			i --;
		}
		else
		{
			//暂时不做读取但是需要将buf指针向后移动
			switch (Arry_InFomation)
			{
			case 0x00:
				pos += 8;
				break;
			case 0x01:
				pos ++;
				break;
			case 0x02:
				pos += 
					flv_tag_data[pos]  << 8 |
					flv_tag_data[pos+1];
				pos +=2;
				break;
			case 0x03:
				goto loop;
				break;
			case 0x04:
				//暂时不作处理 一般不能遇到
				break;
			case 0x07:
				pos += 2;
				break;
			case 0x08:
				//暂时不作处理 一般不能遇到
				break;
			case 0x0A:
				pos += 4;
				break;
			case 0x0B:
				pos += 10;
				break;
			case 0x0C:
				break;
			default:
				break;
			}
		}
	}
	
	memcpy(script_tag.Data,flv_tag_data + pos,tag_data_size - pos );
	return 1;
}