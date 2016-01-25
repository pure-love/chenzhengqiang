/*
*@filename:main.cpp
*@author:chenzhengqiang
*@start date:2016/01/25 14:24:42
*@modified date:
*@desc: 
*/



#include "peonymediaparser.h"
using namespace czq;

int main( int argc, char ** argv )
{
	(void)argc;
	peony::PMFormatContext * pmFormatContext= peony::pmAllocFormatContext();
	if ( pmFormatContext != 0 )
	{
		peony::pmGuessFormat(argv[1], pmFormatContext);
		peony::MediaInfo * mediaInfo = peony::pmRetrieveMediaInfo(pmFormatContext);
		peony::pmDumpMediaInfo(mediaInfo);
		peony::pmFreeMediaInfo(mediaInfo);
	}
	
	peony::pmFreeFormatContext(pmFormatContext);
	return 0;
}
