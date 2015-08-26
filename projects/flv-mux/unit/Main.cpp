#include "Mux.h"

//#define  WIDTH  704.00          //宽 
//#define  HEIGHT 368.00          //高
//#define  FRAMERATE 24.70        //视频帧率

#define  WIDTH  720.00          //宽 
#define  HEIGHT 468.00          //高
#define  FRAMERATE 15.00        //视频帧率


int main()
{
	printf("--------程序运行开始----------\n");
	pVideo_H264_File = OpenFile(INPUTH264FILENAME,"rb");
	pAudio_Aac_File = OpenFile(INPUTAACFILENAME,"rb");
	pVideo_Audio_Flv_File = OpenFile(OUTPUTFLVFILENAME,"wb");

	//////////////////////////////////////////////////////////////////////////
	WriteBuf2File(WIDTH ,HEIGHT,FRAMERATE);
	//////////////////////////////////////////////////////////////////////////

	if (pVideo_H264_File)
	{
		CloseFile(pVideo_H264_File);
		pVideo_H264_File = NULL;
	}
	if (pAudio_Aac_File)
	{
		CloseFile(pAudio_Aac_File);
		pAudio_Aac_File = NULL;
	}
	if (pVideo_Audio_Flv_File)
	{
		CloseFile(pVideo_Audio_Flv_File);
		pVideo_Audio_Flv_File = NULL;
	}
	printf("--------程序运行结束----------\n");
	printf("-------请按任意键退出---------\n");
	return getchar();
}