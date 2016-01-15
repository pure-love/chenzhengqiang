/*
*@file name:serverutility.h
*@author:chenzhengqiang
*@start date:2015/12/23
*/

#ifndef CZQ_SERVERUTILITY_H_
#define CZQ_SERVERUTILITY_H_
#include<string>
#include<map>
#include<sys/epoll.h>
#include<signal.h>
using std::string;


namespace czq
{
	
	struct ServerConfig
	{
		std::map<std::string, std::string> meta;
		std::map<std::string, std::string> server;
		std::string usage;
	};
	
	struct CmdOptions
	{
		bool runAsDaemon;
		bool needPrintHelp;
		bool needPrintVersion;
		std::string configFile;	
	};
	
	class ServerUtil
	{
		public:
		static int setNonBlocking(int fd);	
		static void handleCmdOptions( int ARGC, char * * ARGV, CmdOptions & cmdOptions );
		static void readConfig( const char * configFile, ServerConfig & serverConfig );
		static void generateSimpleRandomValue(unsigned char*buffer, unsigned int bufferLen);
	};


	namespace Epoll
	{
		enum EpollEvents{READ=EPOLLIN | EPOLLET, WRITE=EPOLLOUT|EPOLLET};
		int IOSet(int epollFd, int fd, const EpollEvents & epollEvents, bool nonBlocking = true);
		int IODel(int epollFd, int fd, const EpollEvents & epollEvents, bool nonBlocking = true);
	};

	namespace Signal
	{
		int registerHandler(int signum, sighandler_t sigHandler, bool restart = true);
	};
};
#endif
