/*
 * @author:chenzhengqiang
 * @company:swwy
 * @date:2016/2/1
 * @modified-date:
 * @version:1.0
 * @desc:
 * this is the implementation of the file live server
*/

#include "fileliveserver.h"
#include "netutil.h"
#include "serverutil.h"
#include <iostream>
#include <cstdlib>
using std::cerr;
using std::endl;

using namespace czq;

static const char *DEFAULT_CONFIG_FILE="/etc/fileLiveServer/server.conf";

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

	Service::FileLiveServer  fileLiveServer(serverConfig);
    if ( cmdOptions.needPrintHelp )
    {
		fileLiveServer.printHelp();
    }

    if ( cmdOptions.needPrintVersion )
    {
		fileLiveServer.printVersion();
    }

    int listenFd = NetUtil::registerTcpServer( serverConfig.server["bind-address"].c_str(), atoi(serverConfig.server["bind-port"].c_str()));
	if ( listenFd > 0 )
	{	
		fileLiveServer.registerServer(listenFd);
		fileLiveServer.serveForever();
	}
	else
	{
		cerr<<"register tcp server failed,please check the config file carefully"<<endl;
	}
    return 0;
}	
