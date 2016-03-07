/*
*@filename:main.cpp
*@author:chenzhengqiang
*@start date:2016/01/25 14:24:42
*@modified date:
*@desc: 
*/



#include "peonymediaparser.h"
#include <iostream>
using namespace czq;


int main( int argc, char ** argv )
{
	if ( argc != 2 )
	{
		std::cerr<<"Usage:"<<argv[0]<<" <media file with suffix>"<<std::endl;
		std::cerr<<"Example:./peonyMediaParser test.ts"<<std::endl;
		return 1;
	}

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
