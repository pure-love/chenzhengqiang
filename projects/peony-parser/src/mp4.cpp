/*
*@filename:mp4.cpp
*@author:chenzhengqiang
*@start date:2016/01/25 13:50:23
*@modified date:
*@desc: 
*/



#include "mp4.h"
#include <arpa/inet.h>
#include <iostream>
using namespace std;

namespace czq
{
	namespace mp4
	{
		MP4Boxes * allocateMP4Boxes()
		{
			MP4Boxes * mp4Boxes = new MP4Boxes;
			mp4Boxes->ftypBox = 0;
			if ( mp4Boxes != 0 )
			{
				mp4Boxes->ftypBox = new FtypBox;
				if ( mp4Boxes->ftypBox != 0 )
				{
					mp4Boxes->ftypBox->boxHeader.fullBox = 0;
				}
				else
				{
					delete mp4Boxes;
					mp4Boxes = 0;
				}
			}
			return mp4Boxes;
		}
		
		void deallocateMP4Boxes( void * data)
		{
			MP4Boxes *mp4Boxes = static_cast<MP4Boxes *>(data);
			if ( mp4Boxes != 0 )
			{
				if ( mp4Boxes->ftypBox != 0 )
				{
					delete mp4Boxes->ftypBox;
				}

				delete mp4Boxes;
			}
		}


		MP4Boxes * onMediaMP4Parse(const char * fileName)
		{
			MP4Boxes * mp4Boxes = 0;
			FILE * fmp4Handler = fopen( fileName, "r");
			if ( fmp4Handler != 0 )
			{
				mp4Boxes = allocateMP4Boxes();
				if ( mp4Boxes != 0 )
				{
					bool ok = onFtypBoxParse(fmp4Handler, mp4Boxes->ftypBox);
					if ( ok )
					{
						;//do the other parsing
					}
				}
			}
			return mp4Boxes;
		}


		bool onFtypBoxParse(FILE *fmp4Handler, FtypBox * ftypBox)
		{
			bool ret = false;
			if ( fmp4Handler == 0 || ftypBox == 0 )
			{
				ret = false;
			}
			else
			{
				uint32_t size = 0;
				fread(&size, sizeof(size), 1, fmp4Handler);
				ftypBox->boxHeader.size = ntohl(size);
				ret = true;
			}
			return ret;
		}


		void onMP4InfoDump(void * data)
		{
			MP4Boxes * mp4Boxes = static_cast<MP4Boxes *>(data);
			if ( mp4Boxes != 0 )
			{
				cout<<"size:"<<mp4Boxes->ftypBox->boxHeader.size<<endl;
			}
		}
	};
};
