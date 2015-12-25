/*
*@filename:xtrartmp.cpp
*@author:chenzhengqiang
*@start date:2015/11/26 11:26:16
*@modified date:
*@desc: 
*/

#include "serverutil.h"
#include "xtrartmp.h"
#include<iostream>
#include<cstdlib>

using std::cout;
using std::endl;
namespace czq
{
	XtraRtmp::XtraRtmp( const ServerConfig & serverConfig ):listenFd_(-1),serverConfig_(serverConfig)
	{
		;
	}

	void XtraRtmp::printHelp()
	{
		cout<<serverConfig_.usage<<endl;
		exit(EXIT_SUCCESS);
	}

	void XtraRtmp::printVersion()
	{
		cout<<serverConfig_.meta["version"]<<endl;
		exit(EXIT_SUCCESS);
	}

	void XtraRtmp::registerServer( int listenFd )
	{
		listenFd_ = listenFd;
	}

	void XtraRtmp::serveForever()
	{
		if ( listenFd_ >= 0 )
		{
			while ( true )
			{
				cout<<"server already run"<<endl;
			};
		}
		else
		{
			;
		}
	}
};
