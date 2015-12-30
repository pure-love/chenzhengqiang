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
	enum RtmpShakeHand
	{
		RANDOM_VALUE_SIZE=1528
	};
	
	//the S0 packet for the handshake's sake
	typedef struct CS12 
	{
	    unsigned int timestamp;
	    unsigned int zeroOrTime;
	    unsigned char randomValue[RANDOM_VALUE_SIZE];
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

	enum Amf0Constant
	{
		TYPE_CORE_NUMBER = 0x00,
		TYPE_CORE_BOOLEAN = 0x01,
		TYPE_CORE_STRING = 0x02,
		TYPE_CORE_OBJECT = 0x03,
		TYPE_CORE_NULL = 0x05,
		TYPE_CORE_MAP = 0x08,
		LEN_CORE_STRING=2,
		LEN_CORE_NUMBER=8
	};
	
	//the AMF0's definition
	//ignore the coreMap object temporarily
	struct Amf0
	{
		//the first byte of coreNumber indicates whether the coreNumber exist
		//0->not exists
		//1->exists
		unsigned char coreNumber[LEN_CORE_NUMBER+1];
		//-1 indicates coreBoolean not exist
		char coreBoolean;
		//empty coreString indicates coreString not exist
		std::string coreString;
		//that the type of coreObject's value is coreString
		std::map<std::string, std::string> coreObjectOfString;
		//that the type of coreObject's value is coreNumber
		std::map<std::string, double> coreObjectOfNumber;
	};


	enum LogFacility
	{
		LOG_CONSOLE = 1,
		LOG_FILE = 2,
		LOG_ALL = 3
	};
	//the definition of xtrartmp class
	class XtraRtmp
	{
		public:
			XtraRtmp(const ServerConfig & serverConfig);
			~XtraRtmp();
			void printHelp();
			void printVersion();
			void registerServer( int listenFd );
			void serveForever();
			static int parseRtmpPacket(unsigned char *buffer, size_t len, RtmpPacket & rtmpPacket);
			static void parseRtmpAMF0(unsigned char *buffer, size_t len, Amf0 & amf0);
			static void rtmpAMF0Dump(const Amf0 & amf0);
		private:
			XtraRtmp( const XtraRtmp &){}
			XtraRtmp & operator=(const XtraRtmp &){ return *this;}
		private:
			int listenFd_;
			ServerConfig serverConfig_;
			struct ev_loop *mainEventLoop_;
                    struct ev_io *listenWatcher_;
                    struct ev_io *acceptWatcher_;
			static LogFacility logFacility_;		
	};

	
	void acceptCallback( struct ev_loop * mainEventLoop, struct ev_io * listenWatcher, int revents );
       void shakeHandCallback( struct ev_loop * mainEventLoop, struct ev_io * receiveRequestWatcher, int revents );
	
};
#endif
