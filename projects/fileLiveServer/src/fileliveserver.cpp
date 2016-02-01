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
#include "rosehttp.h"
#include<iostream>
#include<stdint.h>

namespace czq
{
	namespace Service
	{	
		//define the global macro for libev's event clean
		#define DO_EVENT_CB_CLEAN(M,W) \
    		ev_io_stop(M, W);\
    		delete W;\
    		W= NULL;\
    		return;
		
		//the global config
		ServerUtil::ServerConfig SERVER_CONFIG;
		//the global logging tool
		Nana *nana=0;
		FileLiveServer::FileLiveServer( const ServerUtil::ServerConfig & serverConfig )
		:listenFd_(-1),serverConfig_(serverConfig),mainEventLoop_(0),listenWatcher_(0),
		acceptWatcher_(0)
		{
			SERVER_CONFIG = serverConfig;
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
	    		
	    		struct ev_io * requestWatcher = new struct ev_io;
	    		if ( requestWatcher == NULL )
	    		{
	        		nana->say(Nana::COMPLAIN, ToAcceptCallback, "ALLOCATE MEMORY FAILED:%s", strerror( errno ));
	        		close(connFd);
	        		return;
	    		}
	    
	    		requestWatcher->active = 0;
	    		requestWatcher->data = 0;   
	    		if ( nana->is(Nana::PEACE))
	    		{
	        		std::string peerInfo = NetUtil::getPeerInfo(connFd);
	        		nana->say(Nana::PEACE, ToAcceptCallback, "CLIENT %s CONNECTED SOCK FD IS:%d",
	                                                             peerInfo.c_str(), connFd);
	    		}

			//register the socket io callback for reading client's request    
    			ev_io_init(  requestWatcher, requestCallback, connFd, EV_READ );
    			ev_io_start( mainEventLoop, requestWatcher );	
		}


		void requestCallback( struct ev_loop * mainEventLoop, struct ev_io * requestWatcher, int revents )
		{
			#define ToRequestCallback __func__

	    		if ( EV_ERROR & revents )
	    		{
	        		nana->say(Nana::COMPLAIN, ToRequestCallback, "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
				RoseHttp::replyWithRoseHttpStatus( 500, requestWatcher->fd);
        			close(requestWatcher->fd );
				DO_EVENT_CB_CLEAN(mainEventLoop, requestWatcher);
	    		}

			 ssize_t receivedBytes;
    			//used to store those key-values parsed from http request
    			char request[1024];
    			//read the http header and then parse it
    			receivedBytes = RoseHttp::readRoseHttpHeader( requestWatcher->fd, request, sizeof(request) );
    			if(  receivedBytes <= 0  )
    			{     
          			if(  receivedBytes == 0  )
          			{
              			if( nana->is(Nana::PEACE) )
              			{
                    			std::string peerInfo = NetUtil::getPeerInfo( requestWatcher->fd );
                    			nana->say( Nana::PEACE, ToRequestCallback," READ 0 BYTE FROM %s CLIENT DISCONNECTED ALREADY",
                                                                                           peerInfo.c_str() );
              			}
          			}
          			else if(  receivedBytes == LENGTH_OVERFLOW  )
          			{
              			if( nana->is(Nana::PEACE) )
              			{
                    			std::string peerInfo = NetUtil::getPeerInfo( requestWatcher->fd );
                    			nana->say( Nana::PEACE, ToRequestCallback, "CLIENT %s'S HTTP REQUEST LINE IS TOO LONG",
                                                                                          peerInfo.c_str() );
              			}
          			}
          
          			RoseHttp::replyWithRoseHttpStatus( 400, requestWatcher->fd );
          			close( requestWatcher->fd );
          			DO_EVENT_CB_CLEAN(mainEventLoop, requestWatcher);
    			}
    
    			request[receivedBytes]='\0';
    			nana->say( Nana::HAPPY, ToRequestCallback, "HTTP REQUEST HEADER:%s", request );
			RoseHttp::SimpleRoseHttpHeader simpleRoseHttpHeader;
			ssize_t ret = RoseHttp::parseSimpleRoseHttpHeader( request, strlen( request ), simpleRoseHttpHeader);
    			if( ret == STREAM_FORMAT_ERROR )
    			{
        			nana->say( Nana::COMPLAIN, ToRequestCallback, "HTTP REQUEST LINE FORMAT ERROR");
        			RoseHttp::replyWithRoseHttpStatus( 400, requestWatcher->fd );
        			DO_EVENT_CB_CLEAN(mainEventLoop, requestWatcher);	
    			}
			DO_EVENT_CB_CLEAN(mainEventLoop, requestWatcher);	
		}
	};
};
