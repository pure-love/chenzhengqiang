/*
 * @author:chenzhengqiang
 * @company:swwy
 * @date:2015/12/23
 * @modified-date:
 * @version:1.0
 * @desc:
 * this is the implementation of the simple rtmp server named xtrartmp
*/

#include "xtrartmp.h"
#include "netutil.h"
#include "serverutil.h"
#include <iostream>
using std::cerr;
using std::endl;

using namespace czq;

static const char *DEFAULT_CONFIG_FILE="/etc/xtrartmp/server.conf";

int main( int ARGC, char ** ARGV )
{
    CmdOptions cmdOptions;
    ServerUtil::handleCMDOptions( ARGC, ARGV, cmdOptions );
    ServerConfig serverConfig;

    if ( ! cmdOptions.configFile.empty() )
    {
		ServerUtil::readConfig( cmdOptions.configFile.c_str(), serverConfig );
    }
    else
    {
		ServerUtil::readConfig( DEFAULT_CONFIG_FILE, serverConfig );
    }

    XtraRtmp xtraRtmp(serverConfig);
    if ( cmdOptions.needPrintHelp )
    {
		xtraRtmp.printHelp();
    }

    if ( cmdOptions.needPrintVersion )
    {
		xtraRtmp.printVersion();
    }

    int listenFd = NetUtil::registerTcpServer( serverConfig.server["bind-address"].c_str(), atoi(serverConfig.server["bind-port"].c_str()));
	if ( listenFd > 0 )
	{	
		xtraRtmp.registerServer(listenFd);
		xtraRtmp.serveForever();
	}
	else
	{
		cerr<<"register tcp server failed,please check the config file carefully"<<endl;
	}
    return 0;
}	
