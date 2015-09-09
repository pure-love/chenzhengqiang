#include "script.h"

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

int AllocStruct_Script_Tag(Script_Tag ** scripttag)
{
	Script_Tag * scripttag_t = * scripttag;
	if ((scripttag_t = (Script_Tag *)calloc(1,sizeof(Script_Tag))) == NULL)
	{
		printf ("Error: Allocate Meory To AllocStruct_Script_Tag Buffer Failed ");
		return getchar();
	} 
	if ((scripttag_t->Data = (unsigned char * )calloc(ONE_SCRIPT_FRAME_SIZE,sizeof(unsigned char))) == NULL)
	{
		printf ("Error: Allocate Meory To scripttag_t->Data Buffer Failed ");
		return getchar();
	}
	* scripttag = scripttag_t;
	return 1;
}

int FreeStruct_Script_Tag(Script_Tag * scripttag)
{
	if (scripttag)
	{
		if (scripttag->Data)
		{
			free(scripttag->Data);
			scripttag->Data = NULL;
		}
		free(scripttag);
		scripttag = NULL;
	}
	return 1;
}

int ReadStruct_Script_Tag(unsigned char * Buf , unsigned int length ,Script_Tag * tag)
{
	int Script_Tag_pos = 0;
	int Arry_byte_length;
	unsigned char Arry_Name[MAX_ECMAARAY_NAME_LENGH];
	unsigned char Arry_InFomation;
	unsigned char Arry_InFomation_framekey;
	unsigned int  Arry_Name_framekey_Arry_length;

	//读取头，11字节
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
	Script_Tag_pos += 11;

	//读取第一个AMF包
	tag->Type_1 = Buf[Script_Tag_pos];
	Script_Tag_pos ++;
	if (tag->Type_1 == 0x02)
	{
		tag->StringLength = 
			Buf[Script_Tag_pos]   << 8 |
			Buf[Script_Tag_pos+1];
		Script_Tag_pos +=2;
		//查找信息，固定为0x6F 0x6E 0x4D 0x65 0x74 0x64 0x44 0x61 0x74 0x61，表示字符串onMetaData

		Script_Tag_pos +=tag->StringLength;
	}
	//读取第二个AMF包
	tag->Type_1 = Buf[Script_Tag_pos];
	Script_Tag_pos ++;
	if (tag->Type_1 == 0x08)
	{
		tag->ECMAArrayLength =                   //表示接下来的metadata array data 中有多少组数据
			Buf[Script_Tag_pos]     << 24 |
			Buf[Script_Tag_pos+1]   << 16 |
			Buf[Script_Tag_pos+2]   << 8  |
			Buf[Script_Tag_pos+3];
		Script_Tag_pos += 4;
	}

	for (int i = 0 ; i< tag->ECMAArrayLength ; i++)  //一共有多少组数据，即有多少个类似：宽，高，采样率这样的信息
	{
		//首先判断下是不是遇到了Script_Tag的末尾标志，有可能会出现 数组的个数 < tag->ECMAArrayLength 的情况
	    if (Buf[Script_Tag_pos]  == 0x00 && Buf[Script_Tag_pos + 1]  == 0x00 && Buf[Script_Tag_pos + 2]  == 0x00 && Buf[Script_Tag_pos + 3]  == 0x09)
		{
			break;
		}

		//前面2bytes表示，第N个数组的名字所占的bytes
loop:	Arry_byte_length = 
			Buf[Script_Tag_pos]   << 8  |
			Buf[Script_Tag_pos+1];
		Script_Tag_pos +=2;

		memcpy(Arry_Name,Buf + Script_Tag_pos , Arry_byte_length);  //拷贝数组名称
		Script_Tag_pos += Arry_byte_length;

		Arry_InFomation = Buf[Script_Tag_pos];                      //跟着下去的1bytes表示这个数组的属性信息
		Script_Tag_pos ++;

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
			tag->duration= char2double(&Buf[Script_Tag_pos],8);
			Script_Tag_pos += 8;
		}
		else if (strstr((char *)Arry_Name,"width") != NULL)
		{
			//Arry_InFomation == 0;
			tag->width= char2double(&Buf[Script_Tag_pos],8);
			Script_Tag_pos += 8;
		}
		else if (strstr((char *)Arry_Name,"height") != NULL)
		{
			//Arry_InFomation == 0;
			tag->height = char2double(&Buf[Script_Tag_pos],8);
			Script_Tag_pos += 8;
		}
		else if (strstr((char *)Arry_Name,"videodatarate") != NULL)
		{
			//Arry_InFomation == 0;
			tag->videodatarate = char2double(&Buf[Script_Tag_pos],8);
			Script_Tag_pos += 8;
		}
		else if (strstr((char *)Arry_Name,"framerate") != NULL)
		{
			//Arry_InFomation == 0;
			tag->framerate = char2double(&Buf[Script_Tag_pos],8);	
			Script_Tag_pos += 8;
		}
		else if (strstr((char *)Arry_Name,"videocodecid") != NULL)
		{
			//Arry_InFomation == 0;
			tag->videocodecid = char2double(&Buf[Script_Tag_pos],8);
			Script_Tag_pos += 8;
		}
		else if (strstr((char *)Arry_Name,"audiosamplerate") != NULL)
		{
			//Arry_InFomation == 0;
			tag->audiosamplerate = char2double(&Buf[Script_Tag_pos],8);
			Script_Tag_pos += 8;
		}
		else if (strstr((char *)Arry_Name,"audiodatarate") != NULL)
		{
			//Arry_InFomation == 0;
			tag->audiodatarate = char2double(&Buf[Script_Tag_pos],8);
			Script_Tag_pos += 8;
		}
		else if (strstr((char *)Arry_Name,"audiosamplesize") != NULL)
		{
			//Arry_InFomation == 0;
			tag->audiosamplesize = char2double(&Buf[Script_Tag_pos ],8);
			Script_Tag_pos += 8;
		}
		else if (strstr((char *)Arry_Name,"stereo") != NULL)
		{
			//Arry_InFomation == 1;
			tag->stereo = Buf[Script_Tag_pos];
			Script_Tag_pos ++;
		}
		else if (strstr((char *)Arry_Name,"audiocodecid") != NULL)
		{
			//Arry_InFomation == 0;
			tag->audiocodecid = char2double(&Buf[Script_Tag_pos],8);
			Script_Tag_pos += 8;
		}
		else if (strstr((char *)Arry_Name,"filesize") != NULL)
		{
			//Arry_InFomation == 0;
			tag->filesize = char2double(&Buf[Script_Tag_pos],8);
			Script_Tag_pos += 8;
		}
		else if (strstr((char *)Arry_Name,"lasttime") != NULL)
		{
			//Arry_InFomation == 0;
			tag->lasttimetamp = char2double(&Buf[Script_Tag_pos],8);
			Script_Tag_pos += 8;
		}
		else if (strstr((char *)Arry_Name,"lastkeyframetime") != NULL)
		{
			//Arry_InFomation == 0;
			tag->lastkeyframetimetamp = char2double(&Buf[Script_Tag_pos],8);
			Script_Tag_pos += 8;
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
			Arry_Name_framekey_Arry_length = 
				Buf[Script_Tag_pos]      << 24 |
				Buf[Script_Tag_pos + 1]  << 16 |
				Buf[Script_Tag_pos + 2]  << 8  |
				Buf[Script_Tag_pos + 3];
			Script_Tag_pos += 4;
			//将值考入数组
			for (int k = 0 ; k < Arry_Name_framekey_Arry_length ; k ++ )
			{
				Arry_InFomation_framekey =    Buf[Script_Tag_pos];               //类型
				//Arry_InFomation_framekey == 0x00;
				Script_Tag_pos ++;
				tag->filepositions[i]= char2double(&Buf[Script_Tag_pos],8);      //值
				Script_Tag_pos += 8;
			}
			//注意这个不算是 ECMAArrayLength里面的一种
			i --;
		}
		else if ((strstr((char *)Arry_Name,"times") != NULL) && Arry_InFomation == 0x0A)
		{
			//Arry_InFomation == 0x0A;  这个数组是在：keyframe中的
			//数组长度 4bytes  
			Arry_Name_framekey_Arry_length = 
				Buf[Script_Tag_pos]      << 24 |
				Buf[Script_Tag_pos + 1]  << 16 |
				Buf[Script_Tag_pos + 2]  << 8  |
				Buf[Script_Tag_pos + 3];
			Script_Tag_pos += 4;
			//将值考入数组
			for (int k = 0 ; k < Arry_Name_framekey_Arry_length ; k ++ )
			{
				Arry_InFomation_framekey = Buf[Script_Tag_pos];          //类型
				//Arry_InFomation_framekey == 0x00;
				Script_Tag_pos ++;
				tag->times[i]= char2double(&Buf[Script_Tag_pos],8);      //值
				Script_Tag_pos += 8;
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
				Script_Tag_pos += 8;
				break;
			case 0x01:
				Script_Tag_pos ++;
				break;
			case 0x02:
				Script_Tag_pos += 
					Buf[Script_Tag_pos]  << 8 |
					Buf[Script_Tag_pos+1];
				Script_Tag_pos +=2;
				break;
			case 0x03:
				goto loop;
				break;
			case 0x04:
				//暂时不作处理 一般不能遇到
				break;
			case 0x07:
				Script_Tag_pos += 2;
				break;
			case 0x08:
				//暂时不作处理 一般不能遇到
				break;
			case 0x0A:
				Script_Tag_pos += 4;
				break;
			case 0x0B:
				Script_Tag_pos += 10;
				break;
			case 0x0C:
				//暂时不作处理 一般不能遇到
				break;
			default:
                //有可能中间出现 00 00 00 09 结束符，继续向下读取
				printf("Arry_InFomation 继续向下读取\n");
				break;
			}
		}
	}
	//这个data里面的数据 是什么还未知道
	memcpy(tag->Data,Buf + Script_Tag_pos,length - Script_Tag_pos );
	return 1;
}