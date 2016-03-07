/*
*@filename:fuckstreamer.h
*@author:chenzhengqiang
*@start date:2016/02/24 09:32:58
*@modified date:
*@desc: 
*/



#ifndef _CZQ_FUCKSTREAMER_H_
#define _CZQ_FUCKSTREAMER_H_

#include "serverutil.h"
#include <cstdio>
#include <map>

//write the function prototypes or the declaration of variables here
namespace czq
{
	namespace service
	{
		class FuckStreamer
		{
			public:
				FuckStreamer(const ServerUtil::ServerConfig & serverConfig);
				void printHelp();
				void printVersion();
				void serveForever();
			private:
				ServerUtil::ServerConfig serverConfig_;
				struct ConnInfo
				{
					int connFd;
					bool first;
					FILE *handler;
					char stream[1024*200];
					int sentBytes;
				};

				char *streamerIP_;
				int streamerPort_;
				int concurrency_;
				int totalConcurrency_;
				FILE *performanceLogHandler_;
				
				enum{MAX_BUFFER_SIZE=1024*10};
				std::map<int, ConnInfo> ConnInfoPool_;
				char buffer_[MAX_BUFFER_SIZE];
				char *first_;
				char *element_;
				char *end_;
				int epollFd_;
			private:
				void  initConn();
		};
	};
};
#endif
