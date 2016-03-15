/*
*@filename:xtrartmp.h
*@author:chenzhengqiang
*@start date:2015/3/15 11:26:16
*@modified date:
*@desc: 
*/



#ifndef _CZQ_WCSERVER_H_
#define _CZQ_WCSERVER_H_
#include "serverutil.h"
namespace czq
{
	//the detailed definition of xtrartmp class
	namespace service
	{
		class WcServer
		{
		        public:
				WcServer(const ServerUtil::ServerConfig & serverConfig);
				void printHelp();
				void printVersion();
				void registerServer( int listenFd );
				void serveForever();
				void shakeHand(int connfd, const char *serverKey);
				char * fetchSecKey(const char * buf);
				char * computeAcceptKey(const char * buf);
				char * analyData(const char * buf,const int bufLen);
				char * packData(const char * message,unsigned long * len);
				void response(int connfd,const char * message);
			private:
				int listenFd_;
				ServerUtil::ServerConfig serverConfig_;
			private:
				enum{
						REQUEST_LEN_MAX=1024,
						DEFEULT_SERVER_PORT=8000,
						WEB_SOCKET_KEY_LEN_MAX=256,
						RESPONSE_HEADER_LEN_MAX=1024,
						LINE_MAX=256
					};
		};

		void  acceptCallback( struct ev_loop * mainEventLoop, struct ev_io * listenWatcher, int revents );
		void  shakeHandCallback( struct ev_loop * eventLoopEntry, struct ev_io * , int revents );
		void  recvMessageCallback( struct ev_loop * eventLoopEntry, struct ev_io * , int revents );
		void  sendReplyCallback( struct ev_loop * eventLoopEntry, struct ev_io * , int revents );
	};
};
#endif
