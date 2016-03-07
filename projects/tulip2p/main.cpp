/*
 * @author:chenzhengqiang
 * @company:swwy
 * @date:2016/2/1
 * @modified-date:
 * @version:1.0
 * @desc:
 * this is the implementation of the file live server
*/

#include "tulip2pserver.h"
#include "netutil.h"
#include "serverutil.h"
#include <iostream>
#include <cstdlib>
using std::cerr;
using std::endl;

using namespace czq;

static const char *DEFAULT_CONFIG_FILE="/etc/tulip2p/server.conf";

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

	service::Tulip2pServer  tulip2pServer(serverConfig);
    if ( cmdOptions.needPrintHelp )
    {
		tulip2pServer.printHelp();
    }

    if ( cmdOptions.needPrintVersion )
    {
		tulip2pServer.printVersion();
    }

    int serverFd = NetUtil::registerUdpServer( serverConfig.server["bind-address"].c_str(), atoi(serverConfig.server["bind-port"].c_str()));
	if ( serverFd > 0 )
	{	
		tulip2pServer.registerServer(serverFd);
		tulip2pServer.serveForever();
	}
	else
	{
		cerr<<"register ucp server failed,please check the config file carefully"<<endl;
	}
    return 0;
}	
