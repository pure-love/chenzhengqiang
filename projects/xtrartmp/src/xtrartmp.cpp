/*
*@filename:xtrartmp.cpp
*@author:chenzhengqiang
*@start date:2015/11/26 11:26:16
*@modified date:
*@desc: 
*/

#include "common.h"
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

    		
    		struct sockaddr_in client_addr;
    		socklen_t len = sizeof( struct sockaddr_in );
    		ssize_t connFd = accept( listenWatcher->fd, (struct sockaddr *)&client_addr, &len );
    		if ( connFd < 0 )
    		{
        		log_module(LOG_ERROR, "acceptCallback", "ACCEPT ERROR:%s", strerror(errno));   
        		return;
    		}
    		
    		struct ev_io * receiveRequestWatcher = new struct ev_io;
    		if ( receiveRequestWatcher == NULL )
    		{
        		log_module( LOG_ERROR, "acceptCallback", "ALLOCATE MEMORY FAILED:%s", strerror( errno ));
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
    		log_module( LOG_INFO, "shakeHandCallback", "++++++++++RTMP SHAKE HAND START++++++++++");
    		int readBytes = NetUtil::readSpecifySize2(receiveRequestWatcher->fd, &c0, 1);
    		if ( readBytes == 1 )
    		{
    			log_module( LOG_INFO, "shakeHandCallback", "RECEIVE THE C0 PACKET:%d", c0);
    			s0=c0;
    			S1 s1;
    			s1.timestamp = htonl(0);
    			s1.zeroOrTime = htonl(0);
    			ServerUtil::generateSimpleRandomValue(s1.randomValue,1528);
    			struct iovec iovWrite[2];
    			iovWrite[0].iov_base=&s0;
			iovWrite[0].iov_len=1;
			iovWrite[1].iov_base=&s1;
			iovWrite[1].iov_len=sizeof(s1);
			int wrotenVectors=writev(receiveRequestWatcher->fd,iovWrite,2);
			if( wrotenVectors == -1)
			{
				log_module( LOG_INFO, "shakeHandCallback", "SENT THE S0,S1 PACKET FAILED:%s", strerror(errno));
			}
			else
			{
				log_module( LOG_INFO, "shakeHandCallback", "SENT THE S0,S1 PACKET SUCCESSED. S0:%d S1.timestamp:%u",s0,s1.timestamp);
				C1 c1;
				C2 c2;
				struct iovec iovRead[2];
				iovRead[0].iov_base=&c1;
				iovRead[0].iov_len=sizeof(c1);
				iovRead[1].iov_base=&c2;
				iovRead[1].iov_len=sizeof(c2);
				readBytes=readv(receiveRequestWatcher->fd, iovRead, 2);
				if( readBytes == -1 )
				{
					log_module( LOG_INFO, "shakeHandCallback", "READ THE C1,C2 PACKET FAILED:%s", strerror(errno));
				}
				else
				{
					log_module( LOG_INFO, "shakeHandCallback", "READ THE C1,C2 PACKET SUCCESSED. C1.timestamp:%u C2.S2.timestamp:%u C2.C1.timestamp:%u", 
															     ntohl(c1.timestamp),ntohl(c2.timestamp),ntohl(c2.zeroOrTime));
					S2 s2;
					s2.timestamp=c1.timestamp;
					s2.zeroOrTime=s1.timestamp;
					memcpy(s2.randomValue,c1.randomValue,1528);
					write(receiveRequestWatcher->fd, &s2, sizeof(s2));
					unsigned char createStreamRequest[2048];
					log_module( LOG_INFO, "shakeHandCallback", "++++++++++RTMP SHAKE HAND DONE++++++++++");
					readBytes=read(receiveRequestWatcher->fd, createStreamRequest, 2048);
				}
			}
    		}
		DO_RECEIVE_REQUEST_CB_CLEAN();
	}
};
