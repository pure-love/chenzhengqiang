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
		static void handleCmdOptions( int ARGC, char * * ARGV, CmdOptions & cmdOptions );
		static void readConfig( const char * configFile, ServerConfig & serverConfig );
	};
};
#endif
