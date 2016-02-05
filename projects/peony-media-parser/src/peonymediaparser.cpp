/*
*@filename:peonymediaparser.cpp.cpp
*@author:chenzhengqiang
*@start date:2016/01/25 12:28:51
*@modified date:
*@desc: 
*/


#include "errors.h"
#include  "peonymediaparser.h"
#include "mp4.h"
#include "ts.h"
#include <cstring>
#include <cstdio>

namespace czq
{
	namespace peony
	{
		PMFormatContext * pmAllocFormatContext()
		{
			PMFormatContext * pmFormatContext = new PMFormatContext;
			if ( pmFormatContext != 0 )
			{
				pmFormatContext->mediaType = PEONY_MEDIA_NONE;
			}

			return pmFormatContext;
		}

		
		void pmFreeFormatContext( PMFormatContext * pmFormatContext )
		{
			if ( pmFormatContext != 0 )
			{
				;//do the clean here for expansion's sake
				delete pmFormatContext;
				pmFormatContext = 0;
			}
		}

		//get the media type by simply parsing the media file's suffix
		//.mp4 .flv .ts
		//and if the media file have no suffix,then you can use the complex way
		int pmGuessFormat(const char * fileName, PMFormatContext * pmFormatContext)
		{
			if ( fileName == 0 || pmFormatContext == 0 )
			{
				return ARGUMENT_ERROR;
			}
			
			const char *suffix = fileName+(strlen(fileName)-4);
			PeonyMediaType mediaType;
			strncpy(pmFormatContext->fileName, fileName, sizeof(pmFormatContext->fileName));
			if ( strncmp(suffix,".mp4", 4) == 0)
			{
				mediaType = PEONY_MEDIA_MP4;
			}
			else if ( strncmp(suffix, ".flv", 4) == 0 )
			{
				mediaType = PEONY_MEDIA_FLV;
			}
			else if ( strncmp(suffix+1, ".ts", 3) == 0)
			{
				mediaType = PEONY_MEDIA_TS;
			}
			else
			{
				mediaType = PEONY_MEDIA_NONE;
			}
			
			pmFormatContext->mediaType = mediaType;;
			return OK;	
		}


		MediaInfo * pmAllocMediaInfo()
		{
			MediaInfo * mediaInfo = new MediaInfo;
			if ( mediaInfo != 0 )
			{
				mediaInfo->type = PEONY_MEDIA_NONE;
				mediaInfo->data = 0;
			}
			return mediaInfo;
		}

		void pmFreeMediaInfo( MediaInfo * mediaInfo )
		{
			if ( mediaInfo != 0 )
			{
				if ( mediaInfo->data != 0 )
				{
					switch ( mediaInfo->type )
					{
						case PEONY_MEDIA_MP4:
							mp4::deallocatePackage(mediaInfo->data);
							break;
						case PEONY_MEDIA_TS:
							mpeg::deallocatePackage(mediaInfo->data);
							break;
						case PEONY_MEDIA_FLV:
							break;
						default:
							break;
					}
					
				}

				delete mediaInfo;
			}
		}

		
		MediaInfo * pmRetrieveMediaInfo(const PMFormatContext * pmFormatContext)
		{	
			MediaInfo * mediaInfo = 0;
			if ( pmFormatContext != 0 )
			{
				mediaInfo = pmAllocMediaInfo();
				if ( mediaInfo != 0 )
				{
					mediaInfo->type = pmFormatContext->mediaType;
					switch ( mediaInfo->type )
					{
						case PEONY_MEDIA_MP4:
							mediaInfo->data = static_cast<void *>(mp4::onMediaParse(pmFormatContext->fileName));
							break;
						case PEONY_MEDIA_FLV:
							break;
						case PEONY_MEDIA_TS:
							mediaInfo->data = static_cast<void *>(mpeg::onMediaParse(pmFormatContext->fileName));
							break;
						default:
							break;
					}
				}
			}
			return mediaInfo;
		}

		
		void pmDumpMediaInfo(const MediaInfo * mediaInfo )
		{
			if ( mediaInfo != 0 )
			{
				switch ( mediaInfo->type )
				{
					case PEONY_MEDIA_MP4:
						mp4::onMediaInfoDump(mediaInfo->data);
						break;
					case PEONY_MEDIA_FLV:
						break;
					case PEONY_MEDIA_TS:
						mpeg::onMediaInfoDump(mediaInfo->data);
						break;
					default:
						break;
				}
			}
		}
	};
};
