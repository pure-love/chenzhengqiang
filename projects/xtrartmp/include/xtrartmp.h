/*
*@filename:xtrartmp.h
*@author:chenzhengqiang
*@start date:2016/02/29 13:41:44
*@modified date:
*@desc: 
*/



#ifndef _CZQ_XTRARTMP_H_
#define _CZQ_XTRARTMP_H_
#include "nana.h"
#include <string>
#include <map>
using std::string;
//write the function prototypes or the declaration of variables here
namespace czq
{
	namespace XtraRtmp
	{
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
	    		short eventType;	
	    		unsigned int windowAcknowledgementSize;				
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

		ssize_t   parseRtmpPacket(unsigned char *rtmpRequest, size_t len, RtmpPacket & rtmpPacket);
		void rtmpMessageDump(const RtmpMessageType & rtmpMessageType, Nana *nana= 0);
		void parseRtmpAMF0(unsigned char *buffer, size_t len, AmfPacket & amfPacket, 
											   RtmpMessageType rtmpMessageType);
		void rtmpAMF0Dump(const XtraRtmp::AmfPacket & amfPacket, Nana* nana);
	};
};
#endif
