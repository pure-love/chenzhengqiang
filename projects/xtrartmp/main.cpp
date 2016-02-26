/*
 * @author:chenzhengqiang
 * @company:swwy
 * @date:2015/12/23
 * @modified-date:
 * @version:1.0
 * @desc:
 * this is the implementation of the simple rtmp server named xtrartmp
*/

#include "xtrartmpserver.h"
#include "netutil.h"
#include "serverutil.h"
#include <iostream>
#include <cstdlib>
using std::cerr;
using std::endl;

using namespace czq;

static const char *DEFAULT_CONFIG_FILE="/etc/xtraRtmpServer/server.conf";

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

	service::XtraRtmpServer xtraRtmpServer(serverConfig);
    if ( cmdOptions.needPrintHelp )
    {
		xtraRtmpServer.printHelp();
    }

    if ( cmdOptions.needPrintVersion )
    {
		xtraRtmpServer.printVersion();
    }

    int listenFd = NetUtil::registerTcpServer( serverConfig );
	if ( listenFd > 0 )
	{	
		xtraRtmpServer.registerServer(listenFd);
		xtraRtmpServer.serveForever();
	}
	else
	{
		cerr<<"register tcp server failed,please check the config file carefully"<<endl;
	}
    return 0;
}	
