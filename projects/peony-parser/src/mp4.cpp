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
					mp4Boxes->moovBox->traks = 0;
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
		
		void deallocateMP4Boxes( void * data )
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
			#define GET_SIZE_TYPE(P,S,T,D,B) \
			if ( (P+S) <= B )\
			{\
				memcpy(&S, D+P, 4);\
				S = ntohl(S);\
				P +=4;\
				memcpy(T, D+P, 4);\
				P +=4;\
			}\
			else\
			{\
				break;\
			}\
			
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
			
			TrakBox *prevTrackBox = 0; 
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
						memcpy(&moovBox->mvhdBox->nextTrakID, buffer+pos, 4);
						pos += 4;
						moovBox->mvhdBox->nextTrakID = ntohl(moovBox->mvhdBox->nextTrakID);
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);	
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
						moovBox->iodsBox->size = size;
						memcpy(moovBox->iodsBox->type, type, 4);
						moovBox->iodsBox->data = 0;
						if ( size-8 > 0 )
						{
							moovBox->iodsBox->data = new uint8_t[size-8];
							pos += size-8;
							if ( moovBox->iodsBox->data != 0 )
							{
								memcpy(moovBox->iodsBox->data, buffer+pos, size-8);
								GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
							}
							else
							{
								status = false;
								break;
							}
						}
					}
					else
					{
						status = false;
						break;
					}
				}

				if ( type[0] == 't' && type[1] == 'r' && type[2] == 'a' && type[3] == 'k' )
				{
					moovBox->traks = new TrakBox;
					if ( moovBox->traks != 0 )
					{
						moovBox->traks->size = size;
						memcpy(moovBox->traks->type, type, 4);
						moovBox->traks->next = 0;
						moovBox->traks->tkhdBox = 0;
						moovBox->traks->mdiaBox = 0;
						if ( prevTrackBox != 0 )
						{
							prevTrackBox->next = moovBox->traks;
						}
						prevTrackBox = moovBox->traks;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}

				if ( type[0] == 't' && type[1] == 'k' && type[2] == 'h' && type[3] == 'd' )
				{
					prevTrackBox->tkhdBox = new TkhdBox;
					if ( prevTrackBox->tkhdBox != 0 )
					{
						prevTrackBox->tkhdBox->boxHeader.fullBox = 1;
						prevTrackBox->tkhdBox->boxHeader.size = size;
						memcpy(prevTrackBox->tkhdBox->boxHeader.type, type, 4);
						memcpy(&prevTrackBox->tkhdBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrackBox->tkhdBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						memcpy(&prevTrackBox->tkhdBox->creationTime, buffer+pos, 4);
						prevTrackBox->tkhdBox->creationTime = ntohl(prevTrackBox->tkhdBox->creationTime);
						pos += 4;
						memcpy(&prevTrackBox->tkhdBox->modificationTime, buffer+pos, 4);
						prevTrackBox->tkhdBox->modificationTime = ntohl(prevTrackBox->tkhdBox->modificationTime);
						pos += 4;
						memcpy(&prevTrackBox->tkhdBox->trakID, buffer+pos, 4);
						prevTrackBox->tkhdBox->trakID = ntohl(prevTrackBox->tkhdBox->trakID);
						pos += 4;
						memcpy(prevTrackBox->tkhdBox->reserved1, buffer+pos, 4);
						pos += 4;
						memcpy(&prevTrackBox->tkhdBox->duration, buffer+pos, 4);
						pos += 4;
						prevTrackBox->tkhdBox->duration = ntohl(prevTrackBox->tkhdBox->duration);
						memcpy(prevTrackBox->tkhdBox->reserved2, buffer+pos, 8);
						pos += 8;
						memcpy(prevTrackBox->tkhdBox->layer, buffer+pos, 2);
						pos += 2;
						memcpy(prevTrackBox->tkhdBox->alternateGroup, buffer+pos, 2);
						pos += 2;
						memcpy(prevTrackBox->tkhdBox->volume, buffer+pos, 2);
						pos += 2;
						memcpy(prevTrackBox->tkhdBox->reserved3, buffer+pos, 2);
						pos += 2;
						memcpy(prevTrackBox->tkhdBox->matrix, buffer+pos, 36);
						pos += 36;
						memcpy(prevTrackBox->tkhdBox->width, buffer+pos, 4);
						pos += 4;
						memcpy(prevTrackBox->tkhdBox->height, buffer+pos, 4);
						pos += 4;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
					
				}

				if ( type[0] == 'm' && type[1] == 'd' && type[2] == 'i' && type[3] == 'a' )
				{
					prevTrackBox->mdiaBox = new MdiaBox;
					if ( prevTrackBox->mdiaBox != 0 )
					{
						prevTrackBox->mdiaBox->size = size;
						memcpy(prevTrackBox->mdiaBox->type, type, 4);
						prevTrackBox->mdiaBox->mdhdBox = 0;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
					
				}

				if ( type[0] == 'm' && type[1] == 'd' && type[2] == 'h' && type[3] == 'd' )
				{
					prevTrackBox->mdiaBox->mdhdBox = new MdhdBox;
					if ( prevTrackBox->mdiaBox->mdhdBox  != 0 )
					{
						prevTrackBox->mdiaBox->mdhdBox->boxHeader.size = size;
						memcpy(prevTrackBox->mdiaBox->mdhdBox->boxHeader.type, type, 4);
						memcpy(&prevTrackBox->mdiaBox->mdhdBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrackBox->mdiaBox->mdhdBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						memcpy(&prevTrackBox->mdiaBox->mdhdBox->creationTime, buffer+pos, 4);
						prevTrackBox->mdiaBox->mdhdBox->creationTime = ntohl(prevTrackBox->mdiaBox->mdhdBox->creationTime);
						pos += 4;
						memcpy(&prevTrackBox->mdiaBox->mdhdBox->modificationTime, buffer+pos, 4);
						prevTrackBox->mdiaBox->mdhdBox->modificationTime = ntohl(prevTrackBox->mdiaBox->mdhdBox->modificationTime);
						pos += 4;
						memcpy(&prevTrackBox->mdiaBox->mdhdBox->timescale, buffer+pos, 4);
						prevTrackBox->mdiaBox->mdhdBox->timescale = ntohl(prevTrackBox->mdiaBox->mdhdBox->timescale);
						pos += 4;
						memcpy(&prevTrackBox->mdiaBox->mdhdBox->duration, buffer+pos, 4);
						prevTrackBox->mdiaBox->mdhdBox->duration = ntohl(prevTrackBox->mdiaBox->mdhdBox->duration);
						pos += 4;
						memcpy(prevTrackBox->mdiaBox->mdhdBox->language, buffer+pos, 2);
						pos += 2;
						memcpy(prevTrackBox->mdiaBox->mdhdBox->predefined, buffer+pos, 2);
						pos += 2;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}

				if ( type[0] == 'h' && type[1] == 'd' && type[2] == 'l' && type[3] == 'r' )
				{
					prevTrackBox->mdiaBox->hdlrBox = new HdlrBox;
					if ( prevTrackBox->mdiaBox->hdlrBox  != 0 )
					{
						prevTrackBox->mdiaBox->hdlrBox->boxHeader.size = size;
						memcpy(prevTrackBox->mdiaBox->hdlrBox->boxHeader.type, type, 4);
						memcpy(&prevTrackBox->mdiaBox->hdlrBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrackBox->mdiaBox->hdlrBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						memcpy(prevTrackBox->mdiaBox->hdlrBox->componentType, buffer+pos, 4);
						pos += 4;
						memcpy(prevTrackBox->mdiaBox->hdlrBox->componentSubType, buffer+pos, 4);
						pos += 4;
						memcpy(prevTrackBox->mdiaBox->hdlrBox->reserved, buffer+pos, 12);
						pos += 12;
						prevTrackBox->mdiaBox->hdlrBox->name.assign((char*)(buffer+pos), size-32);
						pos += size-32;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}

				if ( type[0] == 'm' && type[1] == 'i' && type[2] == 'n' && type[3] == 'f' )
				{
					prevTrackBox->mdiaBox->minfBox = new MinfBox;
					if ( prevTrackBox->mdiaBox->minfBox  != 0 )
					{
						prevTrackBox->mdiaBox->minfBox->size = size;
						memcpy(prevTrackBox->mdiaBox->minfBox->type, type, 4);
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
						cout<<"next track ID:"<<(int)mp4Boxes->moovBox->mvhdBox->nextTrakID<<endl;
						cout<<endl<<endl;
					}

					if ( mp4Boxes->moovBox->iodsBox != 0 )
					{
						cout<<"++++++++++++IODS BOX++++++++++++"<<endl;
						cout<<"size:"<<mp4Boxes->moovBox->iodsBox->size<<endl;
						cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->iodsBox->type, 4)<<endl;
						cout<<endl<<endl;
					}

					if ( mp4Boxes->moovBox->traks != 0 )
					{
						cout<<"++++++++++++TRAK BOX++++++++++++"<<endl;
						cout<<"size:"<<mp4Boxes->moovBox->traks->size<<endl;
						cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->traks->type, 4)<<endl;
						if ( mp4Boxes->moovBox->traks->tkhdBox != 0 )
						{
							cout<<"++++++++++++TKHD BOX++++++++++++"<<endl;
							cout<<"size:"<<mp4Boxes->moovBox->traks->tkhdBox->boxHeader.size<<endl;
							cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->traks->tkhdBox->boxHeader.type, 4)<<endl;
							cout<<"version:"<<(int)mp4Boxes->moovBox->traks->tkhdBox->boxHeader.version<<endl;
							cout<<"creation time:"<<mp4Boxes->moovBox->traks->tkhdBox->creationTime<<endl;
							cout<<"modification time:"<<mp4Boxes->moovBox->traks->tkhdBox->modificationTime<<endl;
							cout<<"trak ID:"<<mp4Boxes->moovBox->traks->tkhdBox->trakID<<endl;
							cout<<"duration:"<<mp4Boxes->moovBox->traks->tkhdBox->duration<<endl;
							cout<<"layer:"<<mp4Boxes->moovBox->traks->tkhdBox->layer[0]*256
										    +mp4Boxes->moovBox->traks->tkhdBox->layer[1]<<endl;
							cout<<"volume:"<<(double)(mp4Boxes->moovBox->traks->tkhdBox->volume[0])<<endl;
							cout<<"width:"<<mp4Boxes->moovBox->traks->tkhdBox->width[0]*256
										    +mp4Boxes->moovBox->traks->tkhdBox->width[1]<<endl;
							cout<<"height:"<<mp4Boxes->moovBox->traks->tkhdBox->height[0]*256
										    +mp4Boxes->moovBox->traks->tkhdBox->height[1]<<endl;
						}

						if (  mp4Boxes->moovBox->traks->mdiaBox != 0 )
						{
							cout<<"++++++++++++MDIA BOX++++++++++++"<<endl;
							cout<<"size:"<<mp4Boxes->moovBox->traks->mdiaBox->size<<endl;
							cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->traks->mdiaBox->type, 4)<<endl;

							if ( mp4Boxes->moovBox->traks->mdiaBox->mdhdBox != 0 )
							{
								cout<<"++++++++++++MDHD BOX++++++++++++"<<endl;
								cout<<"size:"<<mp4Boxes->moovBox->traks->mdiaBox->mdhdBox->boxHeader.size<<endl;
								cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->traks->mdiaBox->mdhdBox->boxHeader.type, 4)<<endl;
								cout<<"version:"<<(int)mp4Boxes->moovBox->traks->mdiaBox->mdhdBox->boxHeader.version<<endl;
								cout<<"creation time:"<<mp4Boxes->moovBox->traks->mdiaBox->mdhdBox->creationTime<<endl;
								cout<<"modification time:"<<mp4Boxes->moovBox->traks->mdiaBox->mdhdBox->modificationTime<<endl;
								cout<<"timescale:"<<mp4Boxes->moovBox->traks->mdiaBox->mdhdBox->timescale<<endl;
								cout<<"duration:"<<mp4Boxes->moovBox->traks->mdiaBox->mdhdBox->duration<<endl;
								cout<<"language:"<<std::string((char *)mp4Boxes->moovBox->traks->mdiaBox->mdhdBox->language,2)<<endl;
							}

							if ( mp4Boxes->moovBox->traks->mdiaBox->hdlrBox != 0 )
							{
								cout<<"++++++++++++HDLR BOX++++++++++++"<<endl;
								cout<<"size:"<<mp4Boxes->moovBox->traks->mdiaBox->hdlrBox->boxHeader.size<<endl;
								cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->traks->mdiaBox->hdlrBox->boxHeader.type, 4)<<endl;
								cout<<"version:"<<(int)mp4Boxes->moovBox->traks->mdiaBox->hdlrBox->boxHeader.version<<endl;
								cout<<"component type:"<<std::string((char *)mp4Boxes->moovBox->traks->mdiaBox->hdlrBox->componentType, 4)<<endl;
								cout<<"component Subtype:"<<std::string((char *)mp4Boxes->moovBox->traks->mdiaBox->hdlrBox->componentSubType, 4)<<endl;
								cout<<"name:"<<mp4Boxes->moovBox->traks->mdiaBox->hdlrBox->name<<endl;
							}

							if ( mp4Boxes->moovBox->traks->mdiaBox->minfBox != 0 )
							{
								cout<<"++++++++++++MINF BOX++++++++++++"<<endl;
								cout<<"size:"<<mp4Boxes->moovBox->traks->mdiaBox->minfBox->size<<endl;
								cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->traks->mdiaBox->minfBox->type, 4)<<endl;
							}
						}
					}
				}
			}
		}
	};
};
