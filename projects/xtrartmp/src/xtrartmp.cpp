/*
*@filename:xtrartmp.cpp
*@author:chenzhengqiang
*@start date:2016/03/01 12:27:47
*@modified date:
*@desc: 
*/


#include "errors.h"
#include "xtrartmp.h"
#include <cstring>
namespace czq
{
	namespace XtraRtmp
	{
		ssize_t parseRtmpPacket(unsigned char *rtmpRequest, size_t len, RtmpPacket & rtmpPacket)
		{
			#define ToParseRtmpPacket __func__
			if ( len > 0 )
			{
				size_t pos = 0;
				while (pos < len)
				{
					if ( pos +1 >= len )
					break;	
					memset(&rtmpPacket.rtmpPacketHeader, 0, sizeof(rtmpPacket.rtmpPacketHeader));
					char format = (rtmpRequest[pos] & 0xc0) >> 6;
					rtmpPacket.rtmpPacketHeader.chunkStreamID = rtmpRequest[pos] & 0x3f;
					
					switch ( format )
					{
						case 0:
							rtmpPacket.rtmpPacketHeader.size = 12;
							rtmpPacket.rtmpPacketHeader.timestamp = rtmpRequest[pos+1]*65536+rtmpRequest[pos+2]*256+rtmpRequest[pos+3];
							rtmpPacket.rtmpPacketHeader.AMFSize = rtmpRequest[pos+4]*65536+rtmpRequest[pos+5]*256+rtmpRequest[pos+6];
							rtmpPacket.rtmpPacketHeader.AMFType = rtmpRequest[pos+7];
							rtmpPacket.rtmpPacketHeader.streamID = rtmpRequest[pos+8]*256*256*256+rtmpRequest[pos+9]*65536
															+rtmpRequest[pos+10]*256+rtmpRequest[pos+11];
							break;
						case 1:
							rtmpPacket.rtmpPacketHeader.size = 8;
							rtmpPacket.rtmpPacketHeader.timestamp = rtmpRequest[pos+1]*65536+rtmpRequest[pos+2]*256+rtmpRequest[pos+3];
							rtmpPacket.rtmpPacketHeader.AMFSize = rtmpRequest[pos+4]*65536+rtmpRequest[pos+5]*256+rtmpRequest[pos+6];
							rtmpPacket.rtmpPacketHeader.AMFType = rtmpRequest[pos+7];
							rtmpPacket.rtmpPacketHeader.streamID=0;
							break;
						case 2:
							rtmpPacket.rtmpPacketHeader.size = 4;
							rtmpPacket.rtmpPacketHeader.timestamp = rtmpRequest[pos+1]*65536+rtmpRequest[pos+2]*256+rtmpRequest[pos+3];
							rtmpPacket.rtmpPacketHeader.AMFSize=0;
							rtmpPacket.rtmpPacketHeader.AMFType=0;
							rtmpPacket.rtmpPacketHeader.streamID=0;
							break;
						case 3:
							rtmpPacket.rtmpPacketHeader.size = 1;
							rtmpPacket.rtmpPacketHeader.timestamp=0;
							rtmpPacket.rtmpPacketHeader.AMFSize=0;
							rtmpPacket.rtmpPacketHeader.AMFType=0;
							rtmpPacket.rtmpPacketHeader.streamID=0;
							break;
					}
		
					
					pos += rtmpPacket.rtmpPacketHeader.size;
					
					if ( rtmpPacket.rtmpPacketHeader.AMFSize != 0 )
					{
						if ( rtmpPacket.rtmpPacketHeader.AMFSize > XtraRtmp::MAX_PAYLOAD_SIZE )
						{
							return MISS_LENGTH_OVERFLOW;
						}
						memcpy(rtmpPacket.rtmpPacketPayload, rtmpRequest+pos, 
								rtmpPacket.rtmpPacketHeader.AMFSize);
					}
					
					pos += rtmpPacket.rtmpPacketHeader.AMFSize;
				}
			}
			else
			{
				return MISS_ARGUMENT_ERROR;
			}
			
			return MISS_OK;
		}
		
			
		void parseRtmpAMF0(unsigned char *buffer, size_t len, AmfPacket & amfPacket, 
											   RtmpMessageType  rtmpMessageType)
		{
			#define ToParseRtmpAMF0 __func__
			if (buffer == NULL || len == 0)
			{
				return;
			}
		
			amfPacket.eventType = -1;
			amfPacket.windowAcknowledgementSize = 0;
			amfPacket.command = "";
			amfPacket.transactionID[0]=0;
			amfPacket.flag[0]=0;
			amfPacket.streamName="";
			amfPacket.publishType = "";
			amfPacket.streamIDOrMilliSeconds = -1;
		
			if ( rtmpMessageType == XtraRtmp::MESSAGE_WINDOW_ACKNOWLEDGEMENT_SIZE)
			{
				amfPacket.windowAcknowledgementSize = buffer[0]*65536*256+buffer[1]*65536+buffer[2]*256+buffer[3];
				return;
			}
			else if ( rtmpMessageType == XtraRtmp::MESSAGE_USER_CONTROL )
			{
				amfPacket.eventType = static_cast<short>(buffer[0]*256+buffer[1]);
				return; 
			}

			size_t pos = 0;
			bool AMFObjectAlreadyStart = false;
			std::string coreString;
			std::string key,value;
			size_t stringLen,keyLen;
			int countString = 0;
			int countNumber = 0;
				
			while ( pos < len )
			{
				if ( !AMFObjectAlreadyStart )
				{
					switch (buffer[pos])
					{
						case XtraRtmp::TYPE_CORE_NUMBER:
							if ( countNumber == 0 )
							{
								countNumber += 1;
								amfPacket.transactionID[0] = 1;
								memcpy(amfPacket.transactionID+1, (char *)buffer+pos+1, XtraRtmp::LEN_CORE_NUMBER);
							}
							else
							{
								;
							}
							pos+=9;
							break;
						case XtraRtmp::TYPE_CORE_BOOLEAN:
							amfPacket.flag[0] = 1;
							amfPacket.flag[1] = (char)buffer[pos+1];
							pos+=2;
							break;
						case XtraRtmp::TYPE_CORE_STRING:
							stringLen = buffer[pos+1]*16+buffer[pos+2];
							pos += 3;
							if ( countString == 0 )
							{
								countString += 1;
								amfPacket.command = std::string((char *)buffer+pos,stringLen);
							}
							else if (countString == 1)
							{
								countString += 1;
								amfPacket.streamName = std::string((char *)buffer+pos,stringLen);
							}
							else if (countString == 2)
							{
								countString += 1;
								amfPacket.publishType = std::string((char *)buffer+pos,stringLen);
							}
							pos += stringLen;
							break;
						case XtraRtmp::TYPE_CORE_OBJECT:
							AMFObjectAlreadyStart = true;
							pos+=1; 
							break;
						case XtraRtmp::TYPE_CORE_NULL:
							pos+=1;
							break;
						case XtraRtmp::TYPE_CORE_MAP:
							pos+=5;
							break;	
					}
				}
				else
				{
					keyLen = buffer[pos]*16+buffer[pos+1];
					pos+=2;
					key = std::string((char *)buffer+pos, keyLen);
					pos+=keyLen;
					if (buffer[pos] == XtraRtmp::TYPE_CORE_STRING )
					{
						stringLen = buffer[pos+1]*16+buffer[pos+2];
						pos+=3;
						coreString=std::string((char *)buffer+pos,stringLen);
						pos += stringLen;
						amfPacket.parameters.insert(std::make_pair(key, coreString));
					}
						
					if (pos+3 <= len && buffer[pos]==0 && buffer[pos+1]==0 && buffer[pos+2]==0x09)
					{
						AMFObjectAlreadyStart=false;
						break;
					}
				}
			}
		}

		
		void rtmpMessageDump(const XtraRtmp::RtmpMessageType & rtmpMessageType, Nana *nana)
		{
			#define ToRtmpMessageDump __func__
			switch (rtmpMessageType)
			{
				case XtraRtmp::MESSAGE_CHANGE_CHUNK_SIZE:
					nana->say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF CHANGING THE CHUNK SIZE FOR PACKETS");
					break;
				case XtraRtmp::MESSAGE_DROP_CHUNK:
					nana->say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF DROPING THE CHUNK IDENTIFIED BY STREAM CHUNK ID");
					break;
				case XtraRtmp::MESSAGE_SEND_BOTH_READ:
					nana->say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF SENDING EVERY X BYTES READ BY BOTH SIDES");
					break;
				case XtraRtmp::MESSAGE_USER_CONTROL:
					nana->say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF PING,WHICH HAS SUBTYPES");
					break;
				case XtraRtmp::MESSAGE_WINDOW_ACKNOWLEDGEMENT_SIZE:
					nana->say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF THE SERVERS DOWNSTREAM BW");
					break;
				case XtraRtmp::MESSAGE_SET_PEER_BANDWIDTH:
					nana->say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF THE CLIENTS UPSTREAM BW");
					break;
				case XtraRtmp::MESSAGE_AUDIO:
					nana->say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF PACKET CONTAINING AUDIO");
					break;
				case XtraRtmp::MESSAGE_VIDEO:
					nana->say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF PACKET CONTAINING VIDEO");
					break;
				case XtraRtmp::MESSAGE_AMF0_DATA:
					nana->say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF AMF0 DATA");
					break;
				case XtraRtmp::MESSAGE_SUBTYPE:
					nana->say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF SHARED OBJECT WHICH HAS SUBTYPE");
					break;
				case XtraRtmp::MESSAGE_INVOKE:
					nana->say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF INVOKE");
					break;
				default:
					nana->say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF UNKNOWN %x",rtmpMessageType);
					break;
			}
		}


