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
#include<stdint.h>



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
		static const char *_checkbw = "_checkbw";	
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
		std::cout<<serverConfig_.meta["version"]<<std::endl<<std::endl;
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
    		int connFd = accept( listenWatcher->fd, (struct sockaddr *)&clientAddr, &len );
    		if ( connFd < 0 )
    		{
        		Nana::say(Nana::COMPLAIN, ToAcceptCallback, "ACCEPT ERROR:%s", strerror(errno));   
        		return;
    		}
    		
    		struct ev_io * receiveRequestWatcher = new struct ev_io;
    		if ( receiveRequestWatcher == NULL )
    		{
        		Nana::say(Nana::COMPLAIN, ToAcceptCallback, "ALLOCATE MEMORY FAILED:%s", strerror( errno ));
        		close(connFd);
        		return;
    		}
    
    		receiveRequestWatcher->active = 0;
    		receiveRequestWatcher->data = 0;   
    		if ( Nana::is(Nana::PEACE))
    		{
        		std::string peerInfo = NetUtil::getPeerInfo(connFd);
        		Nana::say(Nana::PEACE, ToAcceptCallback, "CLIENT %s CONNECTED SOCK FD IS:%d",
                                                             peerInfo.c_str(), connFd);
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
		ssize_t readBytes=readv(receiveRequestWatcher->fd, iovRead, 2);
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
			ssize_t wrotenBytes=writev(receiveRequestWatcher->fd,iovWrite,2);
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
		#define ToParseRtmpPacket __func__
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
		#define ToRtmpAmf0Dump __func__
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
		#define ToConsultCallback __func__
		if ( EV_ERROR & revents )
    		{
        		Nana::say(Nana::COMPLAIN, ToConsultCallback, "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
        		close(consultWatcher->fd);	
			DO_EVENT_CB_CLEAN(mainEventLoop, consultWatcher);
    		}
		
		unsigned char consultRequest[1024];
		ssize_t readBytes=read(consultWatcher->fd, consultRequest, sizeof(consultRequest));
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
					XtraRtmp::rtmpMessageDump((XtraRtmp::RtmpMessageType)rpIter->rtmpPacketHeader.AMFType);
					//the state machine
					switch (rpIter->rtmpPacketHeader.AMFType)
					{
						case XtraRtmp::MESSAGE_CHANGE_CHUNK_SIZE:
							break;
						case XtraRtmp::MESSAGE_DROP_CHUNK:
							break;
						case XtraRtmp::MESSAGE_SEND_BOTH_READ:
							break;
						case XtraRtmp::MESSAGE_USER_CONTROL:
							break;
						case XtraRtmp::MESSAGE_WINDOW_ACKNOWLEDGEMENT_SIZE:
							break;
						case XtraRtmp::MESSAGE_SET_PEER_BANDWIDTH:
							break;
						case XtraRtmp::MESSAGE_AUDIO:
							break;
						case XtraRtmp::MESSAGE_VIDEO:
							break;
						case XtraRtmp::MESSAGE_AMF0_DATA:
							break;
						case XtraRtmp::MESSAGE_SUBTYPE:
							break;
						case XtraRtmp::MESSAGE_INVOKE:
							done = XtraRtmp::onRtmpInvoke(rpIter->rtmpPacketHeader, amfPacket, consultWatcher->fd);
							if (done)
							{
								consultDone = done;
							}
							break;
						default:
							break;
					}
					++rpIter;
				}
				Nana::say(Nana::HAPPY, ToConsultCallback,"++++++++++RTMP INTERACTIVE SIGNALLING DONE++++++++++");
			}
			else
			{
				Nana::say(Nana::COMPLAIN, ToConsultCallback, "ERROR OCCURRED WHEN READ:%s", strerror(errno));
				close(consultWatcher->fd);
				Nana::say(Nana::HAPPY, ToConsultCallback,"++++++++++RTMP INTERACTIVE SIGNALLING FAILED++++++++++");
				DO_EVENT_CB_CLEAN(mainEventLoop, consultWatcher);
			}
			
		}
		
		if (consultDone)
		{
			Nana::say(Nana::HAPPY, ToConsultCallback, "RTMP CONSULT DONE,STARTING RECEIVE THE RTMP STREAM");
			struct ev_io * receiveStreamWatcher = new struct ev_io;
    			if ( receiveStreamWatcher == NULL )
    			{
        			Nana::say(Nana::COMPLAIN, ToConsultCallback, "ALLOCATE MEMORY FAILED:%s", strerror( errno ));
    			}
    			else
    			{
    				receiveStreamWatcher->active = 0;
    				receiveStreamWatcher->data = 0;   
    				ev_io_init(  receiveStreamWatcher, receiveStreamCallback, consultWatcher->fd, EV_READ );
    				ev_io_start( mainEventLoop, receiveStreamWatcher);
    			}		
			DO_EVENT_CB_CLEAN(mainEventLoop, consultWatcher);
		}
	}



	void XtraRtmp::rtmpMessageDump(const RtmpMessageType & rtmpMessageType)
	{
		#define ToRtmpMessageDump __func__
		switch (rtmpMessageType)
		{
			case XtraRtmp::MESSAGE_CHANGE_CHUNK_SIZE:
				Nana::say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF CHANGING THE CHUNK SIZE FOR PACKETS");
				break;
			case XtraRtmp::MESSAGE_DROP_CHUNK:
				Nana::say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF DROPING THE CHUNK IDENTIFIED BY STREAM CHUNK ID");
				break;
			case XtraRtmp::MESSAGE_SEND_BOTH_READ:
				Nana::say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF SENDING EVERY X BYTES READ BY BOTH SIDES");
				break;
			case XtraRtmp::MESSAGE_USER_CONTROL:
				Nana::say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF PING,WHICH HAS SUBTYPES");
				break;
			case XtraRtmp::MESSAGE_WINDOW_ACKNOWLEDGEMENT_SIZE:
				Nana::say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF THE SERVERS DOWNSTREAM BW");
				break;
			case XtraRtmp::MESSAGE_SET_PEER_BANDWIDTH:
				Nana::say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF THE CLIENTS UPSTREAM BW");
				break;
			case XtraRtmp::MESSAGE_AUDIO:
				Nana::say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF PACKET CONTAINING AUDIO");
				break;
			case XtraRtmp::MESSAGE_VIDEO:
				Nana::say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF PACKET CONTAINING VIDEO");
				break;
			case XtraRtmp::MESSAGE_AMF0_DATA:
				Nana::say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF AMF0 DATA");
				break;
			case XtraRtmp::MESSAGE_SUBTYPE:
				Nana::say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF SHARED OBJECT WHICH HAS SUBTYPE");
				break;
			case XtraRtmp::MESSAGE_INVOKE:
				Nana::say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF INVOKE");
				break;
			default:
				Nana::say(Nana::HAPPY, ToRtmpMessageDump, "THIS IS THE RTMP MESSAGE OF UNKNOWN %x",rtmpMessageType);
				break;
		}
	}


	
	void  receiveStreamCallback(struct ev_loop * mainEventLoop, struct ev_io * receiveStreamWatcher, int revents)
	{
		#define ToReceiveStreamCallback __func__

		Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "++++++++++++++++++++START++++++++++++++++++++");
		if ( EV_ERROR & revents )
    		{
        		Nana::say(Nana::COMPLAIN, ToReceiveStreamCallback, "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
			close(receiveStreamWatcher->fd);
			Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "++++++++++++++++++++DONE++++++++++++++++++++");
			DO_EVENT_CB_CLEAN(mainEventLoop, receiveStreamWatcher);
    		}

		static bool alreadyReadAvcSequenceHeader = false;
		static bool alreadyReadAacSequenceHeader = false;
		static bool AMFDataSizeGT128 = false;
		static size_t LEFT_READ_SIZE = 0;
		static size_t previousAMFDataSize = 0;
		
		unsigned char chunkBasicHeader;
		size_t totalBytes = 0;
		ssize_t readBytes=read(receiveStreamWatcher->fd, &chunkBasicHeader, 1);
		if ( readBytes <= 0 )
		{
			close(receiveStreamWatcher->fd);
			Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "++++++++++++++++++++DONE++++++++++++++++++++");
			DO_EVENT_CB_CLEAN(mainEventLoop, receiveStreamWatcher);
		}
		static size_t TOTAL_READ_BYTES = 0;
		Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "CHUNK BASIC HEADER:%x", chunkBasicHeader);
		unsigned char format = static_cast<unsigned char>((chunkBasicHeader & 0xc0) >> 6);
		unsigned char channelID = static_cast<unsigned char>(chunkBasicHeader & 0x3f);
		unsigned char channelID2[2];
		switch( channelID )
		{
			case 0:
				//just read one byte more
				Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "CHANNEL ID IS 0,JUST READ ONE MORE BYTE");
				read(receiveStreamWatcher->fd, &channelID, 1);
				channelID = static_cast<unsigned char>(channelID +64);
				break;
			case 1:	
				Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "CHANNEL ID IS 1,JUST READ TWO MORE BYTES");
				read(receiveStreamWatcher->fd, channelID2, 2);
				//channelID = channelID2[0]*256+ channelID2[1]*64;
				break;
			case 2:
			case 3:
			case 4:	
				Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "CHANNEL ID IS [234],CONTROL MESSAGE");
				break;	
			default:
				Nana::say(Nana::COMPLAIN, ToReceiveStreamCallback, "RECEIVE THE COMPLETE CHANNEL ID:%d", channelID);
				break;	
		}	

		
		Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "CHUNK BASIC HEADER PARSED.FORMAT:%x CHANNEL ID:%x", format, channelID);
		unsigned char chunkMsgHeader[11];
		memset(chunkMsgHeader, 0, 11);
		size_t chunkMsgHeaderSize;
		switch ( format )
		{
			case 0:
				chunkMsgHeaderSize = 11;
				break;
			case 1:
				chunkMsgHeaderSize = 7;
				break;
			case 2:
				chunkMsgHeaderSize = 3;
				break;
			case 3:
				chunkMsgHeaderSize = 0;
				Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "RECEIVE THE NONE KEY FRAME,JUST RETRIEVE 128 BYTES");
				break;
		}

		switch (channelID)
		{
			case XtraRtmp::CHANNEL_PING_BYTEREAD:
				Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "WELCOME TO THE CHANNEL OF PING & BYTE READ");
				break;
			case XtraRtmp::CHANNEL_INVOKE:
				Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "WELCOME TO THE CHANNEL OF INVOKE");
				break;
			case XtraRtmp::CHANNEL_AUDIO_VIDEO:
				Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "WELCOME TO THE CHANNEL OF AUDIO & VIDEO");
				break;
		}

		
		Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "CHUNK MESSAGE HEADER SIZE:%u", chunkMsgHeaderSize);
		if ( chunkMsgHeaderSize > 0 )
		{
			while( totalBytes != chunkMsgHeaderSize )
			{
				readBytes=read(receiveStreamWatcher->fd, chunkMsgHeader+totalBytes, 
												     chunkMsgHeaderSize-totalBytes);
				if ( readBytes <= 0 )
				{
					Nana::say(Nana::COMPLAIN, ToReceiveStreamCallback, "SOCKET READ ERROR:%s", strerror(errno));
					close(receiveStreamWatcher->fd);
					Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "++++++++++++++++++++DONE++++++++++++++++++++");
					DO_EVENT_CB_CLEAN(mainEventLoop,receiveStreamWatcher);
				}
				totalBytes +=static_cast<size_t>(readBytes);
			}
			Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "READ CHUNK MESSAGE HEADER DONE");
		}

		if ( chunkMsgHeaderSize >= 7 )
		{
			int timestamp = chunkMsgHeader[0]*65536+chunkMsgHeader[1]*256+chunkMsgHeader[2];
			size_t AMFSize = static_cast<size_t>(chunkMsgHeader[3]*65536+chunkMsgHeader[4]*256+chunkMsgHeader[5]);
			if ( AMFSize == 0 )
			{
				Nana::say(Nana::COMPLAIN, ToReceiveStreamCallback, "RTMP PACKET ERROR");
				exit(EXIT_FAILURE);
			}
			
			Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "TIMESTAMP:%d AMF SIZE:%d HEX:%x %x %x DECIMAL:%d %d %d", 
						timestamp, AMFSize,  chunkMsgHeader[3], chunkMsgHeader[4], chunkMsgHeader[5],
						chunkMsgHeader[3],chunkMsgHeader[4],chunkMsgHeader[5]);
			unsigned char msgType = chunkMsgHeader[6];
			if (msgType == XtraRtmp::MESSAGE_AMF0_DATA)
			{
				AMFSize += 3;
			}
			else if ( msgType ==  XtraRtmp::MESSAGE_VIDEO || msgType == XtraRtmp::MESSAGE_AUDIO )
			{
				if ( AMFSize > 128 )
				{
					previousAMFDataSize = AMFSize;
					LEFT_READ_SIZE = AMFSize - 128;
					AMFSize = 128;
					TOTAL_READ_BYTES += 128;
					AMFDataSizeGT128 = true;
				}
				
				if ( msgType ==  XtraRtmp::MESSAGE_VIDEO )
				{
					
					if (!alreadyReadAvcSequenceHeader)
					{
						alreadyReadAvcSequenceHeader = true;
						Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "RECEIVE THE AVC SEQUENCE HEADER");
					}
					else
					{
						Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "RECEIVE THE KEY FRAME & THE KEY FRAME'S SIZE IS %d", AMFSize);
						
					}	
				}
				else
				{
					if (!alreadyReadAacSequenceHeader)
					{
						alreadyReadAacSequenceHeader = true;
						Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "RECEIVE THE AAC SEQUENCE HEADER");
					}
					else
					{
						Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "RECEIVE THE RTMP AUDIO DATA");
					}
				}
				
			}
			else
			{
				Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "RECEIVE THE UNKNOWN RTMP  DATA");
			}

			
			XtraRtmp::rtmpMessageDump((XtraRtmp::RtmpMessageType)msgType);
			uint8_t * AmfData = new uint8_t[AMFSize];
			memset(AmfData, 0, AMFSize);
			totalBytes = 0;

			while ( totalBytes != AMFSize )
			{
				readBytes=read(receiveStreamWatcher->fd, AmfData+totalBytes, AMFSize-totalBytes);
				if ( readBytes <= 0 )
				{
					Nana::say(Nana::COMPLAIN, ToReceiveStreamCallback, "READ ERROR OCCURRED:%s", strerror(errno));
					close(receiveStreamWatcher->fd);
					Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "++++++++++++++++++++DONE++++++++++++++++++++");
					DO_EVENT_CB_CLEAN(mainEventLoop,receiveStreamWatcher);
				}
				totalBytes += static_cast<size_t>(readBytes);
			}

			Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "READ AMF DATA DONE:%d BYTES", totalBytes);
			Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "++++++++++++++++++++DONE++++++++++++++++++++");
			delete [] AmfData;
			AmfData = 0;
			return;
		}
		else if ( format == 2 )
		{
			Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "THIS AMF DATA SIZE IS EQUAL TO PREVIOUS DATA SIZE:%u", previousAMFDataSize);
			LEFT_READ_SIZE = previousAMFDataSize;
			previousAMFDataSize = 0;
			AMFDataSizeGT128 = true;
		}

		size_t bufferSize = 0;
		if ( LEFT_READ_SIZE >= 128 )
		{
			bufferSize = 128;
			LEFT_READ_SIZE -=128;
		}
		else
		{
			bufferSize = LEFT_READ_SIZE;
			LEFT_READ_SIZE = 0;
		}

		if ( ! AMFDataSizeGT128 )
		{
			bufferSize = 128;
		}

		TOTAL_READ_BYTES += bufferSize;
		char rtmpStream[128];
		Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "READ AMF DATA  SIZE %u BYTES START", bufferSize);
		totalBytes = 0;
		while ( totalBytes != bufferSize )
		{
			readBytes=read(receiveStreamWatcher->fd, rtmpStream+totalBytes, bufferSize-totalBytes);
			if ( readBytes <= 0 )
			{
				Nana::say(Nana::COMPLAIN, ToReceiveStreamCallback, "READ ERROR OCCURRED:%s", strerror(errno));
				close(receiveStreamWatcher->fd);
				Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "++++++++++++++++++++DONE++++++++++++++++++++");
				DO_EVENT_CB_CLEAN(mainEventLoop,receiveStreamWatcher);
			}
			totalBytes += static_cast<size_t>(readBytes);
		}

		if ( LEFT_READ_SIZE == 0 )
		{
			AMFDataSizeGT128 = false;
		}
		
		Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "READ AMF DATA  SIZE %u BYTES DONE, TOTAL READ BYTES:%u",
													bufferSize, TOTAL_READ_BYTES);

		if (LEFT_READ_SIZE == 0 || !AMFDataSizeGT128 )
		{
			TOTAL_READ_BYTES = 0;
		}
		Nana::say(Nana::HAPPY, ToReceiveStreamCallback, "++++++++++++++++++++DONE++++++++++++++++++++");
	}


	
	bool XtraRtmp::onRtmpInvoke(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd)
	{
		#define onRtmpInvoke __func__
		(void)connFd;
		//just simply varify the app field
		bool varifyAmfOk = false;
		varifyAmfOk = true;
		bool done = false;
		
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
				onCreateStream(rtmpPacketHeader, amfPacket, connFd);
			}
			else if (amfPacket.command == AmfCommand::releaseStream)
			{
				Nana::say(Nana::HAPPY, onRtmpInvoke, "RECEIVE THE AMF releaseStream COMMAND RELATED TO ID:%s", peerInfo.c_str());
				onReleaseStream(rtmpPacketHeader, amfPacket, connFd);			
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
				onFCPublish(rtmpPacketHeader, amfPacket, connFd);
			}
			else if (amfPacket.command == AmfCommand::receiveAudio)
			{
				
			}
			else if (amfPacket.command == AmfCommand::receiveVideo)
			{
				
			}
			else if (amfPacket.command == AmfCommand::publish)
			{
				Nana::say(Nana::HAPPY, onRtmpInvoke, "RECEIVE THE AMF publish COMMAND");
				onPublish(rtmpPacketHeader, amfPacket, connFd);
				done = true;
			}
			else if (amfPacket.command == AmfCommand::seek)
			{
				
			}
			else if (amfPacket.command == AmfCommand::pause)
			{
				
			}
			else if (amfPacket.command == AmfCommand::_checkbw)
			{
				Nana::say(Nana::HAPPY, onRtmpInvoke, "RECEIVE THE _checkbw COMMAND");
				onCheckbw(rtmpPacketHeader, amfPacket, connFd);
			}
		}
		else
		{
			;
		}
		return done;
	}



	void XtraRtmp::onConnect(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd)
	{
		(void) amfPacket;
		#define ToOnConnect __func__
		
		if (rtmpPacketHeader.size  < 8)
		return;
		
		const char * ConnectParameters[4][2]={ 
									{"_result",0},
									{"level", "status"},
									{"code", "NetConnection.Connect.Success"},
									{"description", "Connection succeeded."}
								};

		const char number[8]={(const char)0x40,(const char)0xc0,(const char)0x0,(const char)0x0,(const char)0x0,
							 (const char)0x0,(const char)0x0,(const char)0x00};
		
		const char *OnBWDone[3][2]={
									{"onBWDone",0},
									{0,0},
									{0,(const char *)number}
								};
		
		size_t windowAcknowledgementSize = 2500000;
		size_t peerBandwidth = 2500000;
		onRtmpReply(MESSAGE_WINDOW_ACKNOWLEDGEMENT_SIZE, connFd, windowAcknowledgementSize);
		onRtmpReply(MESSAGE_SET_PEER_BANDWIDTH, connFd, peerBandwidth);
		onRtmpReply(MESSAGE_USER_CONTROL, connFd);
		onRtmpReply(rtmpPacketHeader, amfPacket.transactionID+1, ConnectParameters, 4, connFd);
		rtmpPacketHeader.flag = 1;
		rtmpPacketHeader.size = 8;
		onRtmpReply(rtmpPacketHeader, 0, OnBWDone, 3, connFd);
		
	}


	
	void XtraRtmp::onCreateStream(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd)
	{
		#define ToOnCreateStream __func__
			
		if (rtmpPacketHeader.size  < 8)
		return;
			
		const char * CreateStreamParameters[2][2]={ 
													{"_result",0},
													{0, 0}
												};
		
		Nana::say(Nana::HAPPY, ToOnCreateStream, "+++++++++++++++START+++++++++++++++");
		rtmpPacketHeader.flag = 1;
		rtmpPacketHeader.chunkStreamID = 3;
		rtmpPacketHeader.size = 8;
		onRtmpReply(rtmpPacketHeader, amfPacket.transactionID+1, CreateStreamParameters, 2, connFd);
		Nana::say(Nana::HAPPY, ToOnCreateStream, "+++++++++++++++DONE+++++++++++++++");
									  
	}


	
	void XtraRtmp::onCheckbw(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd)
	{
		#define ToOnCheckbw __func__
				
		if (rtmpPacketHeader.size  < 8)
		return;
				
		const char * CheckBWS[5][2]={ 
									{"_error", 0},
									{0, 0},
									{"level", "error"},
									{"code", "NetConnection.Call.Failed"},
									{"description", "call to function _checkbw failed"}
								    };
			
		Nana::say(Nana::HAPPY, ToOnCheckbw, "+++++++++++++++START+++++++++++++++");

		rtmpPacketHeader.flag = 1;
		rtmpPacketHeader.chunkStreamID = 3;
		rtmpPacketHeader.size = 8;
		onRtmpReply(rtmpPacketHeader, amfPacket.transactionID+1, CheckBWS, 5, connFd);
		Nana::say(Nana::HAPPY, ToOnCheckbw, "+++++++++++++++DONE+++++++++++++++");
										  
	}


	
	void XtraRtmp::onReleaseStream(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd)
	{
		#define ToReleaseStream __func__
		Nana::say(Nana::HAPPY, ToReleaseStream, "+++++++++++++++START+++++++++++++++");
		const char * ReleaseStreamParametersS[5][2]={ 
													{"_error",0},
													{0, 0},	
													{"level", "error"},
													{"code", "NetConnection.Call.Failed"},
													{"description", "specified stream not found in call to releaseStream"}
									   			   };
		rtmpPacketHeader.flag = 1;
		rtmpPacketHeader.chunkStreamID = 3;
		rtmpPacketHeader.size = 8;
		onRtmpReply(rtmpPacketHeader, amfPacket.transactionID+1, ReleaseStreamParametersS, 5, connFd);
		Nana::say(Nana::HAPPY, ToReleaseStream, "+++++++++++++++DONE+++++++++++++++");
	}



	void XtraRtmp::onFCPublish(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd)
	{
		#define ToFCPublish __func__
			
		if (rtmpPacketHeader.size  < 8)
		return;
	
		Nana::say(Nana::HAPPY, ToFCPublish, "+++++++++++++++START+++++++++++++++");
		const char * FCPublishParametersS[3][2]={ 
												{"_result",0},
												{0, 0},
												{0, 0}
										   	};
			
		const char * FCPublishParametersC[4][2]={ 
												{"onFCPublish",0},
												{0, 0},
												{"code","NetStream.Publish.Start"},
												{"description",0}
										   	};
		rtmpPacketHeader.flag = 0;
		rtmpPacketHeader.chunkStreamID = 3;
		rtmpPacketHeader.size = 12;
		FCPublishParametersC[3][1]=amfPacket.streamName.c_str();
		onRtmpReply(rtmpPacketHeader, amfPacket.transactionID+1, FCPublishParametersS, 3, connFd);
		onRtmpReply(rtmpPacketHeader, 0, FCPublishParametersC, 4, connFd);
		Nana::say(Nana::HAPPY, ToFCPublish, "+++++++++++++++DONE+++++++++++++++");
	}


	
	void XtraRtmp::onPublish(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd)
	{
		#define ToOnPublish __func__
		(void)amfPacket;
		const char * PublishParametersS[5][2]={ 
													{"onStatus",0},
													{0, 0},
													{"level", "status"},
													{"code","NetStream.Publish.Start"},
													{"clientid","mlgb123456"}
												};

			
		Nana::say(Nana::HAPPY, ToOnPublish, "+++++++++++++++START+++++++++++++++");
		rtmpPacketHeader.flag = 0;
		//the audio video channel
		rtmpPacketHeader.chunkStreamID = 4;
		rtmpPacketHeader.size = 12;
		onRtmpReply(rtmpPacketHeader, 0, PublishParametersS, 5, connFd);
		Nana::say(Nana::HAPPY, ToOnPublish, "+++++++++++++++DONE+++++++++++++++");
										  
	}


	
	void XtraRtmp::onRtmpReply(RtmpPacketHeader &rtmpPacketHeader, unsigned char *transactionID, 
									const char *parameters[][2], int rows, int connFd)
	{
		#define ToOnRtmpReply __func__
		Nana::say(Nana::HAPPY, ToOnRtmpReply, "+++++++++++++++START+++++++++++++++");
		unsigned char reply[250];
		size_t AMFSize = generateReply(reply, transactionID, parameters, rows);
		unsigned char * rtmpPack = new unsigned char[rtmpPacketHeader.size+AMFSize];
		memset(rtmpPack, 0, rtmpPacketHeader.size+AMFSize);
		rtmpPack[0] =(unsigned char)(rtmpPacketHeader.flag << 6) |rtmpPacketHeader.chunkStreamID;
		//set the rtmp packet's size
		rtmpPack[4] = static_cast<unsigned char>((AMFSize & 0xff0000) >> 16);
		rtmpPack[5] = static_cast<unsigned char>((AMFSize & 0xff00 ) >> 8);
		rtmpPack[6] = static_cast<unsigned char>(AMFSize & 0xff);

		rtmpPack[7] = XtraRtmp::MESSAGE_INVOKE;

		if (rtmpPacketHeader.size == 12)
		{
			int streamID = htonl(rtmpPacketHeader.streamID);
			memcpy(rtmpPack+8, &streamID, 4); 
		}
		
		memcpy(rtmpPack+rtmpPacketHeader.size, reply, AMFSize);
		ssize_t wroteBytes = write(connFd, rtmpPack, rtmpPacketHeader.size+AMFSize);
		if (wroteBytes < 0 || (static_cast<size_t>(wroteBytes) != rtmpPacketHeader.size+AMFSize))
		{
			Nana::say(Nana::COMPLAIN, ToOnRtmpReply, "WRITE ERROR:%s", strerror(errno));
		}
		else
		{
			Nana::say(Nana::HAPPY, ToOnRtmpReply, "RTMP PACKET WRITTEN BYTES:%d", wroteBytes);
		}
		delete [] rtmpPack;
		Nana::say(Nana::HAPPY, ToOnRtmpReply, "+++++++++++++++DONE+++++++++++++++");
	}



	
	size_t XtraRtmp::generateReply(unsigned char *reply, unsigned char *transactionID, 
											  const char * parameters[][2], int rows)
	{
		//the core string for reply
		int row = 0;
		reply[row]=XtraRtmp::TYPE_CORE_STRING;
		size_t AMFSize = 1;
		size_t len = strlen(parameters[0][0]);
		reply[AMFSize] = static_cast<char>((len & 0xff00) >> 8);
		reply[AMFSize+1] = static_cast<char>(len & 0xff);
		AMFSize += 2;
		memcpy(reply+AMFSize, parameters[0][0], len);
		AMFSize += len;

		reply[AMFSize]=XtraRtmp::TYPE_CORE_NUMBER;
		AMFSize += 1;
		if (transactionID != 0)
		{
			memcpy(reply+AMFSize, transactionID, 8);
		}
		else
		{
			memset(reply+AMFSize, 0, 8);
		}

		AMFSize += 8;
		bool object = false;
		
		for(row=1; row < rows; ++row)
		{
			if (parameters[row][0] != 0 && parameters[row][1] != 0)
			{
				object = true;
				continue;
			}
			
			if (parameters[row][0] == 0 && parameters[row][1] == 0)
			{
				reply[AMFSize]=XtraRtmp::TYPE_CORE_NULL;
				AMFSize += 1;
			}
			else 
			{
				reply[AMFSize]=XtraRtmp::TYPE_CORE_NUMBER;
				AMFSize += 1;
				memcpy(reply+AMFSize, (unsigned char *)parameters[row][1], 8);
				AMFSize += 8;
			}
		}

		if ( object )
		{
				
			//the core object for reply
			reply[AMFSize]=XtraRtmp::TYPE_CORE_OBJECT;
			AMFSize += 1;
		}

		
		for (row=1; row < rows; ++row)
		{
			if (parameters[row][0] == 0 ||parameters[row][1] == 0)
			continue;
			
			len = strlen(parameters[row][0]);
			reply[AMFSize] = static_cast<char>((len & 0xff00) >> 8);
			reply[AMFSize+1] = static_cast<char>(len & 0xff);
			AMFSize += 2;
			memcpy(reply+AMFSize, parameters[row][0], len);
			AMFSize += len;
			reply[AMFSize]=XtraRtmp::TYPE_CORE_STRING;
			AMFSize += 1;
			len = strlen(parameters[row][1]);
			reply[AMFSize] = static_cast<char>((len & 0xff00) >> 8);
			reply[AMFSize+1] = static_cast<char>(len & 0xff);
			AMFSize += 2;
			memcpy(reply+AMFSize, parameters[row][1], len);
			AMFSize += len;
		}


		//the end of core object
		if ( object )
		{
			reply[AMFSize]=0;
			reply[AMFSize+1]=0;
			reply[AMFSize+2]=9;
			AMFSize += 3;
		}
		return AMFSize;
	}



	void XtraRtmp::onRtmpReply(const RtmpMessageType & rtmpMessageType, int connFD,  size_t size)
	{
		unsigned char reply[250];
		memset(reply, 0, sizeof(reply));
		int pos = 0;
		int AMFSize = 0;
		switch (rtmpMessageType)
		{
			case XtraRtmp::MESSAGE_WINDOW_ACKNOWLEDGEMENT_SIZE:
				AMFSize = 4;
				reply[pos]=2;
				pos += 4;
				reply[pos+2] = static_cast<unsigned char>(AMFSize);
				pos+=3;
				reply[pos]=XtraRtmp::MESSAGE_WINDOW_ACKNOWLEDGEMENT_SIZE;
				pos += 1;
				pos += 4;
				reply[pos] = static_cast<unsigned char>((size & 0xff000000) >> 24);
				reply[pos+1] = static_cast<unsigned char>((size & 0x00ff0000) >> 16);
				reply[pos+2] = static_cast<unsigned char>((size & 0x0000ff00) >> 8);
				reply[pos+3] = static_cast<unsigned char>(size & 0x000000ff);
				pos += 4;
				write(connFD, reply, pos);
			break;
			case XtraRtmp::MESSAGE_SET_PEER_BANDWIDTH:
				AMFSize = 5;
				reply[pos]=2;
				pos += 4;
				reply[pos+2] = static_cast<unsigned char>(AMFSize);
				pos+=3;
				reply[pos]=XtraRtmp::MESSAGE_SET_PEER_BANDWIDTH;
				pos += 1;
				pos += 4;
				reply[pos] = static_cast<unsigned char>((size & 0xff000000) >> 24);
				reply[pos+1] = static_cast<unsigned char>((size & 0x00ff0000) >> 16);
				reply[pos+2] = static_cast<unsigned char>((size & 0x0000ff00) >> 8);
				reply[pos+3] = static_cast<unsigned char>(size & 0x000000ff);
				pos += 4;
				reply[pos]=2;
				pos+=1;
				write(connFD, reply, pos);
			break;
			case XtraRtmp::MESSAGE_USER_CONTROL:
				AMFSize = 6;
				reply[pos]=2;
				pos += 4;
				reply[pos+2] = static_cast<unsigned char>(AMFSize);
				pos+=3;
				reply[pos]=XtraRtmp::MESSAGE_USER_CONTROL;
				pos += 1;
				pos += 4;
				pos += 6;
				write(connFD, reply, pos);
			break;
			default:
				break;
		}
	}
};
