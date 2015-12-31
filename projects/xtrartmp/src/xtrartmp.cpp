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
	//define the global macro for libev's event clean
	#define DO_EVENT_CB_CLEAN(M,W) \
    	ev_io_stop(M, W);\
    	delete W;\
    	W= NULL;\
    	return;

	
	//define the global variable for xtrartmp server's sake
	//just simply varify the app field
	static const std::string APP="xtrartmp";
	
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
					struct ev_io * consultWatcher = new struct ev_io;
    					if ( consultWatcher == NULL )
    					{
        					log_module(LOG_ERROR, "shakeHandCallback", "ALLOCATE MEMORY FAILED:%s", strerror( errno ));
        					DO_EVENT_CB_CLEAN(mainEventLoop,receiveRequestWatcher);
    					}
    
    					consultWatcher->active = 0;
    					consultWatcher->data = 0;   
    					ev_io_init(  consultWatcher, consultCallback, receiveRequestWatcher->fd, EV_READ );
    					ev_io_start( mainEventLoop, consultWatcher);
				}
			}
    		}
		DO_EVENT_CB_CLEAN(mainEventLoop,receiveRequestWatcher);
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



	bool XtraRtmp::varifyRtmpAMF0(Amf0 & amf0, const std::string &app)
	{
		return amf0.coreObjectOfString["app"] == app;
	}



	void  consultCallback(struct ev_loop * mainEventLoop, struct ev_io * consultWatcher, int revents)
	{
		(void)revents;
		#define MSG_MODULE_CONSULT "consultCallback" 
		unsigned char consultRequest[1024];
		int readBytes=read(consultWatcher->fd, consultRequest, sizeof(consultRequest));
		if ( readBytes > 0 )
		{
			log_module( LOG_DEBUG, "consultCallback", "READ THE RTMP PACKET %d BYTES", readBytes);
			RtmpPacket rtmpPacket;
			int ret = XtraRtmp::parseRtmpPacket(consultRequest, static_cast<size_t>(readBytes), rtmpPacket);
			Amf0 amf0;
			if (ret == OK )
			{
				log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "RTMP HEADER SIZE:%u", rtmpPacket.rtmpPacketHeader.size );
				log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "RTMP HEADER CHUNK STREAM ID:%u", rtmpPacket.rtmpPacketHeader.chunkStreamID);
				log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "RTMP HEADER TIMESTAMP:%u", rtmpPacket.rtmpPacketHeader.timestamp);
				log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "RTMP HEADER AMF SIZE:%u", rtmpPacket.rtmpPacketHeader.AMFSize);
				log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "RTMP HEADER AMF TYPE:%u", rtmpPacket.rtmpPacketHeader.AMFType);
				log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "RTMP HEADER STREAM ID:%u", rtmpPacket.rtmpPacketHeader.streamID);

				XtraRtmp::parseRtmpAMF0(rtmpPacket.rtmpPacketPayload,  rtmpPacket.rtmpPacketHeader.AMFSize, amf0);
				XtraRtmp::rtmpAMF0Dump(amf0);
				//the state machine
				switch (rtmpPacket.rtmpPacketHeader.AMFType)
				{
					case RTMP_MESSAGE_CHANGE_CHUNK_SIZE:
						log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "THIS IS THE RTMP MESSAGE OF CHANGING THE CHUNK SIZE FOR PACKETS");
						break;
					case RTMP_MESSAGE_DROP_CHUNK:
						log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "THIS IS THE RTMP MESSAGE OF DROPING THE CHUNK IDENTIFIED BY STREAM CHUNK ID");
						break;
					case RTMP_MESSAGE_SEND_BOTH_READ:
						log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "THIS IS THE RTMP MESSAGE OF SENDING EVERY X BYTES READ BY BOTH SIDES");
						break;
					case RTMP_MESSAGE_PING:
						log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "THIS IS THE RTMP MESSAGE OF PING,WHICH HAS SUBTYPES");
						break;
					case RTMP_MESSAGE_SERVER_DOWNSTREAM:
						log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "THIS IS THE RTMP MESSAGE OF THE SERVERS DOWNSTREAM BW");
						break;
					case RTMP_MESSAGE_CLIENT_UPSTREAM:
						log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "THIS IS THE RTMP MESSAGE OF THE CLIENTS UPSTREAM BW");
						break;
					case RTMP_MESSAGE_AUDIO:
						log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "THIS IS THE RTMP MESSAGE OF PACKET CONTAINING AUDIO");
						break;
					case RTMP_MESSAGE_VIDEO:
						log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "THIS IS THE RTMP MESSAGE OF PACKET CONTAINING VIDEO");
						break;
					case RTMP_MESSAGE_INVOKE_WITHOUT_REPLY:
						log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "THIS IS THE RTMP MESSAGE OF AN INVOKE WHICH DOES NOT EXPECT A REPLY");
						break;
					case RTMP_MESSAGE_SUBTYPE:
						log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "THIS IS THE RTMP MESSAGE OF SHARED OBJECT WHICH HAS SUBTYPE");
						break;
					case RTMP_MESSAGE_INVOKE:
						log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "THIS IS THE RTMP MESSAGE OF INVOKE WHICH LIKE REMOTING CALL, USED FOR STREAM ACTIONS TOO");
						handleRtmpInvokeMessage(amf0, consultCallback->fd);
						break;
					default:
						log_module(LOG_DEBUG, MSG_MODULE_CONSULT, "THIS IS THE RTMP MESSAGE OF UNKNOWN");
						break;
					}
				}
				log_module( LOG_INFO, MSG_MODULE_CONSULT,"++++++++++RTMP INTERACTIVE SIGNALLING DONE++++++++++");
			}
			else
			{
				log_module(LOG_ERROR, MSG_MODULE_CONSULT, "ERROR OCCURRED WHEN READ:%s", strerror(errno));
				log_module(LOG_INFO, MSG_MODULE_CONSULT,"++++++++++RTMP INTERACTIVE SIGNALLING FAILED++++++++++");
			}
		DO_EVENT_CB_CLEAN(mainEventLoop, consultWatcher);
	}



	void XtraRtmp::handleRtmpInvokeMessage(const Amf0 &amf0, int connFd)
	{
		#define MSG_MODULE_INVOKE "handleRtmpInvokeMessage"
		
		//just simply varify the app field
		bool varifyAmf0Ok = false;
		varifyAmf0Ok = varifyRtmpAMF0(amf0, APP);
		if ( varifyAmf0Ok )
		{
			log_module(LOG_DEBUG, MSG_MODULE_INVOKE, "VARIFY AMF0 APP FIELD OK: APP IS %s", APP.c_str());
			if (amf0.coreString == AMF_CONNECT_COMMAND)
			{
			}
			else if(amf0.coreString == AMF_CREATE_STREAM_COMMAND)
			{
			}
			else if(amf0.coreString == AMF_PLAY_COMMAND)
			{
				
			}
			else if(amf0.coreString == AMF_PLAY2_COMMAND)
			{
				
			}
			else if(amf0.coreString == AMF_DELETE_STREAM_COMMAND)
			{
				
			}
			else if(amf0.coreString == AMF_RECEIVE_AUDIO_COMMAND)
			{
				
			}
			else if(amf0.coreString == AMF_RECEIVE_VIDEO_COMMAND)
			{
				
			}
			else if(amf0.coreString == AMF_PUBLISH_COMMAND)
			{
				
			}
			else if(amf0.coreString == AMF_SEEK_COMMAND)
			{
				
			}
			else if(amf0.coreString == AMF_PAUSE_COMMAND)
			{
				
			}
		}
		else
		{
			log_module(LOG_DEBUG, MSG_MODULE_INVOKE, "VARIFY AMF0 APP FIELD FAILED:INVALID APP:%s THE CORRECT APP IS:%s", 
																	   amf0.coreObjectOfString["app"].empty() ? "empty": amf0.coreObjectOfString["app"].c_str(), APP.c_str());
		}
	}
};