		void rtmpAMF0Dump(const XtraRtmp::AmfPacket & amfPacket, Nana* nana)
		{
			#define ToRtmpAmf0Dump __func__
			nana->say(Nana::HAPPY, ToRtmpAmf0Dump,"+++++++++++++++START+++++++++++++++");
		
			if ( amfPacket.eventType != -1 )
			{
				switch ( amfPacket.eventType )
				{
					case 3:
						nana->say(Nana::HAPPY, ToRtmpAmf0Dump, "EVENT TYPE:Set Buffer Length");
						break;
					default:
						break;
				}
			}
				
			if ( amfPacket.windowAcknowledgementSize > 0 )
			{
				nana->say(Nana::HAPPY, ToRtmpAmf0Dump, "WINDOW ACKNOWLEDGEMENT SIZE %u", amfPacket.windowAcknowledgementSize);
			}
			
			if (!amfPacket.command.empty())
			{
				nana->say(Nana::HAPPY, ToRtmpAmf0Dump, "COMMAND NAME:%s", amfPacket.command.c_str()); 
			}
					
			if (amfPacket.transactionID[0] == 1)
			{
				nana->say(Nana::HAPPY, ToRtmpAmf0Dump, "TRANSACTION ID:%0x %0x %0x %0x %0x %0x %0x %0x",
													   amfPacket.transactionID[1],
													   amfPacket.transactionID[2],
													   amfPacket.transactionID[3],
													   amfPacket.transactionID[4],
													   amfPacket.transactionID[5],
													   amfPacket.transactionID[6],
													   amfPacket.transactionID[7],
													   amfPacket.transactionID[8]
													   );
			}
	
			if (amfPacket.flag[0] != 0)
			{
				nana->say(Nana::HAPPY, ToRtmpAmf0Dump, "FLAG:%x", amfPacket.flag[1]);
			}
	
			if (!amfPacket.streamName.empty())
			{
				nana->say(Nana::HAPPY, ToRtmpAmf0Dump, "STREAM NAME:%s", amfPacket.streamName.c_str()); 
			}
	
			if (!amfPacket.publishType.empty())
			{
				nana->say(Nana::HAPPY, ToRtmpAmf0Dump, "PUBLISH TYPE:%s", amfPacket.publishType.c_str()); 
			}
			
			if( !amfPacket.parameters.empty() )
			{
				std::map<std::string, std::string>::const_iterator coo_iter = amfPacket.parameters.begin();
				nana->say(Nana::HAPPY, ToRtmpAmf0Dump, "CORE OBJECT EXISTS,THAT THE VALUE TYPE IS CORE STRING");
				while (coo_iter != amfPacket.parameters.end())
				{
					nana->say(Nana::HAPPY, ToRtmpAmf0Dump, "KEY:%s", coo_iter->first.c_str());
					nana->say(Nana::HAPPY, ToRtmpAmf0Dump, "VALUE:%s", coo_iter->second.c_str());
					++coo_iter;
				}
			}
				
			nana->say(Nana::HAPPY, ToRtmpAmf0Dump,"+++++++++++++++DONE+++++++++++++++");
				
		}
	};
};
