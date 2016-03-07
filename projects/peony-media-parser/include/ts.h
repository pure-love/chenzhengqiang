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

		struct TSAdaptationFieldExtension
		{
			unsigned int length:8;
			unsigned int ltwFlag:1;
			unsigned int piecewiseRateFlag:1;
			unsigned int seamlessSpliceFlag:1;
			unsigned int reserved1:5;
			//if ltwFlag is 1
			unsigned int ltwValidFlag:1;
			unsigned int ltwOffset:15;

			//if pieceWiseRateFlag is 1
			unsigned int reserved2:2;
			unsigned int piecewiseRate:22;

			//ifseamlessSpliceFlag is 1
			unsigned int spliceType:4;
			unsigned int DTS32_30:3;
			unsigned int markerBit1:1;
			unsigned int DTS29_15:15;
			unsigned int markerBit2:1;
			unsigned int DTS14_0:15;
			unsigned int markerBit3:1;
		};

		
		struct TSAdaptationField
		{
			unsigned int length:8;
			unsigned int discontinuityIndicator:1;
			unsigned int randomAccessIndicator:1;
			unsigned int elementaryStreamPriorityIndicator:1;
			unsigned int PCRFlag:1;
			unsigned int OPCRFlag:1;
			unsigned int splicingPointFlag:1;
			unsigned int transportPrivateDataFlag:1;
			unsigned int extensionFlag:1;
			//the pcr base holds 33 bits
			unsigned int PCRbase[5];
			unsigned int PCRreserved:6;
			unsigned int PCRextension:9;
			//the original pcr base alse holds 33 bits
			unsigned int OPCRbase[5];
			unsigned int OPCRreserved:6;
			unsigned int OPCRextension:9;
			unsigned int spliceCountdown:8;
			
			//the first byte of transportPrivateDataBuffer holds the buffer's length
			uint8_t * transportPrivateDataBuffer;
			//also the first byte of adaptationFieldExtensionBuffer hlods the buffer's length
			TSAdaptationFieldExtension * TAFextension;
		};
		
		struct TSPackage
		{
			TSPackageHeader tsPackageHeader;
			TSAdaptationField * adaptationField;
		};

		TSPackage * allocateTSPackage();
		void deallocatePackage( void * data);
		TSPackage * onMediaParse(const char * fileName);
		bool onTSPackageHeaderParse(uint8_t *buffer, uint32_t bufferSize, TSPackage *tsPackage);
		void onMediaInfoDump( void * data );
	};
};
#endif
