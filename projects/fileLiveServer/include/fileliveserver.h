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
#include <vector>
#include <ev++.h>
#include <string>
#include <map>
using std::string;
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
			public:
				void getMediaFiles(const char * dir, std::vector<std::string> & mediaFilePool);
			public:	
				ServerUtil::ServerConfig serverConfig_;
			private:
				int listenFd_;
				struct ev_loop *mainEventLoop_;
                    	struct ev_io *listenWatcher_;
                    	struct ev_io *acceptWatcher_;
				struct ev_io *writeWatcher_;
				bool stop_;
				int sleepTime_;
			private:
				FileLiveServer(const FileLiveServer &){}
				FileLiveServer & operator=(const FileLiveServer &) { return *this; }
				void directlyUpdateM3u8();
				void deleteMediaFiles( std::vector<std::string> & mediaFilePool );
				
		};

		struct M3u8
		{
			std::string header;
			std::vector<std::string> tsUrlPool;
			int pos;
		};
		
		typedef std::string Source;
		typedef std::map<Source, M3u8> M3u8Pool;

		
		void *upgradeThreadEntry( void * arg );
		void odtainM3u8Source( const char * dir );
		void writeCallback( struct ev_loop * mainEventLoop, struct ev_io * readWatcher, int revents );
		void acceptCallback( struct ev_loop * mainEventLoop, struct ev_io * listenWatcher, int revents );
		void requestCallback( struct ev_loop * mainEventLoop, struct ev_io * listenWatcher, int revents );
	};
};
#endif
