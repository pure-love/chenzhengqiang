/*
*@filename:xtrartmp.h
*@author:chenzhengqiang
*@start date:2015/11/26 11:26:16
*@modified date:
*@desc: 
*/



#ifndef _CZQ_XTRARTMP_H_
#define _CZQ_XTRARTMP_H_
#include"serverutil.h"
//write the function prototypes or the declaration of variables here
namespace czq
{
	class XtraRtmp
	{
		public:
			XtraRtmp( const ServerConfig & serverConfig);
			void printHelp();
			void printVersion();
			void registerServer( int listenFd );
			void serveForever();
		private:
			XtraRtmp( const XtraRtmp &){}
			XtraRtmp & operator=(const XtraRtmp &){ return *this;}
		private:
			int listenFd_;
			ServerConfig serverConfig_;
	};
};
#endif
