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
        	static const char *releaseStream = "releaseStream";
        	static const char *deleteStream = "deleteStream";
        	static const char *FCPublish = "FCPublish";
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
    			Nana::say(Nana::HAPPY, "serveForever",  "LISTEN SOCKET FD:%d", listenFd_);
    
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
		#define ToAcceptCallback "acceptCallback"

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
		#define ToShakeHandCallback "shakeHandCallback"
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

	int XtraRtmp::parseRtmpPacket(unsigned char *rtmpRequest, size_t len, std::vector<RtmpPacket> & rtmpPacketPool)
	{
		#define ToParseRtmpPacket "parseRtmpPacket"
		Nana::say(Nana::HAPPY, ToParseRtmpPacket, "++++++++++START++++++++++");
		if ( len == 0 )
		{
			Nana::say(Nana::HAPPY, ToParseRtmpPacket, "++++++++++DONE++++++++++");
			return ARGUMENT_ERROR;
		}

		size_t pos = 0;
		while (pos < len)
		{
			RtmpPacket rtmpPacket;
			memset(&rtmpPacket.rtmpPacketHeader, 0, sizeof(rtmpPacket.rtmpPacketHeader));
			char format = (rtmpRequest[pos] & 0xc0) >> 6;
			rtmpPacket.rtmpPacketHeader.chunkStreamID = rtmpRequest[pos] & 0x3f;
			
			switch (format)
			{
				case 0:
					rtmpPacket.rtmpPacketHeader.size = 12;
					rtmpPacket.rtmpPacketHeader.timestamp = rtmpRequest[pos+1]*256+rtmpRequest[pos+2]*16+rtmpRequest[pos+3];
					rtmpPacket.rtmpPacketHeader.AMFSize = rtmpRequest[pos+4]*256+rtmpRequest[pos+5]*16+rtmpRequest[pos+6];
					rtmpPacket.rtmpPacketHeader.AMFType = rtmpRequest[pos+7];
					rtmpPacket.rtmpPacketHeader.streamID = rtmpRequest[pos+8]*65536+rtmpRequest[pos+9]*256
													+rtmpRequest[pos+10]*16+rtmpRequest[pos+11];
					break;
				case 1:
					rtmpPacket.rtmpPacketHeader.size = 8;
					rtmpPacket.rtmpPacketHeader.timestamp = rtmpRequest[pos+1]*256+rtmpRequest[pos+2]*16+rtmpRequest[pos+3];
					rtmpPacket.rtmpPacketHeader.AMFSize = rtmpRequest[pos+4]*256+rtmpRequest[pos+5]*16+rtmpRequest[pos+6];
					rtmpPacket.rtmpPacketHeader.AMFType = rtmpRequest[pos+7];
					rtmpPacket.rtmpPacketHeader.streamID=0;
					break;
				case 2:
					rtmpPacket.rtmpPacketHeader.size = 4;
					rtmpPacket.rtmpPacketHeader.timestamp = rtmpRequest[pos+1]*256+rtmpRequest[pos+2]*16+rtmpRequest[pos+3];
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

			Nana::say(Nana::HAPPY, ToParseRtmpPacket, "RTMP HEADER SIZE:%u, AMF SIZE:%u", rtmpPacket.rtmpPacketHeader.size, rtmpPacket.rtmpPacketHeader.AMFSize);
			pos += rtmpPacket.rtmpPacketHeader.size;
			if ( rtmpPacket.rtmpPacketHeader.AMFSize != 0 )
			{
				if (rtmpPacket.rtmpPacketHeader.AMFSize > MAX_PAYLOAD_SIZE)
				{
					return LENGTH_OVERFLOW;
				}
				memcpy(rtmpPacket.rtmpPacketPayload, rtmpRequest+pos, rtmpPacket.rtmpPacketHeader.AMFSize);
			}
			pos += rtmpPacket.rtmpPacketHeader.AMFSize;
			rtmpPacketPool.push_back(rtmpPacket);
		}
		
		Nana::say(Nana::HAPPY, ToParseRtmpPacket, "++++++++++DONE++++++++++");
		return OK;
	}

	void XtraRtmp::parseRtmpAMF0(unsigned char *buffer, size_t len, AmfPacket & amfPacket)
	{
		#define ToParseRtmpAMF0 "parseRtmpAMF0"
		if (buffer == NULL || len == 0)
		{
			return;
		}

		amfPacket.command = "";
		amfPacket.transactionID[0]=0;
		amfPacket.flag[0]=0;
		amfPacket.streamName="";
		amfPacket.publishType = "";
		amfPacket.streamIDOrMilliSeconds = -1;
		
		size_t pos = 0;
		bool AMFObjectAlreadyStart = false;
		std::string coreString;
		std::string key,value;
		size_t stringLen,keyLen;
		int countString = 0;
		int countNumber = 0;
		Nana::say(Nana::HAPPY, ToParseRtmpAMF0,"+++++++++++++++START+++++++++++++++");
		while (pos < len)
		{
			if ( !AMFObjectAlreadyStart )
			{
				switch (buffer[pos])
				{
					case XtraRtmp::TYPE_CORE_NUMBER:
						if (countNumber == 0)
						{
							countNumber += 1;
							amfPacket.transactionID[0]=1;
							memcpy(amfPacket.transactionID+1, (char *)buffer+pos+1, LEN_CORE_NUMBER);
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
						if (countString == 0)
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
		Nana::say(Nana::HAPPY, ToParseRtmpAMF0,"+++++++++++++++DONE+++++++++++++++");
	}

	void XtraRtmp::rtmpAMF0Dump(const AmfPacket & amfPacket)
	{
		#define ToRtmpAmf0Dump "rtmpAMF0Dump"
		Nana::say(Nana::HAPPY, ToRtmpAmf0Dump,"+++++++++++++++START+++++++++++++++");
		
		if (!amfPacket.command.empty())
		{
					Nana::say(Nana::HAPPY, ToRtmpAmf0Dump, "COMMAND NAME:%s", amfPacket.command.c_str()); 
		}
				
		if (amfPacket.transactionID[0] == 1)
		{
			Nana::say(Nana::HAPPY, ToRtmpAmf0Dump, "TRANSACTION ID:%0x %0x %0x %0x %0x %0x %0x %0x",
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
			Nana::say(Nana::HAPPY, ToRtmpAmf0Dump, "FLAG:%x", amfPacket.flag[1]);
		}

		if (!amfPacket.streamName.empty())
		{
			Nana::say(Nana::HAPPY, ToRtmpAmf0Dump, "STREAM NAME:%s", amfPacket.streamName.c_str()); 
		}

		if (!amfPacket.publishType.empty())
		{
			Nana::say(Nana::HAPPY, ToRtmpAmf0Dump, "PUBLISH TYPE:%s", amfPacket.publishType.c_str()); 
		}
		
		if( !amfPacket.parameters.empty() )
		{
			std::map<std::string, std::string>::const_iterator coo_iter = amfPacket.parameters.begin();
			Nana::say(Nana::HAPPY, ToRtmpAmf0Dump, "CORE OBJECT EXISTS,THAT THE VALUE TYPE IS CORE STRING");
			while (coo_iter != amfPacket.parameters.end())
			{
				Nana::say(Nana::HAPPY, ToRtmpAmf0Dump, "KEY:%s", coo_iter->first.c_str());
				Nana::say(Nana::HAPPY, ToRtmpAmf0Dump, "VALUE:%s", coo_iter->second.c_str());
				++coo_iter;
			}
		}
		Nana::say(Nana::HAPPY, ToRtmpAmf0Dump,"+++++++++++++++DONE+++++++++++++++");
		
	}



	void  consultCallback(struct ev_loop * mainEventLoop, struct ev_io * consultWatcher, int revents)
	{
		#define ToConsultCallback "consultCallback"
		if ( EV_ERROR & revents )
    		{
        		Nana::say(Nana::COMPLAIN, ToConsultCallback, "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
        		return;
    		}
		unsigned char consultRequest[1024];
		int readBytes=read(consultWatcher->fd, consultRequest, sizeof(consultRequest));
		bool consultDone = false;
		bool done = false;
		if ( readBytes > 0 )
		{
			Nana::say( Nana::HAPPY, ToConsultCallback, "READ THE RTMP PACKET %d BYTES", readBytes);
			std::vector<XtraRtmp::RtmpPacket> rtmpPacketPool;
			int ret = XtraRtmp::parseRtmpPacket(consultRequest, static_cast<size_t>(readBytes), rtmpPacketPool);
			XtraRtmp::AmfPacket amfPacket;
			if (ret == OK )
			{
				Nana::say(Nana::HAPPY, ToConsultCallback, "THERE IS(ARE) %u RTMP PACKET(S)", rtmpPacketPool.size());

				std::vector<XtraRtmp::RtmpPacket>::iterator rpIter = rtmpPacketPool.begin();
				while (rpIter != rtmpPacketPool.end())
				{
					Nana::say(Nana::HAPPY, ToConsultCallback, "RTMP HEADER SIZE:%u", rpIter->rtmpPacketHeader.size );
					Nana::say(Nana::HAPPY, ToConsultCallback, "RTMP HEADER CHUNK STREAM ID:%u", rpIter->rtmpPacketHeader.chunkStreamID);
					Nana::say(Nana::HAPPY, ToConsultCallback, "RTMP HEADER TIMESTAMP:%u", rpIter->rtmpPacketHeader.timestamp);
					Nana::say(Nana::HAPPY, ToConsultCallback, "RTMP HEADER AMF SIZE:%u", rpIter->rtmpPacketHeader.AMFSize);
					Nana::say(Nana::HAPPY, ToConsultCallback, "RTMP HEADER AMF TYPE:%u", rpIter->rtmpPacketHeader.AMFType);
					Nana::say(Nana::HAPPY, ToConsultCallback, "RTMP HEADER STREAM ID:%u", rpIter->rtmpPacketHeader.streamID);

					XtraRtmp::parseRtmpAMF0(rpIter->rtmpPacketPayload, rpIter->rtmpPacketHeader.AMFSize, amfPacket);
					XtraRtmp::rtmpAMF0Dump(amfPacket);
					//the state machine
					switch (rpIter->rtmpPacketHeader.AMFType)
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
							done = XtraRtmp::onRtmpInvoke(rpIter->rtmpPacketHeader, amfPacket, consultWatcher->fd);
							if (done)
							{
								consultDone = done;
							}
							break;
						default:
							Nana::say(Nana::HAPPY, ToConsultCallback, "THIS IS THE RTMP MESSAGE OF UNKNOWN");
							break;
					}
					++rpIter;
				}
				Nana::say(Nana::HAPPY, ToConsultCallback,"++++++++++RTMP INTERACTIVE SIGNALLING DONE++++++++++");
			}
			else
			{
				Nana::say(Nana::COMPLAIN, ToConsultCallback, "ERROR OCCURRED WHEN READ:%s", strerror(errno));
				Nana::say(Nana::HAPPY, ToConsultCallback,"++++++++++RTMP INTERACTIVE SIGNALLING FAILED++++++++++");
				DO_EVENT_CB_CLEAN(mainEventLoop, consultWatcher);
			}
			
		}
		
		if (consultDone)
		{
			DO_EVENT_CB_CLEAN(mainEventLoop, consultWatcher);
		}
	}

	bool XtraRtmp::onRtmpInvoke(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd)
	{
		#define onRtmpInvoke "onRtmpInvoke"
		//just simply varify the app field
		bool varifyAmfOk = false;
		varifyAmfOk = true;
		if ( varifyAmfOk )
		{
			std::string peerInfo = NetUtil::getPeerInfo(connFd);
			if (amfPacket.command == AmfCommand::connect)
			{
				Nana::say(Nana::HAPPY, onRtmpInvoke, "RECEIVE A AMF connect COMMAND RELATED TO ID:%s", peerInfo.c_str());
				onConnect(rtmpPacketHeader, amfPacket, connFd);
			}
			else if (amfPacket.command == AmfCommand::createStream)
			{
				Nana::say(Nana::HAPPY, onRtmpInvoke, "RECEIVE THE AMF createStream COMMAND RELATED TO ID:%s", peerInfo.c_str());
			}
			else if (amfPacket.command == AmfCommand::releaseStream)
			{
				Nana::say(Nana::HAPPY, onRtmpInvoke, "RECEIVE THE AMF receiveStream COMMAND RELATED TO ID:%s", peerInfo.c_str());
			}
			else if (amfPacket.command == AmfCommand::deleteStream)
			{
				
			}
			else if (amfPacket.command == AmfCommand::play)
			{
				
			}
			else if (amfPacket.command == AmfCommand::play2)
			{
				
			}
			else if (amfPacket.command == AmfCommand::FCPublish)
			{
				Nana::say(Nana::HAPPY, onRtmpInvoke, "RECEIVE THE AMF FCPublish COMMAND");
			}
			else if (amfPacket.command == AmfCommand::receiveAudio)
			{
				
			}
			else if (amfPacket.command == AmfCommand::receiveVideo)
			{
				
			}
			else if (amfPacket.command == AmfCommand::publish)
			{
				
			}
			else if (amfPacket.command == AmfCommand::seek)
			{
				
			}
			else if (amfPacket.command == AmfCommand::pause)
			{
				
			}
		}
		else
		{
			;
		}
		return false;
	}



	void XtraRtmp::onConnect(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd)
	{
		#define ToOnConnect "onConnect"
		
		if (rtmpPacketHeader.size  < 8)
		return;
		
		const char * Connect[4][2]={ 
									{"_result",0},
									{"level", "status"},
									{"code", "NetConnection.Connect.Success"},
									{"description", "Connection succeeded."}
								};
								  
		char onConnectReply[250];
		int AMFSize = generateReply(onConnectReply, (char *)amfPacket.transactionID+1, Connect, 4);

		
		char * rtmpPack = new char[rtmpPacketHeader.size+AMFSize];
		memset(rtmpPack, 0, rtmpPacketHeader.size+AMFSize);
		rtmpPack[0] =rtmpPacketHeader.chunkStreamID;
		//set the rtmp packet's size
		rtmpPack[4] = (AMFSize & 0xff0000) >> 16;
		rtmpPack[5] = (AMFSize & 0xff00 ) >> 8;
		rtmpPack[6] = AMFSize & 0xff;

		AMFSize = rtmpPack[4]*256+rtmpPack[5]*16+rtmpPack[6];
		rtmpPack[7] = XtraRtmp::MESSAGE_INVOKE;

		if (rtmpPacketHeader.size == 12)
		{
			int streamID = htonl(rtmpPacketHeader.streamID);
			memcpy(rtmpPack+8, &streamID, 4);
		}
		
		memcpy(rtmpPack+rtmpPacketHeader.size, onConnectReply, AMFSize);
		int wroteBytes = write(connFd, rtmpPack, rtmpPacketHeader.size+AMFSize);
		if (wroteBytes != rtmpPacketHeader.size+AMFSize)
		{
			Nana::say(Nana::COMPLAIN, ToOnConnect, "WRITE ERROR:%s", strerror(errno));
		}
		delete [] rtmpPack;
	}



	int XtraRtmp::generateReply(char *reply, char *coreNumber, 
											  const char * commandObject[][2], int rows)
	{
		//the core string for reply
		int row = 0;
		reply[row]=XtraRtmp::TYPE_CORE_STRING;
		int AMFSize = 1;
		short len = strlen(commandObject[0][0]);
		reply[AMFSize] = (len & 0xff00) >> 8;
		reply[AMFSize+1] = len & 0xff;
		AMFSize += 2;
		memcpy(reply+AMFSize, commandObject[0][0], len);
		AMFSize += len;

		//the core number for reply
		if (coreNumber != 0)
		{
			reply[AMFSize]=XtraRtmp::TYPE_CORE_NUMBER;
			AMFSize += 1;
			memcpy(reply+AMFSize, coreNumber, 8);
			AMFSize += 8;
		}

		//the core object for reply
		reply[AMFSize]=XtraRtmp::TYPE_CORE_OBJECT;
		AMFSize += 1;

		
		for (; row < rows; ++row)
		{
			if (commandObject[row][1] == 0 )
			continue;
			
			len = strlen(commandObject[row][0]);
			reply[AMFSize] = (len & 0xff00) >> 8;
			reply[AMFSize+1] = len & 0xff;
			AMFSize += 2;
			memcpy(reply+AMFSize, commandObject[row][0], len);
			AMFSize += len;
			reply[AMFSize]=XtraRtmp::TYPE_CORE_STRING;
			AMFSize += 1;
			len = strlen(commandObject[row][1]);
			reply[AMFSize] = (len & 0xff00) >> 8;
			reply[AMFSize+1] = len & 0xff;
			AMFSize += 2;
			memcpy(reply+AMFSize, commandObject[row][1], len);
			AMFSize += len;
		}

		//the end of core object
		reply[AMFSize]=0;
		reply[AMFSize+1]=0;
		reply[AMFSize+2]=9;
		AMFSize += 3;
		return AMFSize;
	}
};
