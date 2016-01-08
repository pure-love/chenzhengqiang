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
#include<vector>
//write the function prototypes or the declaration of variables here
namespace czq
{
	//the detailed definition of xtrartmp class
	class XtraRtmp
	{
	        public:
			XtraRtmp(const ServerConfig & serverConfig);
			~XtraRtmp();
			void printHelp();
			void printVersion();
			void registerServer( int listenFd );
			void serveForever();
	        public:  
		       enum RtmpShakeHand
                    {
                        //the fixed random value size related to shake hand is 1528
                        RANDOM_VALUE_SIZE=1528
                    };
                    
                    //the C1,S1,C2,S2 packet for the shake hand's sake
                    typedef struct CS12 
                    {
                        unsigned int timestamp;
                        unsigned int zeroOrTime;
                        unsigned char randomValue[RANDOM_VALUE_SIZE];
                    }C1,S1,C2,S2;
                    
                    //the type of rtmp message
	             enum RtmpMessageType
	             {
		            MESSAGE_CHANGE_CHUNK_SIZE=0x01,
		            MESSAGE_DROP_CHUNK=0x02,
		            MESSAGE_SEND_BOTH_READ=0x03,
		            MESSAGE_USER_CONTROL=0x04,
		            MESSAGE_WINDOW_ACKNOWLEDGEMENT_SIZE=0x05,
		            MESSAGE_SET_PEER_BANDWIDTH=0x06,
		            MESSAGE_AUDIO=0x08,
		            MESSAGE_VIDEO=0x09,
		            MESSAGE_AMF0_DATA=0x12,
		            MESSAGE_SUBTYPE=0x13,
		            MESSAGE_INVOKE=0x14		
	             };

			enum RtmpChannelType
			{
				CHANNEL_PING_BYTEREAD = 0x02,
				CHANNEL_INVOKE = 0x03,
				CHANNEL_AUDIO_VIDEO = 0x04
			};
			
                    //the RtmpPacket's rtmp header's definition
	            struct RtmpPacketHeader
	            {
			      unsigned char flag;		
	                   unsigned char size;
		            unsigned char chunkStreamID;
		            unsigned int timestamp;
		            unsigned int AMFSize;
		            unsigned char AMFType;
		            unsigned int streamID;
		            
	            };

	            //define the amf field type
	            enum AmfConstant
	            {
		            TYPE_CORE_NUMBER = 0x00,
		            TYPE_CORE_BOOLEAN = 0x01,
		            TYPE_CORE_STRING = 0x02,
		            TYPE_CORE_OBJECT = 0x03,
		            TYPE_CORE_NULL = 0x05,
		            TYPE_CORE_MAP = 0x08,
		            LEN_CORE_STRING=2,
		            LEN_CORE_NUMBER=8,
		            MAX_PAYLOAD_SIZE=1460
	            };

	            //the RtmpPacket's definition
	            struct RtmpPacket
	            {
		            RtmpPacketHeader rtmpPacketHeader;
		            unsigned char rtmpPacketPayload[MAX_PAYLOAD_SIZE];
	            };
	            
                   //the AMF0's definition
                   //ignore the coreMap object temporarily
                   struct AmfPacket
                    {
                        std::string command;
                        //the first byte indicates whether the transaction ID exist
                        //0->not exists
                        //1->exists
                        unsigned char transactionID[LEN_CORE_NUMBER+1];
                        //
                        std::map<std::string, void *>commandObject;
                        //the rtmp's bool type
                        char flag[2];
                        std::string streamName;
                        std::string publishType;
                        int streamIDOrMilliSeconds;
                        std::map<std::string, std::string> parameters; 
                    };
		public:
			static int parseRtmpPacket(unsigned char *buffer, size_t len, std::vector<RtmpPacket> & rtmpPacketPool);
			static void parseRtmpAMF0(unsigned char *buffer, size_t len, AmfPacket & amf);
			static void rtmpAMF0Dump(const AmfPacket & amfPacket);
			static bool onRtmpInvoke(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd);
		private:
			XtraRtmp( const XtraRtmp &){}
			XtraRtmp & operator=(const XtraRtmp &){ return *this;}
			static void onConnect(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd);
			static void onCheckbw(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd);
			static void onCreateStream(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd);
			static void onReleaseStream(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd);
			static void onFCPublish(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd);
			static void onPublish(RtmpPacketHeader &rtmpPacketHeader, AmfPacket &amfPacket, int connFd);
			static size_t generateReply(unsigned char *reply, unsigned char *transactionID, const char * parameters[][2], int rows);
			static void onRtmpReply(RtmpPacketHeader & rtmpPacketHeader, unsigned char *transactionID, const char *parameters[][2], int rows, int connFD);
			static void onRtmpReply(const RtmpMessageType & rtmpMessageType, int connFD, size_t size=0);
			
		private:
			int listenFd_;
			ServerConfig serverConfig_;
			struct ev_loop *mainEventLoop_;
                    struct ev_io *listenWatcher_;
                    struct ev_io *acceptWatcher_;	
	};


	//global libev callback functions in namespace czq;
	void  acceptCallback( struct ev_loop * mainEventLoop, struct ev_io * listenWatcher, int revents );
	void  shakeHandCallback( struct ev_loop * mainEventLoop, struct ev_io * receiveRequestWatcher, int revents );
	void  consultCallback(struct ev_loop * mainEventLoop, struct ev_io * consultWatcher, int revents);
	void  receiveStreamCallback(struct ev_loop * mainEventLoop, struct ev_io * receiveStreamWatcher, int revents);
};
#endif
