/*
*@filename:peonymediaparser.h
*@author:chenzhengqiang
*@start date:2016/01/22 16:42:58
*@modified date:
*@desc: 
*/



#ifndef _CZQ_PEONYMEDIAPARSER_H_
#define _CZQ_PEONYMEDIAPARSER_H_
//write the function prototypes or the declaration of variables here
#include "mp4.h"
namespace czq
{
	namespace peony
	{
		//enum the media type
		enum PeonyMediaType
		{
			PEONY_MEDIA_MP4, //for mp4 media file
			PEONY_MEDIA_FLV,//for flv media file
			PEONY_MEDIA_TS,//for mpeg2-ts file
			PEONY_MEDIA_NONE//for nothing
		};
		
		struct PMFormatContext
		{
			//the media type
			PeonyMediaType mediaType;
			//for storing the media file name
			char fileName[1024];
		};

		struct MediaInfo
		{
			PeonyMediaType type;
			void *data;
		};
		
		//the prefix pm stands for the peony media
		PMFormatContext * pmAllocFormatContext();
		void pmFreeFormatContext( PMFormatContext * pmFormatContext );
		
		//get the media file's type by simply parsing the file's suffix
		int pmGuessFormat(const char * fileName, PMFormatContext * pmFormatContext);
		MediaInfo * pmAllocMediaInfo();
		MediaInfo * pmRetrieveMediaInfo(const PMFormatContext * pmFormatContext);
		void pmFreeMediaInfo( MediaInfo * mediaInfo );
		void pmDumpMediaInfo(const MediaInfo * mediaInfo );
	};
};
#endif
