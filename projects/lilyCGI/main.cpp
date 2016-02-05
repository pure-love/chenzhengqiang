/*
*@filename:main.cpp
*@author:chenzhengqiang
*@start date:2016/01/15 09:30:01
*@modified date:
*@desc: 
*/


#include "serverutil.h"
#include "netutil.h"
#include "ppool.h"
#include "lilycgi.h"
#include <iostream>
#include <cstdlib>
using std::cerr;
using std::endl;
using namespace czq;

static const char *DEFAULT_CONFIG_FILE="/etc/lilycgi/server.conf";

int main( int ARGC, char ** ARGV )
{
	CmdOptions cmdOptions;
    	ServerUtil::handleCmdOptions( ARGC, ARGV, cmdOptions );
    	ServerConfig serverConfig;
		
    	if ( ! cmdOptions.configFile.empty() )
    	{
		ServerUtil::readConfig( cmdOptions.configFile.c_str(), serverConfig );
    	}
    	else
    	{
		ServerUtil::readConfig( DEFAULT_CONFIG_FILE, serverConfig );
    	}
	
    	int listenFd = NetUtil::registerTcpServer( serverConfig.server["bind-address"].c_str(), atoi(serverConfig.server["bind-port"].c_str()));
	if ( listenFd > 0 )
	{	
		ppool<SimpleCGI> * simpleCGIPool = ppool<SimpleCGI>::getInstance( listenFd );
		if ( simpleCGIPool )
		{
			simpleCGIPool->go();
			delete simpleCGIPool;
		}

		close(listenFd);
	}
	else
	{
		cerr<<"register tcp server failed,please check the config file carefully"<<endl;
	}
	return 0;
}
