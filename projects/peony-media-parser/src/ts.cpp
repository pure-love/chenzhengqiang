/*
*@filename:ts.cpp
*@author:chenzhengqiang
*@start date:2016/02/05 12:24:28
*@modified date:
*@desc: 
*/



#include "ts.h"
#include <cstring>
#include <cstdio>


namespace czq
{
	namespace mpeg
	{
		TSPackage * allocateTSPackage()
		{
			TSPackage *tsPackage = new TSPackage;
			memset(&tsPackage->tsPackageHeader, 0, sizeof(tsPackage->tsPackageHeader));
			return tsPackage;
		}

		void deallocatePackage( void * data)
		{
			TSPackage *tsPackage = static_cast<TSPackage *>(data);
			if ( tsPackage != 0 )
			{
				delete tsPackage;
			}
		}
		
		TSPackage * onMediaParse(const char * fileName)
		{
			TSPackage *tsPackage = allocateTSPackage();
			if ( tsPackage != 0 )
			{
				FILE * ftsHandler = fopen( fileName, "r");
				if ( ftsHandler != 0 )
				{
					uint8_t buffer[4];
					fread(buffer,sizeof(buffer),1,ftsHandler);
					bool ok = onTSPackageHeaderParse(buffer, sizeof(buffer), tsPackage);
					if ( ok )
					{
						;
					}
				}
			}
			return tsPackage;
		}


		bool onTSPackageHeaderParse(uint8_t *buffer, uint32_t bufferSize, TSPackage *tsPackage)
		{
				
			bool status = false;
			if ( buffer == 0 || bufferSize == 0 || tsPackage == 0 )
			{
				return status;
			}
			
			size_t pos = 0;
			tsPackage->tsPackageHeader.syncByte = buffer[pos];
			pos += 1;
			if ( pos+2 <= bufferSize )
			{
				tsPackage->tsPackageHeader.errorIndicator = (buffer[pos] & 0x80) >> 7;
				tsPackage->tsPackageHeader.payloadUnitStartIndicator = (buffer[pos] & 0x40) >> 6;
				tsPackage->tsPackageHeader.priority = (buffer[pos] & 0x20) >> 5;
				tsPackage->tsPackageHeader.PID = ((buffer[pos] & 0x1f) << 8) | buffer[pos+1];
				pos += 2;

				if ( pos +1 <= bufferSize )
				{
					tsPackage->tsPackageHeader.scramblingControl = (buffer[pos] & 0xc0) >> 6;
					tsPackage->tsPackageHeader.adaptationFieldControl = (buffer[pos] & 0x30) >> 4;
					tsPackage->tsPackageHeader.continuityCounter = buffer[pos] & 0x0f;
					status = true;
				}
			}

			return status;
			
		}
		
		void onMediaInfoDump( void * data )
		{
			TSPackage *tsPackage = static_cast<TSPackage *>(data);
			if ( tsPackage != 0 )
			{
				printf("+++++++++++++++++++++TS+++++++++++++++++++++\n");
				printf("sync byte:%x\n",tsPackage->tsPackageHeader.syncByte);
				printf("error indicator:%u\n",tsPackage->tsPackageHeader.errorIndicator);
				printf("payload unit start indicator:%u\n",tsPackage->tsPackageHeader.payloadUnitStartIndicator);
				printf("transport priority:%u\n",tsPackage->tsPackageHeader.priority);
				printf("PID:%u\n",tsPackage->tsPackageHeader.PID);
				printf("scrambling control:%u\n",tsPackage->tsPackageHeader.scramblingControl);
				printf("adaptation field control:%u\n",tsPackage->tsPackageHeader.adaptationFieldControl);
				printf("continuity counter:%u\n",tsPackage->tsPackageHeader.continuityCounter);
			}
		}
	};
};
