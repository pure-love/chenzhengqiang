/*
*@file name:serverutility.h
*@author:chenzhengqiang
*@start date:2015/12/23
*/

#ifndef CZQ_SERVERUTILITY_H_
#define CZQ_SERVERUTILITY_H_
#include<string>
#include<map>
using std::string;


namespace czq
{
	namespace ServerUtil
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
		
		void handleCmdOptions( int ARGC, char * * ARGV, CmdOptions & cmdOptions );
		void readConfig( const char * configFile, ServerConfig & serverConfig );
		bool fileExists(const char *file);
		void generateSimpleRandomValue(unsigned char*buffer, unsigned int bufferLen);
		void setNonBlocking(int fd);
		ssize_t readSpecifySize( int fd, void *buffer, size_t totalBytes);
	};

	namespace Epoll
	{
		
	};

	namespace Signall
	{
	};
};
#endif
