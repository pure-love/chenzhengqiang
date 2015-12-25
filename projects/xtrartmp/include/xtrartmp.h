/*
*@filename:xtrartmp.h
*@author:chenzhengqiang
*@start date:2015/11/26 11:26:16
*@modified date:
*@desc: 
*/



#ifndef _CZQ_XTRARTMP_H_
#define _CZQ_XTRARTMP_H_
//write the function prototypes or the declaration of variables here
struct ServerConfig;
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
			void
		private:
			XtraRtmp( const XtraRtmp &){}
			XtraRtmp & operator=(const XtraRtmp &){}
		private:
			int listenFd_;
			const ServerConfig serverConfig_;
	};
};
#endif
