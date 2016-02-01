/*
*@filename:fileliveserver.h
*@author:chenzhengqiang
*@start date:2016/01/30 18:37:10
*@modified date:
*@desc: 
*/



#ifndef _CZQ_FILELIVESERVER_H_
#define _CZQ_FILELIVESERVER_H_
//write the function prototypes or the declaration of variables here
#include "serverutil.h"
#include <ev++.h>
namespace czq
{
	namespace Service
	{
		class FileLiveServer
		{
			public:
				FileLiveServer(const ServerUtil::ServerConfig & serverConfig);
				~FileLiveServer();
				void printHelp();
				void printVersion();
				void registerServer( int listenFd );
				void serveForever();
			private:
				int listenFd_;
				ServerUtil::ServerConfig serverConfig_;
				struct ev_loop *mainEventLoop_;
                    	struct ev_io *listenWatcher_;
                    	struct ev_io *acceptWatcher_;	
			private:
				FileLiveServer(const FileLiveServer &){}
				FileLiveServer & operator=(const FileLiveServer &) { return *this; }
		};

		void acceptCallback( struct ev_loop * mainEventLoop, struct ev_io * listenWatcher, int revents );
		void requestCallback( struct ev_loop * mainEventLoop, struct ev_io * listenWatcher, int revents );
	};
};
#endif
