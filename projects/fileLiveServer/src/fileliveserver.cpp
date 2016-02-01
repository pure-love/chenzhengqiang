/*
*@filename:fileliveserver.cpp
*@author:chenzhengqiang
*@start date:2016/01/30 18:37:10
*@modified date:
*@desc: 
*/



#include "fileliveserver.h"
#include "common.h"
#include "errors.h"
#include "netutil.h"
#include "nana.h"
#include<iostream>
#include<stdint.h>

namespace czq
{
	namespace Service
	{
		//the global logging tool
		Nana *nana=0;
		
		FileLiveServer::FileLiveServer( const ServerUtil::ServerConfig & serverConfig )
		:listenFd_(-1),serverConfig_(serverConfig),mainEventLoop_(0),listenWatcher_(0),
		acceptWatcher_(0)
		{
			;	
		}

		FileLiveServer::~FileLiveServer()
		{
			if ( mainEventLoop_ != 0 )
			free(mainEventLoop_);
			if ( listenWatcher_ !=0 )
			delete listenWatcher_;
			if ( acceptWatcher_ != 0 )
			delete acceptWatcher_;
		}

		
		void FileLiveServer::printHelp()
		{
			std::cout<<serverConfig_.usage<<std::endl;
			exit(EXIT_SUCCESS);
		}


		void FileLiveServer::printVersion()
		{
			std::cout<<serverConfig_.meta["version"]<<std::endl<<std::endl;
			exit(EXIT_SUCCESS);
		}


		void FileLiveServer::registerServer( int listenFd )
		{
			listenFd_ = listenFd;
		}


		void FileLiveServer::serveForever()
		{
			if ( listenFd_ >= 0 )
			{
				if ( serverConfig_.server["daemon"] == "yes" )
    				{
        				daemon(0,0);
    				}

    				nana = Nana::born(serverConfig_.server["log-file"], atoi(serverConfig_.server["log-level"].c_str()),
    					    atoi(serverConfig_.server["flush-time"].c_str()));
    					   
    				nana->say(Nana::HAPPY, __func__,  "LISTEN SOCKET FD:%d", listenFd_);
    
    				//you have to ignore the PIPE's signal when client close the socket
    				struct sigaction sa;
    				sa.sa_handler = SIG_IGN;//just ignore the sigpipe
    				sa.sa_flags = 0;
    				if ( sigemptyset( &sa.sa_mask ) == -1 ||sigaction( SIGPIPE, &sa, 0 ) == -1 )
    				{ 
        				nana->say(Nana::COMPLAIN, __func__, "FAILED TO IGNORE SIGPIPE SIGNAL");
    				}
				else
				{
    					//initialization on main event-loop
    					mainEventLoop_ = EV_DEFAULT;
    					listenWatcher_ = new ev_io;
    					ev_io_init( listenWatcher_, acceptCallback, listenFd_, EV_READ );
             				ev_io_start( mainEventLoop_, listenWatcher_ );
            				ev_run( mainEventLoop_, 0 );
    				}
    				nana->die();
			}
			else
			{
				;
			}
		}

		void acceptCallback( struct ev_loop * mainEventLoop, struct ev_io * listenWatcher, int revents )
		{
			(void)mainEventLoop;
			#define ToAcceptCallback __func__

	    		if ( EV_ERROR & revents )
	    		{
	        		nana->say(Nana::COMPLAIN, ToAcceptCallback, "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
	        		return;
	    		}

	    		struct sockaddr_in clientAddr;
	    		socklen_t len = sizeof( struct sockaddr_in );
	    		int connFd = accept( listenWatcher->fd, (struct sockaddr *)&clientAddr, &len );
	    		if ( connFd < 0 )
	    		{
	        		nana->say(Nana::COMPLAIN, ToAcceptCallback, "ACCEPT ERROR:%s", strerror(errno));   
	        		return;
	    		}
	    		
	    		struct ev_io * receiveRequestWatcher = new struct ev_io;
	    		if ( receiveRequestWatcher == NULL )
	    		{
	        		nana->say(Nana::COMPLAIN, ToAcceptCallback, "ALLOCATE MEMORY FAILED:%s", strerror( errno ));
	        		close(connFd);
	        		return;
	    		}
	    
	    		receiveRequestWatcher->active = 0;
	    		receiveRequestWatcher->data = 0;   
	    		if ( nana->is(Nana::PEACE))
	    		{
	        		std::string peerInfo = NetUtil::getPeerInfo(connFd);
	        		nana->say(Nana::PEACE, ToAcceptCallback, "CLIENT %s CONNECTED SOCK FD IS:%d",
	                                                             peerInfo.c_str(), connFd);
	    		}
		}
	};
};
