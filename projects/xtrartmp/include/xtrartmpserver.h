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
#include <pthread.h>

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
				static ssize_t onRtmpInvoke(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
			private:
				XtraRtmpServer( const XtraRtmpServer &){}
				XtraRtmpServer & operator=(const XtraRtmpServer &){ return *this;}

				static ssize_t onConnect(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static ssize_t onCheckbw(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static ssize_t onCreateStream(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static ssize_t onReleaseStream(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static ssize_t onFCPublish(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static ssize_t onPublish(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static ssize_t onPlay(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static ssize_t onGetStreamLength(XtraRtmp::RtmpPacketHeader &rtmpPacketHeader, XtraRtmp::AmfPacket &amfPacket, int connFd);
				static size_t generateReply(unsigned char *reply, unsigned char *transactionID, const char * parameters[][2], int rows);
				static ssize_t onRtmpReply(XtraRtmp::RtmpPacketHeader & rtmpPacketHeader, unsigned char *transactionID, const char *parameters[][2], int rows, int connFD);
				static ssize_t onRtmpReply(const XtraRtmp::RtmpMessageType & rtmpMessageType, int connFD, size_t size=0);
				
			private:
				int listenFd_;
				ServerUtil::ServerConfig serverConfig_;	
		};
		

		//the channel structure
		typedef struct Channel
		{
			;
		}*ChannelPtr;

		
		struct LibevAsyncData
		{
    			int type;
    			int sockFd;
    			void *data;
		};

		
		//each single thread is related to a WorkthreadInfo structure
		struct WorkthreadInfo
		{
    			struct ev_loop *eventLoopEntry;
    			struct ev_async *asyncWatcher;
    			std::map<std::string, ChannelPtr> channelsPool;
		};

		typedef WorkthreadInfo * WorkthreadInfoPtr;
		void * workthreadEntry( void * arg );
		bool startupThreadsPool( size_t totalThreads );
		void freeThreadsPool();
		
		//global libev callback functions in namespace czq;
		void  acceptCallback( struct ev_loop * mainEventLoop, struct ev_io * listenWatcher, int revents );
		void  shakeHandCallback( struct ev_loop * mainEventLoop, struct ev_io * receiveRequestWatcher, int revents );
		void  consultCallback(struct ev_loop * mainEventLoop, struct ev_io * consultWatcher, int revents);
		void  receiveStreamCallback(struct ev_loop * mainEventLoop, struct ev_io * receiveStreamWatcher, int revents);
	};
};
#endif
