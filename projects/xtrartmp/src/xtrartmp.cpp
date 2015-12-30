/*
*@filename:xtrartmp.cpp
*@author:chenzhengqiang
*@start date:2015/11/26 11:26:16
*@modified date:
*@desc: 
*/

#include "common.h"
#include "errors.h"
#include "netutil.h"
#include "serverutil.h"
#include "logging.h"
#include "xtrartmp.h"
#include<iostream>
#include<cstdlib>

using std::cout;
using std::endl;
namespace czq
{
	//define the global's log facility
	LogFacility XtraRtmp::logFacility_ = LOG_FILE;
	XtraRtmp::XtraRtmp( const ServerConfig & serverConfig ):listenFd_(-1),serverConfig_(serverConfig),
														mainEventLoop_(0),listenWatcher_(0),acceptWatcher_(0)
	{
		;	
	}

	XtraRtmp::~XtraRtmp()
	{
		if ( mainEventLoop_ != 0 )
		free(mainEventLoop_);
		if ( listenWatcher_ !=0 )
		delete listenWatcher_;
		if ( acceptWatcher_ != 0 )
		delete acceptWatcher_;
	}
	void XtraRtmp::printHelp()
	{
		cout<<serverConfig_.usage<<endl;
		exit(EXIT_SUCCESS);
	}

	void XtraRtmp::printVersion()
	{
		cout<<serverConfig_.meta["version"]<<endl;
		exit(EXIT_SUCCESS);
	}

	void XtraRtmp::registerServer( int listenFd )
	{
		listenFd_ = listenFd;
	}

	void XtraRtmp::serveForever()
	{
		if ( listenFd_ >= 0 )
		{
			if ( serverConfig_.server["daemon"] == "yes" )
    			{
        			daemon(0,0);
    			}
    			
    			logging_init( serverConfig_.server["log-file"].c_str(), atoi(serverConfig_.server["log-level"].c_str()));
    			log_module( LOG_DEBUG, "serveForever",  "LISTEN SOCKET FD:%d", listenFd_);
    
    			//you have to ignore the PIPE's signal when client close the socket
    			struct sigaction sa;
    			sa.sa_handler = SIG_IGN;//just ignore the sigpipe
    			sa.sa_flags = 0;
    			if ( sigemptyset( &sa.sa_mask ) == -1 ||sigaction( SIGPIPE, &sa, 0 ) == -1 )
    			{ 
        			log_module(LOG_ERROR,"serveForever","FAILED TO IGNORE SIGPIPE SIGNAL");
    			}
			else
			{
    				//initialization on main event-loop
    				mainEventLoop_ = EV_DEFAULT;
    				listenWatcher_ = new ev_io;
    				ev_io_init( listenWatcher_, acceptCallback, listenFd_, EV_READ );
             			ev_io_start( mainEventLoop_, listenWatcher_ );
            			ev_run( mainEventLoop_,0 );
    			}
		}
		else
		{
			;
		}
	}

