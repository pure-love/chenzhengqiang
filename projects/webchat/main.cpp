/*
 * @author:chenzhengqiang
 * @company:swwy
 * @date:2015/12/23
 * @modified-date:
 * @version:1.0
 * @desc:
 * this is the implementation of the simple rtmp server named xtrartmp
*/

#include "wcserver.h"
#include "netutil.h"
#include "serverutil.h"
#include <iostream>
#include <cstdlib>
using std::cerr;
using std::endl;

using namespace czq;

static const char *DEFAULT_CONFIG_FILE="/etc/wcserver/server.conf";

int main( int ARGC, char ** ARGV )
{
	ServerUtil::CmdOptions cmdOptions;
	ServerUtil::handleCmdOptions( ARGC, ARGV, cmdOptions );
	ServerUtil::ServerConfig serverConfig;

    	if ( ! cmdOptions.configFile.empty() )
    	{
		ServerUtil::readConfig( cmdOptions.configFile.c_str(), serverConfig );
    	}
    	else
    	{
		ServerUtil::readConfig( DEFAULT_CONFIG_FILE, serverConfig );
    	}

	service::WcServer wcServer(serverConfig);
    	if ( cmdOptions.needPrintHelp )
    	{
		wcServer.printHelp();
    	}

    	if ( cmdOptions.needPrintVersion )
    	{
		wcServer.printVersion();
    	}

    	int listenFd = NetUtil::registerTcpServer( serverConfig );
	if ( listenFd > 0 )
	{	
		wcServer.registerServer(listenFd);
		wcServer.serveForever();
	}
	else
	{
		cerr<<"register tcp server failed,please check the config file carefully"<<endl;
	}
    	return 0;
}	
