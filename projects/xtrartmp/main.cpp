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

    if ( cmdOptions.needPrintHelp )
    {
		XtraRtmp::printHelp();
    }

    if ( cmdOptions.needPrintVersion )
    {
		XtraRtmp::printVersion( serverConfig );
    }

    int listenFd = NetUtil::registerTcpServer( serverConfig.server["bind-address"].c_str(), atoi(serverConfig.server["bind-port"].c_str()));
	if ( listenFd > 0 )
	{	
		XtraRtmp xtraRtmp(listenFd, serverConfig);
		xtraRtmp.serveForever();
	}
	else
	{
		ServerUtil::errorQuit("register tcp server failed,please check the configure file");
	}
    return 0;
}	
