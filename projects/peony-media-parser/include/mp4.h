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
#include<string>

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
			uint8_t reserved1[4];
			uint32_t duration;
			uint8_t reserved2[8];
			uint8_t layer[2];
			uint8_t alternateGroup[2];
			uint8_t volume[2];
			uint8_t reserved3[2];
			uint8_t matrix[36];
			uint8_t width[4];
			uint8_t height[4];
		};


		struct MdhdBox
		{
			Boxheader boxHeader;
			uint32_t creationTime;
			uint32_t modificationTime;
			uint32_t timescale;
			uint32_t duration;
			uint8_t language[2];
			uint8_t predefined[2];
			
		};

		struct HdlrBox
		{
			Boxheader boxHeader;
			uint8_t componentType[4];
			uint8_t componentSubType[4];
			uint8_t reserved[12];
			std::string name;
		};

		struct VmhdBox
		{
			Boxheader boxHeader;
			uint8_t graphics[2];
			uint8_t opcolor[2*3];
		};

		struct SmhdBox
		{
			Boxheader boxHeader;
			uint8_t balance[2];
			uint8_t reserved[2];
		};

		struct HmhdBox
		{
			Boxheader boxHeader;
		};

		struct NmhdBox
		{
			Boxheader boxHeader;
		};

		struct DrefBox
		{
			Boxheader boxHeader;
			uint32_t entryCount;
			//the urls' size is size -16
			uint8_t *urls;
		};
		
		struct DinfBox
		{
			uint32_t size;
			uint8_t type[4];
			DrefBox * drefBox;
		};

		struct StsdBox
		{
			Boxheader boxHeader;
		};
		
		struct SttsBox
		{
			Boxheader boxHeader;
		};
		
		struct StscBox
		{
			Boxheader boxHeader;
		};
		
		struct StszBox
		{
			Boxheader boxHeader;
		};
		
		struct StcoBox
		{
			Boxheader boxHeader;
		};
		
		struct StssBox
		{
			Boxheader boxHeader;
		};
		
		struct StblBox
		{
			uint32_t size;
			uint8_t type[4];
			StsdBox *stsdBox;
			SttsBox *sttsBox;
			StscBox *stscBox;
			StszBox *stszBox;
			StcoBox *stcoBox;
			StssBox *stssBox;
		};
		
		struct MinfBox
		{
			uint32_t size;
			uint8_t type[4];
			VmhdBox *vmhdBox;
			SmhdBox *smhdBox;
			HmhdBox *hmhdBox;
			NmhdBox *nmhdBox;
			DinfBox 	*dinfBox; 
			StblBox *stblBox;
		};

		
		struct MdiaBox
		{
			uint32_t size;
			uint8_t type[4];
			MdhdBox * mdhdBox;
			HdlrBox * hdlrBox;
			MinfBox * minfBox;
		};
		
		struct TrakBox
		{
			uint32_t size;
			uint8_t type[4];
			TkhdBox *tkhdBox;
			MdiaBox *mdiaBox;
			TrakBox * next;
		};

		struct UdtaBox
		{
			uint32_t size;
			uint8_t type[4];
			//data's size is size -8
			uint8_t *data;
		};
		
		struct MoovBox
		{
			uint32_t size;
			uint8_t type[4];//it should be moov
			MvhdBox *mvhdBox;
			IodsBox  *iodsBox;
			TrakBox *traks;
			UdtaBox *udtaBox;
		};

		struct MdtaBox
		{
			uint32_t size;
			uint8_t type[4];
			//data's size is size-8
			uint8_t * data;
		};
		
		struct MP4Boxes
		{
			FtypBox  *ftypBox;
			MoovBox *moovBox;
			MdtaBox *mdtaBox;
		};

		MP4Boxes * allocateMP4Boxes();
		FtypBox * allocateFtypBox();
		MoovBox * allocateMoovBox();
		MdtaBox * allocateMdtaBox();
		void deallocatePackage( void * data);
		void deallocateFtypBox(void *data);
		void deallocateMoovBox(void *data);
		void deallocateMdtaBox(void *data);
		MP4Boxes * onMediaParse(const char * fileName);
		bool onFtypBoxParse(uint8_t *buffer, uint32_t size, FtypBox * ftypBox);
		bool onMoovBoxParse(uint8_t *buffer, uint32_t size, MoovBox * moovBox);
		bool onMdtaBoxParse(uint8_t *buffer, uint32_t size, MdtaBox * mdtaBox);
		void onMediaInfoDump(void * data);
	};
};
#endif
