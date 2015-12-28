/*
*@filename:xtrartmp.h
*@author:chenzhengqiang
*@start date:2015/11/26 11:26:16
*@modified date:
*@desc: 
*/



#ifndef _CZQ_XTRARTMP_H_
#define _CZQ_XTRARTMP_H_
#include<ev++.h>
#include"serverutil.h"
//write the function prototypes or the declaration of variables here
namespace czq
{
	class XtraRtmp
	{
		public:
			XtraRtmp(const ServerConfig & serverConfig);
			~XtraRtmp();
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
			struct ev_loop *mainEventLoop_;
                    struct ev_io *listenWatcher_;
                    struct ev_io *acceptWatcher_;            
	};

	//the S0 packet for the handshake's sake
	typedef struct _CS12 
	{
	    unsigned int timestamp;
	    unsigned int zeroOrTime;
	    unsigned char randomValue[1528];
	}C1,S1,C2,S2;

	//the RtmpPacket's rtmp header's definition
	struct RtmpPacketHeader
	{
		unsigned char size;
		unsigned char chunkStreamID;
		unsigned int timestamp;
		unsigned int AMFSize;
		unsigned char AMFType;
		unsigned int streamID;
	};
	//the RtmpPacket's definition
	struct RtmpPacket
	{
		RtmpPacketHeader rtmpPacketHeader;
		unsigned char *rtmpPacketPayload;
	};
	void acceptCallback( struct ev_loop * mainEventLoop, struct ev_io * listenWatcher, int revents );
       void shakeHandCallback( struct ev_loop * mainEventLoop, struct ev_io * receiveRequestWatcher, int revents );
	int parseRtmpPacket(unsigned char *rtmpSignallingBuf, size_t len, RtmpPacket & rtmpPacket);    
};
#endif
