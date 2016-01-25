/*
*@filename:mp4.h
*@author:chenzhengqiang
*@start date:2016/01/22 16:42:50
*@modified date:
*@desc: 
*/



#ifndef _CZQ_MP4_H_
#define _CZQ_MP4_H_
//write the function prototypes or the declaration of variables here
#include<stdint.h>
namespace czq
{
	namespace mp4
	{
		struct Boxheader
		{
			unsigned char fullBox:1;
			uint32_t size;
			uint8_t type[4];
			uint8_t version;
			uint8_t flags[3];
			uint8_t largeSize[8];
			uint8_t uuid[16];
		};

		struct FtypBox
		{
			Boxheader boxHeader;
			uint8_t majorBrand[4];
			uint8_t minorVersion[4];
			uint8_t comtableBrands[12];
		};

		struct MP4
		{
			FtypBox *ftypeBox;
		};
			
	};
};
#endif
