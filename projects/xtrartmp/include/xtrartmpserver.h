/*
*@filename:xtrartmp.h
*@author:chenzhengqiang
*@start date:2015/11/26 11:26:16
*@modified date:
*@desc: 
*/



#ifndef _CZQ_XTRARTMPSERVER_H_
#define _CZQ_XTRARTMPSERVER_H_
#include "xtrartmp.h"
#include "serverutil.h"
#include <ev++.h>
#include <vector>
//write the function prototypes or the declaration of variables here
namespace czq
{
	//the detailed definition of xtrartmp class
	namespace service
	{
		class XtraRtmpServer
		{
		        public:
				XtraRtmpServer(const ServerUtil::ServerConfig & serverConfig);
				void printHelp();
				void printVersion();
				void registerServer( int listenFd );
				void serveForever();
			public:
				static bool onRtmpInvoke(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
			private:
				XtraRtmpServer( const XtraRtmpServer &){}
				XtraRtmpServer & operator=(const XtraRtmpServer &){ return *this;}

				static void onConnect(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static void onCheckbw(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static void onCreateStream(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static void onReleaseStream(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static void onFCPublish(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static void onPublish(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static void onPlay(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static void onGetStreamLength(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static size_t generateReply(unsigned char *reply, unsigned char *transactionID, const char * parameters[][2], int rows);
				static void onRtmpReply(XtraRtmp::RtmpPacketHeader & rtmpPacketHeader, unsigned char *transactionID, const char *parameters[][2], int rows, int connFD);
				static void onRtmpReply(const XtraRtmp::RtmpMessageType & rtmpMessageType, int connFD, size_t size=0);
				
			private:
				int listenFd_;
				ServerUtil::ServerConfig serverConfig_;	
		};
		

		//each single thread is related to a WorkthreadInfo structure
		struct WorkthreadInfo
		{
    			struct ev_loop *eventLoopEntry;
    			struct ev_async *asyncWatcher;
    			//std::map<std::string,CAMERAS_PTR> channelsPool;
		};

		typedef WorkthreadInfo * WorkthreadInfoPtr;
		void * workthreadEntry( void * arg );
		bool startupThreadsPool( int totalThreads );
		void freeThreadsPool();
		
		//global libev callback functions in namespace czq;
		void  acceptCallback( struct ev_loop * mainEventLoop, struct ev_io * listenWatcher, int revents );
		void  shakeHandCallback( struct ev_loop * mainEventLoop, struct ev_io * receiveRequestWatcher, int revents );
		void  consultCallback(struct ev_loop * mainEventLoop, struct ev_io * consultWatcher, int revents);
		void  receiveStreamCallback(struct ev_loop * mainEventLoop, struct ev_io * receiveStreamWatcher, int revents);
	};
};
#endif
