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
#include "nana.h"
#include "xtrartmp.h"
#include<iostream>
#include<cstdlib>

namespace czq
{
	//define the global macro for libev's event clean
	#define DO_EVENT_CB_CLEAN(M,W) \
    	ev_io_stop(M, W);\
    	delete W;\
    	W= NULL;\
    	return;

	//the amf command
	namespace AmfCommand
       {
       	static const char *connect = "connect";
        	static const char *play = "play";
        	static const char *play2 = "play2";
        	static const char *seek = "seek";
        	static const char *pause = "pause";
        	static const char *publish = "publish";
        	static const char *createStream = "createStream";
        	static const char *deleteStream = "deleteStream";
        	static const char *receiveAudio = "receiveAudio";
        	static const char *receiveVideo = "receiveVideo";
       };
       
	//define the global variable for xtrartmp server's sake
	//just simply varify the app field
	static const std::string APP="xtrartmp";
	
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
		std::cout<<serverConfig_.usage<<std::endl;
		exit(EXIT_SUCCESS);
	}

	void XtraRtmp::printVersion()
	{
		std::cout<<serverConfig_.meta["version"]<<std::endl;
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
    			
    			Nana::born(serverConfig_.server["log-file"], atoi(serverConfig_.server["log-level"].c_str()),
    					   atoi(serverConfig_.server["flush-time"].c_str()));
    			Nana::say(Nana::HAPPY, __func__,  "LISTEN SOCKET FD:%d", listenFd_);
    
    			//you have to ignore the PIPE's signal when client close the socket
    			struct sigaction sa;
    			sa.sa_handler = SIG_IGN;//just ignore the sigpipe
    			sa.sa_flags = 0;
    			if ( sigemptyset( &sa.sa_mask ) == -1 ||sigaction( SIGPIPE, &sa, 0 ) == -1 )
    			{ 
        			Nana::say(Nana::COMPLAIN,"serveForever","FAILED TO IGNORE SIGPIPE SIGNAL");
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
    			Nana::die();
		}
		else
		{
			;
		}
	}

	void acceptCallback( struct ev_loop * mainEventLoop, struct ev_io * listenWatcher, int revents )
	{
		#define ToAcceptCallback __func__

    		if ( EV_ERROR & revents )
    		{
        		Nana::say(Nana::COMPLAIN, ToAcceptCallback, "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
        		return;
    		}

    		
    		struct sockaddr_in clientAddr;
    		socklen_t len = sizeof( struct sockaddr_in );
    		ssize_t connFd = accept( listenWatcher->fd, (struct sockaddr *)&clientAddr, &len );
    		if ( connFd < 0 )
    		{
        		Nana::say(Nana::COMPLAIN, ToAcceptCallback, "ACCEPT ERROR:%s", strerror(errno));   
        		return;
    		}
    		
    		struct ev_io * receiveRequestWatcher = new struct ev_io;
    		if ( receiveRequestWatcher == NULL )
    		{
        		Nana::say(Nana::COMPLAIN, ToAcceptCallback, "ALLOCATE MEMORY FAILED:%s", strerror( errno ));
        		close( connFd );
        		return;
    		}
    
    		receiveRequestWatcher->active = 0;
    		receiveRequestWatcher->data = 0;   
    		if ( Nana::is(Nana::PEACE))
    		{
        		std::string peerInfo = NetUtil::getPeerInfo( connFd );
        		Nana::say(Nana::PEACE, ToAcceptCallback, "CLIENT %s CONNECTED SOCK FD IS:%d",
                                                             peerInfo.c_str(), connFd );
    		}
    
    		//register the socket io callback for reading client's request    
    		ev_io_init(  receiveRequestWatcher, shakeHandCallback, connFd, EV_READ );
    		ev_io_start( mainEventLoop, receiveRequestWatcher );
	}

	void shakeHandCallback( struct ev_loop * mainEventLoop, struct ev_io * receiveRequestWatcher, int revents )
	{
		#define ToShakeHandCallback __func__
    		char c0,s0;
		XtraRtmp::C1 c1;
		if ( EV_ERROR & revents )
    		{
        		Nana::say(Nana::COMPLAIN, ToShakeHandCallback, "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
        		return;
    		}
    		
    		Nana::say(Nana::HAPPY, ToShakeHandCallback, "++++++++++RTMP SHAKE HAND START++++++++++");
		struct iovec iovRead[2];
		iovRead[0].iov_base=&c0;
		iovRead[0].iov_len=1;
		iovRead[1].iov_base=&c1;
		iovRead[1].iov_len=sizeof(c1);
		int readBytes=readv(receiveRequestWatcher->fd, iovRead, 2);
    		if ( readBytes != -1 )
    		{
    			Nana::say(Nana::HAPPY, ToShakeHandCallback, "RECEIVE THE C0 AND C1 PACKET TOTAL BYTES:%d", readBytes);
			Nana::say(Nana::HAPPY, ToShakeHandCallback, "C0 VERSION:%d C1.timestamp:%u", c0, ntohl(c1.timestamp));	
    			s0=c0;
			XtraRtmp::S1 s1;	
    			s1.timestamp = htonl(0);
    			s1.zeroOrTime = htonl(0);
    			ServerUtil::generateSimpleRandomValue(s1.randomValue, XtraRtmp::RANDOM_VALUE_SIZE);
    			struct iovec iovWrite[2];
    			iovWrite[0].iov_base=&s0;
			iovWrite[0].iov_len=1;
			iovWrite[1].iov_base=&s1;
			iovWrite[1].iov_len=sizeof(s1);
			int wrotenBytes=writev(receiveRequestWatcher->fd,iovWrite,2);
			if ( wrotenBytes == -1)
			{
				Nana::say(Nana::COMPLAIN, ToShakeHandCallback, "SENT THE S0,S1 PACKET FAILED:%s", strerror(errno));
			}
			else
			{
				Nana::say(Nana::HAPPY, ToShakeHandCallback, "SENT THE S0,S1 PACKET SUCCESSED. TOTAL SENT BYTES:%d", wrotenBytes);
				XtraRtmp::C2 c2;
				readBytes=read(receiveRequestWatcher->fd, &c2, sizeof(c2));
				if ( readBytes == -1 )
				{
					Nana::say(Nana::COMPLAIN, ToShakeHandCallback, "READ THE C2 PACKET FAILED:%s", strerror(errno));
				}
				else
				{
					Nana::say(Nana::HAPPY, ToShakeHandCallback, "READ THE C2 PACKET SUCCESSED. TOTAL READ BYTES:%d C2.S1.timestamp:%u C2.C1.timestamp:%u", 
															     readBytes,ntohl(c2.timestamp),ntohl(c2.zeroOrTime));
					XtraRtmp::S2 s2;
					s2.timestamp=c1.timestamp;
					s2.zeroOrTime=s1.timestamp;
					memcpy(s2.randomValue, c1.randomValue, XtraRtmp::RANDOM_VALUE_SIZE);
					wrotenBytes=write(receiveRequestWatcher->fd, &s2, sizeof(s2));
					
					Nana::say(Nana::HAPPY, ToShakeHandCallback, "++++++++++RTMP SHAKE HAND DONE++++++++++");
					struct ev_io * consultWatcher = new struct ev_io;
    					if ( consultWatcher == NULL )
    					{
        					Nana::say(Nana::COMPLAIN, ToShakeHandCallback, "ALLOCATE MEMORY FAILED:%s", strerror( errno ));
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
				Nana::say(Nana::COMPLAIN, "parseRtmpPacket", "MEMORY ALLOCATE ERROR:%s", strerror(errno));
				return SYSTEM_ERROR;
			}
			memcpy(rtmpPacket.rtmpPacketPayload, rtmpRequest+rtmpPacket.rtmpPacketHeader.size,
											     len-rtmpPacket.rtmpPacketHeader.size);
		}
		rtmpPacket.rtmpPacketHeader.chunkStreamID = rtmpRequest[0] & 0x3f;
		return OK;
	}


	void XtraRtmp::parseRtmpAMF0(unsigned char *buffer, size_t len, AmfPacket & amfPacket)
	{
		if (buffer == NULL || len == 0)
		{
			return;
		}

		amfPacket.coreBoolean = -1;
		amfPacket.coreString = "";
		amfPacket.coreNumber[0]=0;
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
					case XtraRtmp::TYPE_CORE_NUMBER:
						amfPacket.coreNumber[0]=1;
						memcpy(amfPacket.coreNumber+1, (char *)buffer+pos+1, LEN_CORE_NUMBER);
						pos+=9;
						break;
					case XtraRtmp::TYPE_CORE_BOOLEAN:
						amfPacket.coreBoolean = (char)buffer[pos+1];
						pos+=2;
						break;
					case XtraRtmp::TYPE_CORE_STRING:
						stringLen = buffer[pos+1]*16+buffer[pos+2];
						pos += 3;
						amfPacket.coreString = std::string((char *)buffer+pos,stringLen);
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
				keyLen = buffer[pos];
				pos+=1;
				if (keyLen == 0 )
				continue;	
				key = std::string((char *)buffer+pos, keyLen);
				pos+=keyLen;
				if ( buffer[pos] == XtraRtmp::TYPE_CORE_STRING )
				{
					stringLen = buffer[pos+1]*16+buffer[pos+2];
					pos+=3;
					coreString=std::string((char *)buffer+pos,stringLen);
					pos += stringLen;
					amfPacket.coreObjectOfString.insert(std::make_pair(key, coreString));
				}
				
				if (pos+3 <= len && buffer[pos]==0 && buffer[pos+1]==0 && buffer[pos+2]==0x09)
				{
					AMFObjectAlreadyStart=false;
				}
			}
		}
	}


	void XtraRtmp::rtmpAMF0Dump(const AmfPacket & amfPacket)
	{
		#define ToRtmpAmf0Dump __func__
		Nana::say(Nana::HAPPY, ToRtmpAmf0Dump,"+++++++++++++++START+++++++++++++++");
		if (amfPacket.coreNumber[0] == 1)
		{
			Nana::say(Nana::HAPPY, ToRtmpAmf0Dump, "CORE NUMBER EXISTS");
		}

		if (!amfPacket.coreString.empty())
		{
			Nana::say(Nana::HAPPY, ToRtmpAmf0Dump, "CORE STRING:%s", amfPacket.coreString.c_str());	
		}

		if (amfPacket.coreBoolean != -1)
		{
			Nana::say(Nana::HAPPY, ToRtmpAmf0Dump, "CORE BOOLEAN:%x", amfPacket.coreBoolean);
		}
		if( !amfPacket.coreObjectOfString.empty() )
		{
			std::map<std::string, std::string>::const_iterator coo_iter = amfPacket.coreObjectOfString.begin();
			Nana::say(Nana::HAPPY, ToRtmpAmf0Dump, "CORE OBJECT EXISTS,THAT THE VALUE TYPE IS CORE STRING");
			while (coo_iter != amfPacket.coreObjectOfString.end())
			{
				Nana::say(Nana::HAPPY, ToRtmpAmf0Dump, "KEY:%s", coo_iter->first.c_str());
				Nana::say(Nana::HAPPY, ToRtmpAmf0Dump, "VALUE:%s", coo_iter->second.c_str());
				++coo_iter;
			}
		}
		Nana::say(Nana::HAPPY, ToRtmpAmf0Dump,"+++++++++++++++DONE+++++++++++++++");
		
	}



	bool XtraRtmp::varifyRtmpAMF0(AmfPacket & amfPacket, const std::string &app)
	{
		return amfPacket.coreObjectOfString["app"] == app;
	}



	void  consultCallback(struct ev_loop * mainEventLoop, struct ev_io * consultWatcher, int revents)
	{
		#define ToConsultCallback __func__
		if ( EV_ERROR & revents )
    		{
        		Nana::say(Nana::COMPLAIN, ToConsultCallback, "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
        		return;
    		}
		unsigned char consultRequest[1024];
		int readBytes=read(consultWatcher->fd, consultRequest, sizeof(consultRequest));
		if ( readBytes > 0 )
		{
			Nana::say( Nana::HAPPY, ToConsultCallback, "READ THE RTMP PACKET %d BYTES", readBytes);
			XtraRtmp::RtmpPacket rtmpPacket;
			int ret = XtraRtmp::parseRtmpPacket(consultRequest, static_cast<size_t>(readBytes), rtmpPacket);
			XtraRtmp::AmfPacket amfPacket;
			if (ret == OK )
			{
				Nana::say(Nana::HAPPY, ToConsultCallback, "RTMP HEADER SIZE:%u", rtmpPacket.rtmpPacketHeader.size );
				Nana::say(Nana::HAPPY, ToConsultCallback, "RTMP HEADER CHUNK STREAM ID:%u", rtmpPacket.rtmpPacketHeader.chunkStreamID);
				Nana::say(Nana::HAPPY, ToConsultCallback, "RTMP HEADER TIMESTAMP:%u", rtmpPacket.rtmpPacketHeader.timestamp);
				Nana::say(Nana::HAPPY, ToConsultCallback, "RTMP HEADER AMF SIZE:%u", rtmpPacket.rtmpPacketHeader.AMFSize);
				Nana::say(Nana::HAPPY, ToConsultCallback, "RTMP HEADER AMF TYPE:%u", rtmpPacket.rtmpPacketHeader.AMFType);
				Nana::say(Nana::HAPPY, ToConsultCallback, "RTMP HEADER STREAM ID:%u", rtmpPacket.rtmpPacketHeader.streamID);

				XtraRtmp::parseRtmpAMF0(rtmpPacket.rtmpPacketPayload,  rtmpPacket.rtmpPacketHeader.AMFSize, amfPacket);
				XtraRtmp::rtmpAMF0Dump(amfPacket);
				//the state machine
				switch (rtmpPacket.rtmpPacketHeader.AMFType)
				{
					case XtraRtmp::MESSAGE_CHANGE_CHUNK_SIZE:
						Nana::say(Nana::HAPPY, ToConsultCallback, "THIS IS THE RTMP MESSAGE OF CHANGING THE CHUNK SIZE FOR PACKETS");
						break;
					case XtraRtmp::MESSAGE_DROP_CHUNK:
						Nana::say(Nana::HAPPY, ToConsultCallback, "THIS IS THE RTMP MESSAGE OF DROPING THE CHUNK IDENTIFIED BY STREAM CHUNK ID");
						break;
					case XtraRtmp::MESSAGE_SEND_BOTH_READ:
						Nana::say(Nana::HAPPY, ToConsultCallback, "THIS IS THE RTMP MESSAGE OF SENDING EVERY X BYTES READ BY BOTH SIDES");
						break;
					case XtraRtmp::MESSAGE_PING:
						Nana::say(Nana::HAPPY, ToConsultCallback, "THIS IS THE RTMP MESSAGE OF PING,WHICH HAS SUBTYPES");
						break;
					case XtraRtmp::MESSAGE_SERVER_DOWNSTREAM:
						Nana::say(Nana::HAPPY, ToConsultCallback, "THIS IS THE RTMP MESSAGE OF THE SERVERS DOWNSTREAM BW");
						break;
					case XtraRtmp::MESSAGE_CLIENT_UPSTREAM:
						Nana::say(Nana::HAPPY, ToConsultCallback, "THIS IS THE RTMP MESSAGE OF THE CLIENTS UPSTREAM BW");
						break;
					case XtraRtmp::MESSAGE_AUDIO:
						Nana::say(Nana::HAPPY, ToConsultCallback, "THIS IS THE RTMP MESSAGE OF PACKET CONTAINING AUDIO");
						break;
					case XtraRtmp::MESSAGE_VIDEO:
						Nana::say(Nana::HAPPY, ToConsultCallback, "THIS IS THE RTMP MESSAGE OF PACKET CONTAINING VIDEO");
						break;
					case XtraRtmp::MESSAGE_INVOKE_WITHOUT_REPLY:
						Nana::say(Nana::HAPPY, ToConsultCallback, "THIS IS THE RTMP MESSAGE OF AN INVOKE WHICH DOES NOT EXPECT A REPLY");
						break;
					case XtraRtmp::MESSAGE_SUBTYPE:
						Nana::say(Nana::HAPPY, ToConsultCallback, "THIS IS THE RTMP MESSAGE OF SHARED OBJECT WHICH HAS SUBTYPE");
						break;
					case XtraRtmp::MESSAGE_INVOKE:
						Nana::say(Nana::HAPPY, ToConsultCallback, "THIS IS THE RTMP MESSAGE OF INVOKE WHICH LIKE REMOTING CALL, USED FOR STREAM ACTIONS TOO");
						XtraRtmp::handleRtmpInvokeMessage(amfPacket, consultWatcher->fd);
						break;
					default:
						Nana::say(Nana::HAPPY, ToConsultCallback, "THIS IS THE RTMP MESSAGE OF UNKNOWN");
						break;
					}
				}
				Nana::say(Nana::HAPPY, ToConsultCallback,"++++++++++RTMP INTERACTIVE SIGNALLING DONE++++++++++");
			}
			else
			{
				Nana::say(Nana::COMPLAIN, ToConsultCallback, "ERROR OCCURRED WHEN READ:%s", strerror(errno));
				Nana::say(Nana::HAPPY, ToConsultCallback,"++++++++++RTMP INTERACTIVE SIGNALLING FAILED++++++++++");
			}
		DO_EVENT_CB_CLEAN(mainEventLoop, consultWatcher);
	}



	void XtraRtmp::handleRtmpInvokeMessage(AmfPacket &amfPacket, int connFd)
	{
		#define ToHandleRtmpInvokeMessage __func__
		(void)connFd;
		//just simply varify the app field
		bool varifyAmfOk = false;
		varifyAmfOk = varifyRtmpAMF0(amfPacket, APP);
		if ( varifyAmfOk )
		{
			Nana::say(Nana::HAPPY, ToHandleRtmpInvokeMessage, "VARIFY AMF0 APP FIELD OK: APP IS %s", APP.c_str());
			if (amfPacket.coreString == AmfCommand::connect)
			{
				
			}
			else if(amfPacket.coreString == AmfCommand::createStream)
			{
			}
			else if(amfPacket.coreString == AmfCommand::play)
			{
				
			}
			else if(amfPacket.coreString == AmfCommand::play2)
			{
				
			}
			else if(amfPacket.coreString == AmfCommand::deleteStream)
			{
				
			}
			else if(amfPacket.coreString == AmfCommand::receiveAudio)
			{
				
			}
			else if(amfPacket.coreString == AmfCommand::receiveVideo)
			{
				
			}
			else if(amfPacket.coreString == AmfCommand::publish)
			{
				
			}
			else if(amfPacket.coreString == AmfCommand::seek)
			{
				
			}
			else if(amfPacket.coreString == AmfCommand::pause)
			{
				
			}
		}
		else
		{
			Nana::say(Nana::HAPPY, ToHandleRtmpInvokeMessage, "VARIFY AMF0 APP FIELD FAILED:INVALID APP:%s THE CORRECT APP IS:%s", 
																	   amfPacket.coreObjectOfString["app"].empty() ? "empty": amfPacket.coreObjectOfString["app"].c_str(), APP.c_str());
		}
	}
};
