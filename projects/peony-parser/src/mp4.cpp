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
#include <cstring>
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

				mp4Boxes->moovBox = new MoovBox;
				if ( mp4Boxes->moovBox != 0 )
				{
					mp4Boxes->moovBox->mvhdBox = 0;
					mp4Boxes->moovBox->iodsBox = 0;
					mp4Boxes->moovBox->tracks = 0;
				}
				else
				{
					delete mp4Boxes->ftypBox;
					mp4Boxes->ftypBox = 0;
					delete mp4Boxes;
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
					mp4Boxes->ftypBox = 0;
				}

				if ( mp4Boxes->moovBox != 0 )
				{
					delete mp4Boxes->moovBox;
					mp4Boxes->moovBox = 0;
				}
				delete mp4Boxes;
			}
		}


		MP4Boxes * onMediaMP4Parse(const char * fileName)
		{
			MP4Boxes * mp4Boxes = 0;
			FILE * fmp4Handler = fopen( fileName, "r");
			uint32_t size = 0;
			uint8_t * buffer = 0;
			if ( fmp4Handler != 0 )
			{
				mp4Boxes = allocateMP4Boxes();
				if ( mp4Boxes != 0 )
				{	
					fread(&size, sizeof(size), 1, fmp4Handler);
					fseek(fmp4Handler, 0, SEEK_SET);
					
					//ignore the condition of large size
					size = ntohl(size);
					buffer = new uint8_t[size];
					if ( buffer != 0)
					{
						fread(buffer, size, 1, fmp4Handler);
						bool ok = onFtypBoxParse(buffer, size, mp4Boxes->ftypBox);
						delete [] buffer;
						buffer = 0;

						//then parse the moov box
						if ( ok )
						{
							fread(&size, sizeof(size), 1, fmp4Handler);
							fseek(fmp4Handler, -4, SEEK_CUR);
							size = ntohl(size);
							buffer = new uint8_t[size];
							if ( buffer != 0 )
							{
								fread(buffer, size, 1, fmp4Handler);
								ok = onMoovBoxParse(buffer, size, mp4Boxes->moovBox);
								delete [] buffer;
								buffer = 0;
							}
							
						}
					}
				}
			}
			return mp4Boxes;
		}


		bool onFtypBoxParse(uint8_t *buffer, uint32_t size, FtypBox * ftypBox)
		{
			bool status = false;
			if ( buffer == 0 || size == 0 || ftypBox == 0 )
			{
				status = false;
			}
			else
			{
				if ( size < 16 )
				{
					status = false;
				}
				else
				{
					ftypBox->boxHeader.size = size;
					memcpy(ftypBox->boxHeader.type, buffer+4, 4);
					memcpy(ftypBox->majorBrand, buffer+8, 4);
					memcpy(ftypBox->minorVersion, buffer+12, 4);
					ftypBox->compatibleBrands[0] = static_cast<uint8_t>(size-16);
					memcpy(ftypBox->compatibleBrands+1, buffer+16, size-16);
					status = true;
				}
			}
			return status;
		}

		bool onMoovBoxParse(uint8_t *buffer,uint32_t bufferSize, MoovBox * moovBox)
		{
			bool status = true;
			if ( buffer == 0 || bufferSize < 16 || moovBox == 0 )
			{
				status = false;
			}

			uint32_t pos = 4;
			moovBox->size = bufferSize;
			memcpy(moovBox->type, buffer+pos, 4);
			pos += 4;
			uint32_t size=0;
			uint8_t type[4]={0};

			memcpy(&size, buffer+pos, 4);
			size = ntohl(size);
			pos +=4;
			memcpy(type, buffer+pos, 4);
			pos +=4;
			
			while ( (pos-8+size) <= bufferSize  )
			{
				if ( type[0] == 'm' && type[1] == 'v' && type[2] == 'h' && type[3] == 'd' )
				{
					moovBox->mvhdBox = new MvhdBox;
					if ( moovBox->mvhdBox != 0 )
					{
						moovBox->mvhdBox->boxHeader.size = size;
						memcpy(moovBox->mvhdBox->boxHeader.type, type, 4);
						memcpy(&moovBox->mvhdBox->boxHeader.version, buffer+pos, 1);
						pos +=1;
						memcpy(moovBox->mvhdBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						memcpy(&moovBox->mvhdBox->creationTime, buffer+pos, 4);
						moovBox->mvhdBox->creationTime = ntohl(moovBox->mvhdBox->creationTime );
						pos +=4;
						memcpy(&moovBox->mvhdBox->modificationTime, buffer+pos, 4);
						moovBox->mvhdBox->modificationTime = ntohl(moovBox->mvhdBox->modificationTime );
						pos += 4;
						memcpy(&moovBox->mvhdBox->timescale, buffer+pos, 4);
						moovBox->mvhdBox->timescale = ntohl(moovBox->mvhdBox->timescale);
						pos += 4;
						memcpy(&moovBox->mvhdBox->duration, buffer+pos, 4);
						moovBox->mvhdBox->duration = ntohl(moovBox->mvhdBox->duration );
						pos += 4;
						memcpy(moovBox->mvhdBox->rate, buffer+pos, 4);
						pos += 4;
						memcpy(moovBox->mvhdBox->volume, buffer+pos, 2);
						pos +=2;
						memcpy(moovBox->mvhdBox->reserved, buffer+pos, 10);
						pos +=10;
						memcpy(moovBox->mvhdBox->matrix, buffer+pos, 36);
						pos +=36;
						memcpy(moovBox->mvhdBox->predefined, buffer+pos, 24);
						pos +=24;
						memcpy(&moovBox->mvhdBox->nextTrackID, buffer+pos, 4);
						pos += 4;
						moovBox->mvhdBox->nextTrackID = ntohl(moovBox->mvhdBox->nextTrackID);

						if ( (pos + size) <= bufferSize )
						{
							memcpy(&size, buffer+pos, 4);
							size = ntohl(size);
							pos +=4;
							memcpy(type, buffer+pos, 4);
							pos +=4;
						}
						else
						break;	
					}
					else
					{
						status = false;
						break;
					}
				}	

				if ( type[0] == 'i' && type[1] == 'o' && type[2] == 'd' && type[3] == 's' )
				{
					moovBox->iodsBox = new IodsBox;
					if ( moovBox->iodsBox != 0 )
					{
						moovBox->iodsBox->boxHeader.size = size;
						memcpy(moovBox->iodsBox->boxHeader.type, type, 4);
						break;
					}
					else
					{
						status = false;
						break;
					}
				}
			}
			return status;
		}

		
		void onMP4InfoDump(void * data)
		{
			MP4Boxes * mp4Boxes = static_cast<MP4Boxes *>(data);
			if ( mp4Boxes != 0 )
			{
				cout<<endl;
				if ( mp4Boxes->ftypBox != 0 )
				{
					cout<<"++++++++++++++++++++++++FTYP BOX++++++++++++++++++++++++++"<<endl;
					cout<<"size:"<<mp4Boxes->ftypBox->boxHeader.size<<endl;
					cout<<"type:"<<std::string((char *)mp4Boxes->ftypBox->boxHeader.type)<<endl;
					cout<<"major brand:"<<std::string((char *)mp4Boxes->ftypBox->majorBrand)<<endl;
					cout<<"minor version:"<<(int)mp4Boxes->ftypBox->minorVersion[3]<<endl;
					cout<<"compatible brands:"<<std::string((char *)mp4Boxes->ftypBox->compatibleBrands+1, mp4Boxes->ftypBox->compatibleBrands[0])<<endl;
					cout<<endl<<endl;
				}

				if ( mp4Boxes->moovBox != 0 )
				{
					cout<<"++++++++++++++++++++++++MOOV BOX++++++++++++++++++++++++++"<<endl;
					cout<<"size:"<<mp4Boxes->moovBox->size<<endl;
					cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->type, 4)<<endl;

					if ( mp4Boxes->moovBox->mvhdBox != 0 )
					{
						cout<<"++++++++++++MVHD BOX++++++++++++"<<endl;
						cout<<"size:"<<mp4Boxes->moovBox->mvhdBox->boxHeader.size<<endl;
						cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->mvhdBox->boxHeader.type)<<endl;
						cout<<"version:"<<(int)mp4Boxes->moovBox->mvhdBox->boxHeader.version<<endl;
						cout<<"creation time:"<<mp4Boxes->moovBox->mvhdBox->creationTime<<endl;
						cout<<"modification time:"<<mp4Boxes->moovBox->mvhdBox->modificationTime<<endl;
						cout<<"timescale:"<<mp4Boxes->moovBox->mvhdBox->timescale<<endl;
						cout<<"duration:"<<mp4Boxes->moovBox->mvhdBox->duration<<endl;
						double rate = static_cast<double>(mp4Boxes->moovBox->mvhdBox->rate[0]*256+mp4Boxes->moovBox->mvhdBox->rate[1]);
						cout<<"rate:"<<rate<<endl;
						double volume = static_cast<double>(mp4Boxes->moovBox->mvhdBox->volume[0]);
						cout<<"volume:"<<volume<<endl;
						cout<<"next track ID:"<<(int)mp4Boxes->moovBox->mvhdBox->nextTrackID<<endl;
						cout<<endl<<endl;
					}

					if ( mp4Boxes->moovBox->iodsBox != 0 )
					{
						cout<<"++++++++++++IODS BOX++++++++++++"<<endl;
						cout<<"size:"<<mp4Boxes->moovBox->iodsBox->boxHeader.size<<endl;
						cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->iodsBox->boxHeader.type)<<endl;
						cout<<endl<<endl;
					}
				}
			}
		}
	};
};
