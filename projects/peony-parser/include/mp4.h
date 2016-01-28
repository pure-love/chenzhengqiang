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
#include<cstdio>

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
			uint8_t *largeSize;
			uint8_t *uuid;
		};

		struct FtypBox
		{
			Boxheader boxHeader;
			uint8_t majorBrand[4];
			uint8_t minorVersion[4];
			uint8_t compatibleBrands[12+1];
		};

		struct MvhdBox
		{
			Boxheader boxHeader;
			uint32_t creationTime;
			uint32_t modificationTime;
			uint32_t timescale;
			uint32_t duration;
			uint8_t rate[4];
			uint8_t volume[2];
			uint8_t reserved[10];
			uint8_t matrix[36];
			uint8_t predefined[24];
			uint32_t nextTrakID;
		};

		struct IodsBox
		{
			uint32_t size;
			uint8_t type[4];
			//data's size is size-8
			uint8_t * data;
		};

		struct TkhdBox
		{
			Boxheader boxHeader;
			uint32_t creationTime;
			uint32_t modificationTime;
			uint32_t trakID;
		};
		
		struct TrakBox
		{
			uint32_t size;
			uint8_t type[4];
			TkhdBox *tkhdBox;
			TrakBox * next;
		};
		
		struct MoovBox
		{
			uint32_t size;
			uint8_t type[4];//it should be moov
			MvhdBox *mvhdBox;
			IodsBox  *iodsBox;
			TrakBox *traks;
		};
		
		struct MP4Boxes
		{
			FtypBox  *ftypBox;
			MoovBox *moovBox;
		};

		MP4Boxes * allocateMP4Boxes();
		void deallocateMP4Boxes( void * data);
		MP4Boxes * onMediaMP4Parse(const char * fileName);
		bool onFtypBoxParse(uint8_t *buffer, uint32_t size, FtypBox * ftypBox);
		bool onMoovBoxParse(uint8_t *buffer, uint32_t size, MoovBox * moovBox);
		void onMP4InfoDump(void * data);
	};
};
#endif