	void acceptCallback( struct ev_loop * mainEventLoop, struct ev_io * listenWatcher, int revents )
	{
		    
    		if ( EV_ERROR & revents )
    		{
        		log_module(LOG_ERROR, "acceptCallback", "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
        		return;
    		}

    		
    		struct sockaddr_in clientAddr;
    		socklen_t len = sizeof( struct sockaddr_in );
    		ssize_t connFd = accept( listenWatcher->fd, (struct sockaddr *)&clientAddr, &len );
    		if ( connFd < 0 )
    		{
        		log_module(LOG_ERROR, "acceptCallback", "ACCEPT ERROR:%s", strerror(errno));   
        		return;
    		}
    		
    		struct ev_io * receiveRequestWatcher = new struct ev_io;
    		if ( receiveRequestWatcher == NULL )
    		{
        		log_module(LOG_ERROR, "acceptCallback", "ALLOCATE MEMORY FAILED:%s", strerror( errno ));
        		close( connFd );
        		return;
    		}
    
    		receiveRequestWatcher->active = 0;
    		receiveRequestWatcher->data = 0;   
    		if ( loglevel_is_enabled( LOG_INFO ))
    		{
        		std::string peerInfo = NetUtil::getPeerInfo( connFd );
        		log_module( LOG_INFO, "acceptCallback", "CLIENT %s CONNECTED SOCK FD IS:%d",
                                                             peerInfo.c_str(), connFd );
    		}
    
    		//register the socket io callback for reading client's request    
    		ev_io_init(  receiveRequestWatcher, shakeHandCallback, connFd, EV_READ );
    		ev_io_start( mainEventLoop, receiveRequestWatcher );
	}

	void shakeHandCallback( struct ev_loop * mainEventLoop, struct ev_io * receiveRequestWatcher, int revents )
	{
		(void)revents;
		(void)mainEventLoop;
		(void)receiveRequestWatcher;

		#define DO_RECEIVE_REQUEST_CB_CLEAN() \
    		ev_io_stop( mainEventLoop, receiveRequestWatcher);\
    		delete receiveRequestWatcher;\
    		receiveRequestWatcher= NULL;\
    		return;

    		char c0,s0;
		C1 c1;	
    		log_module( LOG_INFO, "shakeHandCallback", "++++++++++RTMP SHAKE HAND START++++++++++");
		struct iovec iovRead[2];
		iovRead[0].iov_base=&c0;
		iovRead[0].iov_len=1;
		iovRead[1].iov_base=&c1;
		iovRead[1].iov_len=sizeof(c1);
		int readBytes=readv(receiveRequestWatcher->fd, iovRead, 2);
    		if ( readBytes != -1 )
    		{
    			log_module( LOG_INFO, "shakeHandCallback", "RECEIVE THE C0 AND C1 PACKET TOTAL BYTES:%d", readBytes);
			log_module( LOG_INFO, "shakeHandCallback","C0 VERSION:%d C1.timestamp:%u", c0, ntohl(c1.timestamp));	
    			s0=c0;
			S1 s1;	
    			s1.timestamp = htonl(0);
    			s1.zeroOrTime = htonl(0);
    			ServerUtil::generateSimpleRandomValue(s1.randomValue, RANDOM_VALUE_SIZE);
    			struct iovec iovWrite[2];
    			iovWrite[0].iov_base=&s0;
			iovWrite[0].iov_len=1;
			iovWrite[1].iov_base=&s1;
			iovWrite[1].iov_len=sizeof(s1);
			int wrotenBytes=writev(receiveRequestWatcher->fd,iovWrite,2);
			if ( wrotenBytes == -1)
			{
				log_module( LOG_INFO, "shakeHandCallback", "SENT THE S0,S1 PACKET FAILED:%s", strerror(errno));
			}
			else
			{
				log_module( LOG_INFO, "shakeHandCallback", "SENT THE S0,S1 PACKET SUCCESSED. TOTAL SENT BYTES:%d", wrotenBytes);
				C2 c2;
				readBytes=read(receiveRequestWatcher->fd, &c2, sizeof(c2));
				if ( readBytes == -1 )
				{
					log_module( LOG_INFO, "shakeHandCallback", "READ THE C2 PACKET FAILED:%s", strerror(errno));
				}
				else
				{
					log_module( LOG_INFO, "shakeHandCallback", "READ THE C2 PACKET SUCCESSED. TOTAL READ BYTES:%d C2.S1.timestamp:%u C2.C1.timestamp:%u", 
															     readBytes,ntohl(c2.timestamp),ntohl(c2.zeroOrTime));
					S2 s2;
					s2.timestamp=c1.timestamp;
					s2.zeroOrTime=s1.timestamp;
					memcpy(s2.randomValue, c1.randomValue, RANDOM_VALUE_SIZE);
					wrotenBytes=write(receiveRequestWatcher->fd, &s2, sizeof(s2));
					log_module( LOG_INFO, "shakeHandCallback", "++++++++++RTMP SHAKE HAND DONE++++++++++");
					log_module( LOG_INFO, "shakeHandCallback","++++++++++RTMP INTERACTIVE SIGNALLING START++++++++++");
					unsigned char connectRequest[1024];
					readBytes=read(receiveRequestWatcher->fd, connectRequest, sizeof(connectRequest));
					if ( readBytes > 0 )
					{
						log_module( LOG_DEBUG, "shakeHandCallback", "READ THE RTMP CONNECT SIGNALLING %d BYTES", readBytes);
						RtmpPacket rtmpPacket;
						int ret = XtraRtmp::parseRtmpPacket(connectRequest, static_cast<size_t>(readBytes), rtmpPacket);
						Amf0 amf0;
						if (ret == OK )
						{
							log_module(LOG_DEBUG, "shakeHandCallback", "RTMP HEADER SIZE:%u", rtmpPacket.rtmpPacketHeader.size );
							log_module(LOG_DEBUG, "shakeHandCallback", "RTMP HEADER CHUNK STREAM ID:%u", rtmpPacket.rtmpPacketHeader.chunkStreamID);
							log_module(LOG_DEBUG, "shakeHandCallback", "RTMP HEADER TIMESTAMP:%u", rtmpPacket.rtmpPacketHeader.timestamp);
							log_module(LOG_DEBUG, "shakeHandCallback", "RTMP HEADER AMF SIZE:%u", rtmpPacket.rtmpPacketHeader.AMFSize);
							log_module(LOG_DEBUG, "shakeHandCallback", "RTMP HEADER AMF TYPE:%u", rtmpPacket.rtmpPacketHeader.AMFType);
							log_module(LOG_DEBUG, "shakeHandCallback", "RTMP HEADER STREAM ID:%u", rtmpPacket.rtmpPacketHeader.streamID);

							switch(rtmpPacket.rtmpPacketHeader.AMFType)
							{
								case 0x01:
									log_module(LOG_DEBUG, "shakeHandCallback", "THIS IS THE RTMP MESSAGE OF CHANGING THE CHUNK SIZE FOR PACKETS");
									break;
								case 0x02:
									log_module(LOG_DEBUG, "shakeHandCallback", "THIS IS THE RTMP MESSAGE OF DROPING THE CHUNK IDENTIFIED BY STREAM CHUNK ID");
									break;
								case 0x03:
									log_module(LOG_DEBUG, "shakeHandCallback", "THIS IS THE RTMP MESSAGE OF SENDING EVERY X BYTES READ BY BOTH SIDES");
									break;
								case 0x04:
									log_module(LOG_DEBUG, "shakeHandCallback", "THIS IS THE RTMP MESSAGE OF PING,WHICH HAS SUBTYPES");
									break;
								case 0x05:
									log_module(LOG_DEBUG, "shakeHandCallback", "THIS IS THE RTMP MESSAGE OF THE SERVERS DOWNSTREAM BW");
									break;
								case 0x06:
									log_module(LOG_DEBUG, "shakeHandCallback", "THIS IS THE RTMP MESSAGE OF THE CLIENTS UPSTREAM BW");
									break;
								case 0x08:
									log_module(LOG_DEBUG, "shakeHandCallback", "THIS IS THE RTMP MESSAGE OF PACKET CONTAINING AUDIO");
									break;
								case 0x09:
									log_module(LOG_DEBUG, "shakeHandCallback", "THIS IS THE RTMP MESSAGE OF PACKET CONTAINING VIDEO");
									break;
								case 0x12:
									log_module(LOG_DEBUG, "shakeHandCallback", "THIS IS THE RTMP MESSAGE OF AN INVOKE WHICH DOES NOT EXPECT A REPLY");
									break;
								case 0x13:
									log_module(LOG_DEBUG, "shakeHandCallback", "THIS IS THE RTMP MESSAGE OF SHARED OBJECT WHICH HAS SUBTYPES");
									break;
								case 0x14:
									log_module(LOG_DEBUG, "shakeHandCallback", "THIS IS THE RTMP MESSAGE OF INVOKE WHICH LIKE REMOTING CALL, USED FOR STREAM ACTIONS TOO");
									XtraRtmp::parseRtmpAMF0(rtmpPacket.rtmpPacketPayload,  rtmpPacket.rtmpPacketHeader.AMFSize, amf0);
									XtraRtmp::rtmpAMF0Dump(amf0);
									break;
								default:
									log_module(LOG_DEBUG, "shakeHandCallback", "THIS IS THE RTMP MESSAGE OF UNKNOWN");
									break;
							}
						}
						log_module( LOG_INFO, "shakeHandCallback","++++++++++RTMP INTERACTIVE SIGNALLING DONE++++++++++");
					}
					else
					{
						log_module(LOG_ERROR, "shakeHandCallback", "ERROR OCCURRED WHEN READ:%s", strerror(errno));
						log_module( LOG_INFO, "shakeHandCallback","++++++++++RTMP INTERACTIVE SIGNALLING FAILED++++++++++");
					}
				}
			}
    		}
		DO_RECEIVE_REQUEST_CB_CLEAN();
	}

	int XtraRtmp::parseRtmpPacket(unsigned char *rtmpRequest, size_t len, RtmpPacket & rtmpPacket)
	{
		if ( len == 0 )
		{
			return ARGUMENT_ERROR;
		}

		memset(&rtmpPacket.rtmpPacketHeader, 0, sizeof(rtmpPacket.rtmpPacketHeader));
		//first parse the format which hold the high 2 bit
		char format = rtmpRequest[0] & 0xc0;
		switch (format)
		{
			case 0:
				rtmpPacket.rtmpPacketHeader.size = 12;
				rtmpPacket.rtmpPacketHeader.timestamp = rtmpRequest[1]*256+rtmpRequest[2]*16+rtmpRequest[3];
				rtmpPacket.rtmpPacketHeader.AMFSize = rtmpRequest[4]*256+rtmpRequest[5]*16+rtmpRequest[6];
				rtmpPacket.rtmpPacketHeader.AMFType = rtmpRequest[7];
				rtmpPacket.rtmpPacketHeader.streamID = rtmpRequest[8]*65536+rtmpRequest[9]*256
													+rtmpRequest[10]*16+rtmpRequest[11];
				break;
			case 1:
				rtmpPacket.rtmpPacketHeader.size = 8;
				rtmpPacket.rtmpPacketHeader.timestamp = rtmpRequest[1]*256+rtmpRequest[2]*16+rtmpRequest[3];
				rtmpPacket.rtmpPacketHeader.AMFSize = rtmpRequest[4]*256+rtmpRequest[5]*16+rtmpRequest[6];
				rtmpPacket.rtmpPacketHeader.AMFType = rtmpRequest[7];
				rtmpPacket.rtmpPacketHeader.streamID=0;
				break;
			case 2:
				rtmpPacket.rtmpPacketHeader.size = 4;
				rtmpPacket.rtmpPacketHeader.timestamp = rtmpRequest[1]*256+rtmpRequest[2]*16+rtmpRequest[3];
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
		
		if ( rtmpPacket.rtmpPacketHeader.AMFSize != 0 )
		{
			rtmpPacket.rtmpPacketPayload = new unsigned char[rtmpPacket.rtmpPacketHeader.AMFSize];
			if ( rtmpPacket.rtmpPacketPayload == NULL )
			{
				log_module(LOG_ERROR, "parseRtmpPacket", "MEMORY ALLOCATE ERROR:%s", strerror(errno));
				return SYSTEM_ERROR;
			}
			memcpy(rtmpPacket.rtmpPacketPayload, rtmpRequest+rtmpPacket.rtmpPacketHeader.size,
											     len-rtmpPacket.rtmpPacketHeader.size);
		}
		rtmpPacket.rtmpPacketHeader.chunkStreamID = rtmpRequest[0] & 0x3f;
		return OK;
	}


	void XtraRtmp::parseRtmpAMF0(unsigned char *buffer, size_t len, Amf0 & amf0)
	{
		if (buffer == NULL || len == 0)
		{
			return;
		}

		amf0.coreBoolean = -1;
		amf0.coreString = "";
		amf0.coreNumber[0]=0;
		size_t pos = 0;
		bool AMFObjectAlreadyStart = false;
		std::string coreString;
		std::string key,value;
		size_t stringLen,keyLen;
		
		while (pos <= len)
		{
			if ( !AMFObjectAlreadyStart )
			{
				switch (buffer[pos])
				{
					case TYPE_CORE_NUMBER:
						amf0.coreNumber[0]=1;
						memcpy(amf0.coreNumber+1, (char *)buffer+pos+1, LEN_CORE_NUMBER);
						pos+=9;
						break;
					case TYPE_CORE_BOOLEAN:
						amf0.coreBoolean = (char)buffer[pos+1];
						pos+=2;
						break;
					case TYPE_CORE_STRING:
						stringLen = buffer[pos+1]*16+buffer[pos+2];
						pos += 3;
						amf0.coreString = std::string((char *)buffer+pos,stringLen);
						pos += stringLen;
						break;
					case TYPE_CORE_OBJECT:
						AMFObjectAlreadyStart = true;
						pos+=1; 
						break;
					case TYPE_CORE_NULL:
						pos+=1;
						break;
					case TYPE_CORE_MAP:
						pos+=5;
						break;	
				}
			}
			else
			{
				keyLen = buffer[pos];
				pos+=1;
				if (keyLen == 0 )
				continue;	
				key = std::string((char *)buffer+pos, keyLen);
				pos+=keyLen;
				if ( buffer[pos] == TYPE_CORE_STRING )
				{
					stringLen = buffer[pos+1]*16+buffer[pos+2];
					pos+=3;
					coreString=std::string((char *)buffer+pos,stringLen);
					pos += stringLen;
					amf0.coreObjectOfString.insert(std::make_pair(key, coreString));
				}
				
				if (pos+3 <= len && buffer[pos]==0 && buffer[pos+1]==0 && buffer[pos+2]==0x09)
				{
					AMFObjectAlreadyStart=false;
				}
			}
		}
	}


	void XtraRtmp::rtmpAMF0Dump(const Amf0 & amf0)
	{
		if (logFacility_ == LOG_FILE)
		{
			log_module(LOG_DEBUG, "rtmpAMF0Dump","+++++++++++++++START+++++++++++++++");
			if (amf0.coreNumber[0] == 1)
			{
				log_module(LOG_DEBUG, "rtmpAMF0Dump", "CORE NUMBER EXISTS");
			}

			if (!amf0.coreString.empty())
			{
				log_module(LOG_DEBUG, "rtmpAMF0Dump", "CORE STRING:%s", amf0.coreString.c_str());	
			}

			if (amf0.coreBoolean != -1)
			{
				log_module(LOG_DEBUG, "rtmpAMF0Dump", "CORE BOOLEAN:%x", amf0.coreBoolean);
			}
			if( !amf0.coreObjectOfString.empty() )
			{
				std::map<std::string, std::string>::const_iterator coo_iter = amf0.coreObjectOfString.begin();
				log_module(LOG_DEBUG, "rtmpAMF0Dump", "CORE OBJECT EXISTS,THAT THE VALUE TYPE IS CORE STRING");
				while (coo_iter != amf0.coreObjectOfString.end())
				{
					log_module(LOG_DEBUG, "rtmpAMF0Dump", "KEY:%s", coo_iter->first.c_str());
					log_module(LOG_DEBUG, "rtmpAMF0Dump", "VALUE:%s", coo_iter->second.c_str());
					++coo_iter;
				}
			}
			log_module(LOG_DEBUG, "rtmpAMF0Dump","+++++++++++++++DONE+++++++++++++++");
		}
	}
};
