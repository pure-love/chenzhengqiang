/*
*@filename:ts.h
*@author:chenzhengqiang
*@start date:2016/02/05 12:24:28
*@modified date:
*@desc: 
*/



#ifndef _CZQ_TS_H_
#define _CZQ_TS_H_
//write the function prototypes or the declaration of variables here
#include <stdint.h>
namespace czq
{
	namespace mpeg
	{
		struct TSPackageHeader
		{
			unsigned int syncByte:8;
			unsigned int errorIndicator:1;
			unsigned int payloadUnitStartIndicator:1;
			unsigned int priority:1;
			unsigned int PID:13;
			unsigned int scramblingControl:2;
			unsigned int adaptationFieldControl:2;
			unsigned int continuityCounter:4;
		};
		
		struct TSPackage
		{
			TSPackageHeader tsPackageHeader;
		};

		TSPackage * allocateTSPackage();
		void deallocatePackage( void * data);
		TSPackage * onMediaParse(const char * fileName);
		bool onTSPackageHeaderParse(uint8_t *buffer, uint32_t bufferSize, TSPackage *tsPackage);
		void onMediaInfoDump( void * data );
	};
};
#endif
