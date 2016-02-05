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
			if ( mp4Boxes != 0 )
			{
				mp4Boxes->ftypBox = allocateFtypBox();
				mp4Boxes->moovBox = allocateMoovBox();
				mp4Boxes->mdtaBox = allocateMdtaBox();
				if ( mp4Boxes->ftypBox == 0 || mp4Boxes->moovBox == 0 || mp4Boxes->mdtaBox == 0 )
				{
					deallocateMP4Boxes(mp4Boxes);
					mp4Boxes = 0;
				}
			}
			return mp4Boxes;
		}

		FtypBox * allocateFtypBox()
		{
			FtypBox * ftypBox = new FtypBox;
			return ftypBox;
		}

		MoovBox * allocateMoovBox()
		{
			MoovBox * moovBox = new MoovBox;
			if ( moovBox != 0 )
			{
				moovBox->mvhdBox = 0;
				moovBox->iodsBox = 0;
				moovBox->traks = 0;
				moovBox->udtaBox = 0;
			}
			return moovBox;
		}

		
		MdtaBox * allocateMdtaBox()
		{
			MdtaBox * mdtaBox = new MdtaBox;
			if ( mdtaBox != 0 )
			{
				mdtaBox->data = 0;
			}
			return mdtaBox;
		}

		
		void deallocateMP4Boxes( void * data )
		{
			MP4Boxes *mp4Boxes = static_cast<MP4Boxes *>(data);
			if ( mp4Boxes != 0 )
			{
				deallocateFtypBox(mp4Boxes->ftypBox);
				deallocateMoovBox(mp4Boxes->moovBox);
				deallocateMdtaBox(mp4Boxes->mdtaBox);
				delete mp4Boxes;
			}
		}

		void deallocateFtypBox(void *data)
		{
			(void)data;
			;//do nothing temporarily
		}
		
		void deallocateMoovBox(void *data)
		{
			(void)data;
			;//do nothing temporarily
		}
		void deallocateMdtaBox(void *data)
		{
			(void)data;
			;//do nothing temporarily
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
						else
						{
							deallocateMP4Boxes(mp4Boxes);
						}

						if ( ok )
						{
							fread(&size, sizeof(size), 1, fmp4Handler);
							fseek(fmp4Handler, -4, SEEK_CUR);
							size = ntohl(size);
							buffer = new uint8_t[size];
							if ( buffer != 0 )
							{
								fread(buffer, size, 1, fmp4Handler);
								ok = onMdtaBoxParse(buffer, size, mp4Boxes->mdtaBox);
								delete [] buffer;
								buffer = 0;
							}
						}
						else
						{
							deallocateMP4Boxes(mp4Boxes);
						}
					}
				}
			}
			return mp4Boxes;
		}

		bool onMdtaBoxParse(uint8_t *buffer, uint32_t bufferSize, MdtaBox * mdtaBox)
		{
			bool status = false;
			if ( buffer == 0  || bufferSize == 0 || mdtaBox == 0 )
			return status;
			memcpy(&mdtaBox->size, buffer, 4);
			mdtaBox->size = ntohl(mdtaBox->size);
			if ( mdtaBox->size == bufferSize )
			{
				memcpy(mdtaBox->type, buffer+4, 4);
				if ( mdtaBox->type[0] == 'm' && mdtaBox->type[1] == 'd' && mdtaBox->type[2] == 't' && mdtaBox->type[3] == 'a' )
				{
					mdtaBox->data = new uint8_t[bufferSize-8];
					if ( mdtaBox->data != 0 )
					{
						status = true;
						memcpy(mdtaBox->data, buffer+8, bufferSize-8);
					}
				}
			}

			return status;
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
			if ( (P+4) < B )\
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
				return status;
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
			
			TrakBox *prevTrakBox = 0; 
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
					TrakBox * trakBox = new TrakBox;
					if ( trakBox != 0 )
					{
						trakBox->size = size;
						memcpy(trakBox->type, type, 4);
						trakBox->next = 0;
						trakBox->tkhdBox = 0;
						trakBox->mdiaBox = 0;
						
						if ( prevTrakBox != 0 )
						{
							prevTrakBox->next = trakBox;
						}
						else
						{
							moovBox->traks = trakBox;
						}
						prevTrakBox = trakBox;
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
					prevTrakBox->tkhdBox = new TkhdBox;
					if ( prevTrakBox->tkhdBox != 0 )
					{
						prevTrakBox->tkhdBox->boxHeader.fullBox = 1;
						prevTrakBox->tkhdBox->boxHeader.size = size;
						memcpy(prevTrakBox->tkhdBox->boxHeader.type, type, 4);
						memcpy(&prevTrakBox->tkhdBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrakBox->tkhdBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						memcpy(&prevTrakBox->tkhdBox->creationTime, buffer+pos, 4);
						prevTrakBox->tkhdBox->creationTime = ntohl(prevTrakBox->tkhdBox->creationTime);
						pos += 4;
						memcpy(&prevTrakBox->tkhdBox->modificationTime, buffer+pos, 4);
						prevTrakBox->tkhdBox->modificationTime = ntohl(prevTrakBox->tkhdBox->modificationTime);
						pos += 4;
						memcpy(&prevTrakBox->tkhdBox->trakID, buffer+pos, 4);
						prevTrakBox->tkhdBox->trakID = ntohl(prevTrakBox->tkhdBox->trakID);
						pos += 4;
						memcpy(prevTrakBox->tkhdBox->reserved1, buffer+pos, 4);
						pos += 4;
						memcpy(&prevTrakBox->tkhdBox->duration, buffer+pos, 4);
						pos += 4;
						prevTrakBox->tkhdBox->duration = ntohl(prevTrakBox->tkhdBox->duration);
						memcpy(prevTrakBox->tkhdBox->reserved2, buffer+pos, 8);
						pos += 8;
						memcpy(prevTrakBox->tkhdBox->layer, buffer+pos, 2);
						pos += 2;
						memcpy(prevTrakBox->tkhdBox->alternateGroup, buffer+pos, 2);
						pos += 2;
						memcpy(prevTrakBox->tkhdBox->volume, buffer+pos, 2);
						pos += 2;
						memcpy(prevTrakBox->tkhdBox->reserved3, buffer+pos, 2);
						pos += 2;
						memcpy(prevTrakBox->tkhdBox->matrix, buffer+pos, 36);
						pos += 36;
						memcpy(prevTrakBox->tkhdBox->width, buffer+pos, 4);
						pos += 4;
						memcpy(prevTrakBox->tkhdBox->height, buffer+pos, 4);
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
					prevTrakBox->mdiaBox = new MdiaBox;
					if ( prevTrakBox->mdiaBox != 0 )
					{
						prevTrakBox->mdiaBox->size = size;
						memcpy(prevTrakBox->mdiaBox->type, type, 4);
						prevTrakBox->mdiaBox->mdhdBox = 0;
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
					prevTrakBox->mdiaBox->mdhdBox = new MdhdBox;
					if ( prevTrakBox->mdiaBox->mdhdBox  != 0 )
					{
						prevTrakBox->mdiaBox->mdhdBox->boxHeader.size = size;
						memcpy(prevTrakBox->mdiaBox->mdhdBox->boxHeader.type, type, 4);
						memcpy(&prevTrakBox->mdiaBox->mdhdBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrakBox->mdiaBox->mdhdBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						memcpy(&prevTrakBox->mdiaBox->mdhdBox->creationTime, buffer+pos, 4);
						prevTrakBox->mdiaBox->mdhdBox->creationTime = ntohl(prevTrakBox->mdiaBox->mdhdBox->creationTime);
						pos += 4;
						memcpy(&prevTrakBox->mdiaBox->mdhdBox->modificationTime, buffer+pos, 4);
						prevTrakBox->mdiaBox->mdhdBox->modificationTime = ntohl(prevTrakBox->mdiaBox->mdhdBox->modificationTime);
						pos += 4;
						memcpy(&prevTrakBox->mdiaBox->mdhdBox->timescale, buffer+pos, 4);
						prevTrakBox->mdiaBox->mdhdBox->timescale = ntohl(prevTrakBox->mdiaBox->mdhdBox->timescale);
						pos += 4;
						memcpy(&prevTrakBox->mdiaBox->mdhdBox->duration, buffer+pos, 4);
						prevTrakBox->mdiaBox->mdhdBox->duration = ntohl(prevTrakBox->mdiaBox->mdhdBox->duration);
						pos += 4;
						memcpy(prevTrakBox->mdiaBox->mdhdBox->language, buffer+pos, 2);
						pos += 2;
						memcpy(prevTrakBox->mdiaBox->mdhdBox->predefined, buffer+pos, 2);
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
					prevTrakBox->mdiaBox->hdlrBox = new HdlrBox;
					if ( prevTrakBox->mdiaBox->hdlrBox  != 0 )
					{
						prevTrakBox->mdiaBox->hdlrBox->boxHeader.size = size;
						memcpy(prevTrakBox->mdiaBox->hdlrBox->boxHeader.type, type, 4);
						memcpy(&prevTrakBox->mdiaBox->hdlrBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrakBox->mdiaBox->hdlrBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						memcpy(prevTrakBox->mdiaBox->hdlrBox->componentType, buffer+pos, 4);
						pos += 4;
						memcpy(prevTrakBox->mdiaBox->hdlrBox->componentSubType, buffer+pos, 4);
						pos += 4;
						memcpy(prevTrakBox->mdiaBox->hdlrBox->reserved, buffer+pos, 12);
						pos += 12;
						prevTrakBox->mdiaBox->hdlrBox->name.assign((char*)(buffer+pos), size-32);
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
					prevTrakBox->mdiaBox->minfBox = new MinfBox;
					if ( prevTrakBox->mdiaBox->minfBox  != 0 )
					{
						prevTrakBox->mdiaBox->minfBox->size = size;
						prevTrakBox->mdiaBox->minfBox->vmhdBox = 0;
						prevTrakBox->mdiaBox->minfBox->smhdBox = 0;
						prevTrakBox->mdiaBox->minfBox->hmhdBox = 0;
						prevTrakBox->mdiaBox->minfBox->nmhdBox = 0;
						prevTrakBox->mdiaBox->minfBox->dinfBox = 0;
						prevTrakBox->mdiaBox->minfBox->stblBox = 0;
						memcpy(prevTrakBox->mdiaBox->minfBox->type, type, 4);
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}

				if ( type[0] == 's' && type[1] == 'm' && type[2] == 'h' && type[3] == 'd' )
				{
					prevTrakBox->mdiaBox->minfBox->smhdBox = new SmhdBox;
					if ( prevTrakBox->mdiaBox->minfBox->smhdBox != 0 )
					{
						prevTrakBox->mdiaBox->minfBox->smhdBox->boxHeader.size = size;
						memcpy(prevTrakBox->mdiaBox->minfBox->smhdBox->boxHeader.type, type, 4);
						memcpy(&prevTrakBox->mdiaBox->minfBox->smhdBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrakBox->mdiaBox->minfBox->smhdBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						memcpy(prevTrakBox->mdiaBox->minfBox->smhdBox->balance, type, 2);
						pos += 2;
						memcpy(prevTrakBox->mdiaBox->minfBox->smhdBox->reserved, type, 2);
						pos += 2;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}

				if ( type[0] == 'v' && type[1] == 'm' && type[2] == 'h' && type[3] == 'd' )
				{
					prevTrakBox->mdiaBox->minfBox->vmhdBox = new VmhdBox;
					if ( prevTrakBox->mdiaBox->minfBox->vmhdBox != 0 )
					{
						prevTrakBox->mdiaBox->minfBox->vmhdBox->boxHeader.size = size;
						memcpy(prevTrakBox->mdiaBox->minfBox->vmhdBox->boxHeader.type, type, 4);
						memcpy(&prevTrakBox->mdiaBox->minfBox->vmhdBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrakBox->mdiaBox->minfBox->vmhdBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						memcpy(prevTrakBox->mdiaBox->minfBox->vmhdBox->graphics, type, 2);
						pos += 2;
						memcpy(prevTrakBox->mdiaBox->minfBox->vmhdBox->opcolor, type, 6);
						pos += 6;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}
				
				if ( type[0] == 'd' && type[1] == 'i' && type[2] == 'n' && type[3] == 'f' )
				{
					prevTrakBox->mdiaBox->minfBox->dinfBox = new DinfBox;
					if ( prevTrakBox->mdiaBox->minfBox->dinfBox != 0 )
					{
						prevTrakBox->mdiaBox->minfBox->dinfBox->size = size;
						memcpy(prevTrakBox->mdiaBox->minfBox->dinfBox->type, type, 4);
						prevTrakBox->mdiaBox->minfBox->dinfBox->drefBox = 0;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}

				if ( type[0] == 'd' && type[1] == 'r' && type[2] == 'e' && type[3] == 'f' )
				{
					prevTrakBox->mdiaBox->minfBox->dinfBox->drefBox  = new DrefBox;
					if ( prevTrakBox->mdiaBox->minfBox->dinfBox->drefBox != 0 )
					{
						prevTrakBox->mdiaBox->minfBox->dinfBox->drefBox->boxHeader.size = size;
						memcpy(prevTrakBox->mdiaBox->minfBox->dinfBox->drefBox->boxHeader.type, type, 4);
						memcpy(&prevTrakBox->mdiaBox->minfBox->dinfBox->drefBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrakBox->mdiaBox->minfBox->dinfBox->drefBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						memcpy(&prevTrakBox->mdiaBox->minfBox->dinfBox->drefBox->entryCount, buffer+pos, 4);
						prevTrakBox->mdiaBox->minfBox->dinfBox->drefBox->entryCount = 
						ntohl(prevTrakBox->mdiaBox->minfBox->dinfBox->drefBox->entryCount);
						pos += 4;
						prevTrakBox->mdiaBox->minfBox->dinfBox->drefBox->urls = new uint8_t[size-16];
						memcpy(prevTrakBox->mdiaBox->minfBox->dinfBox->drefBox->urls, buffer+pos, size-16);
						pos += size-16;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}

				if ( type[0] == 's' && type[1] == 't' && type[2] == 'b' && type[3] == 'l' )
				{
					prevTrakBox->mdiaBox->minfBox->stblBox = new StblBox;
					if ( prevTrakBox->mdiaBox->minfBox->stblBox != 0 )
					{
						prevTrakBox->mdiaBox->minfBox->stblBox->stsdBox = 0;
						prevTrakBox->mdiaBox->minfBox->stblBox->stscBox = 0;
						prevTrakBox->mdiaBox->minfBox->stblBox->stcoBox = 0;
						prevTrakBox->mdiaBox->minfBox->stblBox->sttsBox = 0;
						prevTrakBox->mdiaBox->minfBox->stblBox->stszBox = 0;
						prevTrakBox->mdiaBox->minfBox->stblBox->stssBox = 0;
						prevTrakBox->mdiaBox->minfBox->stblBox->size = size;
						memcpy(prevTrakBox->mdiaBox->minfBox->stblBox->type,type,4);
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}

				if ( type[0] == 's' && type[1] == 't' && type[2] == 's' && type[3] == 'd' )
				{
					prevTrakBox->mdiaBox->minfBox->stblBox->stsdBox = new StsdBox;
					if ( prevTrakBox->mdiaBox->minfBox->stblBox->stsdBox != 0 )
					{
						prevTrakBox->mdiaBox->minfBox->stblBox->stsdBox->boxHeader.size = size;
						memcpy(prevTrakBox->mdiaBox->minfBox->stblBox->stsdBox->boxHeader.type,type,4);
						memcpy(&prevTrakBox->mdiaBox->minfBox->stblBox->stsdBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrakBox->mdiaBox->minfBox->stblBox->stsdBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						pos += size-12;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}
				
				if ( type[0] == 's' && type[1] == 't' && type[2] == 't' && type[3] == 's' )
				{
					prevTrakBox->mdiaBox->minfBox->stblBox->sttsBox = new SttsBox;
					if ( prevTrakBox->mdiaBox->minfBox->stblBox->sttsBox != 0 )
					{
						prevTrakBox->mdiaBox->minfBox->stblBox->sttsBox->boxHeader.size = size;
						memcpy(prevTrakBox->mdiaBox->minfBox->stblBox->sttsBox->boxHeader.type,type,4);
						memcpy(&prevTrakBox->mdiaBox->minfBox->stblBox->sttsBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrakBox->mdiaBox->minfBox->stblBox->sttsBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						pos += size-12;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}
				
				if ( type[0] == 's' && type[1] == 't' && type[2] == 's' && type[3] == 'c' )
				{
					prevTrakBox->mdiaBox->minfBox->stblBox->stscBox = new StscBox;
					if ( prevTrakBox->mdiaBox->minfBox->stblBox->stscBox != 0 )
					{
						prevTrakBox->mdiaBox->minfBox->stblBox->stscBox->boxHeader.size = size;
						memcpy(prevTrakBox->mdiaBox->minfBox->stblBox->stscBox->boxHeader.type,type,4);
						memcpy(&prevTrakBox->mdiaBox->minfBox->stblBox->stscBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrakBox->mdiaBox->minfBox->stblBox->stscBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						pos += size-12;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}
				
				if ( type[0] == 's' && type[1] == 't' && type[2] == 's' && type[3] == 'z' )
				{
					prevTrakBox->mdiaBox->minfBox->stblBox->stszBox = new StszBox;
					if ( prevTrakBox->mdiaBox->minfBox->stblBox->stszBox != 0 )
					{
						prevTrakBox->mdiaBox->minfBox->stblBox->stszBox->boxHeader.size = size;
						memcpy(prevTrakBox->mdiaBox->minfBox->stblBox->stszBox->boxHeader.type,type,4);
						memcpy(&prevTrakBox->mdiaBox->minfBox->stblBox->stszBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrakBox->mdiaBox->minfBox->stblBox->stszBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						pos += size-12;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}

				if ( type[0] == 's' && type[1] == 't' && type[2] == 's' && type[3] == 's' )
				{
					prevTrakBox->mdiaBox->minfBox->stblBox->stssBox = new StssBox;
					if ( prevTrakBox->mdiaBox->minfBox->stblBox->stssBox != 0 )
					{
						prevTrakBox->mdiaBox->minfBox->stblBox->stssBox->boxHeader.size = size;
						memcpy(prevTrakBox->mdiaBox->minfBox->stblBox->stssBox->boxHeader.type,type,4);
						memcpy(&prevTrakBox->mdiaBox->minfBox->stblBox->stssBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrakBox->mdiaBox->minfBox->stblBox->stssBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						pos += size-12;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}
					
				if ( type[0] == 's' && type[1] == 't' && type[2] == 'c' && type[3] == 'o' )
				{
					prevTrakBox->mdiaBox->minfBox->stblBox->stcoBox = new StcoBox;
					if ( prevTrakBox->mdiaBox->minfBox->stblBox->stcoBox != 0 )
					{
						prevTrakBox->mdiaBox->minfBox->stblBox->stcoBox->boxHeader.size = size;
						memcpy(prevTrakBox->mdiaBox->minfBox->stblBox->stcoBox->boxHeader.type,type,4);
						memcpy(&prevTrakBox->mdiaBox->minfBox->stblBox->stcoBox->boxHeader.version, buffer+pos, 1);
						pos += 1;
						memcpy(prevTrakBox->mdiaBox->minfBox->stblBox->stcoBox->boxHeader.flags, buffer+pos, 3);
						pos += 3;
						pos += size-12;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
					}
					else
					{
						status = false;
						break;
					}
				}

				if ( type[0] == 'u' && type[1] == 'd' && type[2] == 't' && type[3] == 'a' )
				{
					moovBox->udtaBox = new UdtaBox;
					if ( moovBox->udtaBox != 0 )
					{
						moovBox->udtaBox->size = size;
						memcpy(moovBox->udtaBox->type, type, 4);
						moovBox->udtaBox->data = new uint8_t[size-8];
						memcpy(moovBox->udtaBox->data, buffer+pos, size-8);
						pos += size-8;
						GET_SIZE_TYPE(pos, size, type, buffer, bufferSize);
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
					cout<<"+++++++++++++++++++++++++++++FTYP BOX+++++++++++++++++++++++++++++++"<<endl;
					cout<<"size:"<<mp4Boxes->ftypBox->boxHeader.size<<endl;
					cout<<"type:"<<std::string((char *)mp4Boxes->ftypBox->boxHeader.type)<<endl;
					cout<<"major brand:"<<std::string((char *)mp4Boxes->ftypBox->majorBrand)<<endl;
					cout<<"minor version:"<<(int)mp4Boxes->ftypBox->minorVersion[3]<<endl;
					cout<<"compatible brands:"<<std::string((char *)mp4Boxes->ftypBox->compatibleBrands+1, mp4Boxes->ftypBox->compatibleBrands[0])<<endl;
				}

				if ( mp4Boxes->moovBox != 0 )
				{
					cout<<"+++++++++++++++++++++++++MOOV BOX+++++++++++++++++++++++++++"<<endl;
					cout<<"size:"<<mp4Boxes->moovBox->size<<endl;
					cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->type, 4)<<endl;

					if ( mp4Boxes->moovBox->mvhdBox != 0 )
					{
						cout<<"++++++++++++++++++++MVHD BOX++++++++++++++++++++"<<endl;
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
					}

					if ( mp4Boxes->moovBox->iodsBox != 0 )
					{
						cout<<"++++++++++++++++++++IODS BOX++++++++++++++++++++"<<endl;
						cout<<"size:"<<mp4Boxes->moovBox->iodsBox->size<<endl;
						cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->iodsBox->type, 4)<<endl;
					}

					while ( mp4Boxes->moovBox->traks != 0 )
					{
						cout<<"++++++++++++++++++++TRAK BOX++++++++++++++++++++"<<endl;
						cout<<"size:"<<mp4Boxes->moovBox->traks->size<<endl;
						cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->traks->type, 4)<<endl;
						if ( mp4Boxes->moovBox->traks->tkhdBox != 0 )
						{
							cout<<"+++++++++++++++TKHD BOX+++++++++++++++"<<endl;
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
							cout<<"+++++++++++++++MDIA BOX+++++++++++++++"<<endl;
							cout<<"size:"<<mp4Boxes->moovBox->traks->mdiaBox->size<<endl;
							cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->traks->mdiaBox->type, 4)<<endl;

							if ( mp4Boxes->moovBox->traks->mdiaBox->mdhdBox != 0 )
							{
								cout<<"++++++++++MDHD BOX++++++++++"<<endl;
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
								cout<<"++++++++++HDLR BOX++++++++++"<<endl;
								cout<<"size:"<<mp4Boxes->moovBox->traks->mdiaBox->hdlrBox->boxHeader.size<<endl;
								cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->traks->mdiaBox->hdlrBox->boxHeader.type, 4)<<endl;
								cout<<"version:"<<(int)mp4Boxes->moovBox->traks->mdiaBox->hdlrBox->boxHeader.version<<endl;
								cout<<"component type:"<<std::string((char *)mp4Boxes->moovBox->traks->mdiaBox->hdlrBox->componentType, 4)<<endl;
								cout<<"component Subtype:"<<std::string((char *)mp4Boxes->moovBox->traks->mdiaBox->hdlrBox->componentSubType, 4)<<endl;
								cout<<"name:"<<mp4Boxes->moovBox->traks->mdiaBox->hdlrBox->name<<endl;
							}

							if ( mp4Boxes->moovBox->traks->mdiaBox->minfBox != 0 )
							{
								cout<<"++++++++++MINF BOX++++++++++"<<endl;
								cout<<"size:"<<mp4Boxes->moovBox->traks->mdiaBox->minfBox->size<<endl;
								cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->traks->mdiaBox->minfBox->type, 4)<<endl;
								if ( mp4Boxes->moovBox->traks->mdiaBox->minfBox->smhdBox != 0 )
								{
									cout<<"+++++SMHD BOX+++++"<<endl;
									cout<<"size:"<<mp4Boxes->moovBox->traks
												->mdiaBox->minfBox->smhdBox->boxHeader.size<<endl;
									cout<<"type:"<<std::string((char *)mp4Boxes->moovBox
										->traks->mdiaBox->minfBox->smhdBox->boxHeader.type, 4)<<endl;
								}
								
								if ( mp4Boxes->moovBox->traks->mdiaBox->minfBox->vmhdBox != 0 )
								{
									cout<<"+++++VMHD BOX+++++"<<endl;
									cout<<"size:"<<mp4Boxes->moovBox->traks
												->mdiaBox->minfBox->vmhdBox->boxHeader.size<<endl;
									cout<<"type:"<<std::string((char *)mp4Boxes->moovBox
										->traks->mdiaBox->minfBox->vmhdBox->boxHeader.type, 4)<<endl;
								}
								
								if ( mp4Boxes->moovBox->traks->mdiaBox->minfBox->dinfBox != 0 )
								{
									cout<<"+++++DINF BOX+++++"<<endl;
									cout<<"size:"<<mp4Boxes->moovBox->traks
												->mdiaBox->minfBox->dinfBox->size<<endl;
									cout<<"type:"<<std::string((char *)mp4Boxes->moovBox
										->traks->mdiaBox->minfBox->dinfBox->type, 4)<<endl;

									if ( mp4Boxes->moovBox->traks->mdiaBox->minfBox->dinfBox->drefBox != 0 )
									{
										cout<<"+DREF BOX+"<<endl;
										cout<<"size:"<<mp4Boxes->moovBox->traks
												->mdiaBox->minfBox->dinfBox->drefBox->boxHeader.size<<endl;
										cout<<"type:"<<std::string((char *)mp4Boxes->moovBox
										->traks->mdiaBox->minfBox->dinfBox->drefBox->boxHeader.type, 4)<<endl;
										cout<<"entry count:"<<mp4Boxes->moovBox->traks
												->mdiaBox->minfBox->dinfBox->drefBox->entryCount<<endl;
									}
								}

								if ( mp4Boxes->moovBox->traks->mdiaBox->minfBox->stblBox != 0 )
								{
									cout<<"+++++STBL BOX+++++"<<endl;
									cout<<"size:"<<mp4Boxes->moovBox->traks
												->mdiaBox->minfBox->stblBox->size<<endl;
									cout<<"type:"<<std::string((char *)mp4Boxes->moovBox
										->traks->mdiaBox->minfBox->stblBox->type, 4)<<endl;
								}
							}
						}
						mp4Boxes->moovBox->traks = mp4Boxes->moovBox->traks->next;
					}

					
					if ( mp4Boxes->moovBox->udtaBox != 0 )
					{
						cout<<"++++++++++++++++++++UDTA BOX++++++++++++++++++++"<<endl;
						cout<<"size:"<<mp4Boxes->moovBox->udtaBox->size<<endl;
						cout<<"type:"<<std::string((char *)mp4Boxes->moovBox->udtaBox->type, 4)<<endl;
					}
				}

				if ( mp4Boxes->mdtaBox != 0 )
				{
					cout<<"+++++++++++++++++++++++++++++MDTA BOX+++++++++++++++++++++++++++++++"<<endl;
					cout<<"size:"<<mp4Boxes->mdtaBox->size<<endl;
					cout<<"type:"<<std::string((char *)mp4Boxes->mdtaBox->type, 4)<<endl;
				}
			}
		}
	};
};
