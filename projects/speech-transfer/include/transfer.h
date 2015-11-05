/*
*@file name:transfer.h
*@author:chenzhengqiang
*@start date:2015/6/6
*@modified date:
*/
#ifndef _TRANSFER_H_
#define _TRANSFER_H_

#include "netutility.h"
#include <stdint.h>
static const int CAMERA=0;
static const int PC = 1;
static const int CREATE = 0;
static const int CANCEL  =  1;
static const int JOIN = 0;
static const int LEAVE  =  1;
static const int REPLY_FLAG=0x55aa;

//self-define structure relates to client's request
//each field parsed from the personal signalling
struct CLIENT_REQUEST
{
    uint8_t type;// 0 indicates CMERA,1 indicates PC
    uint8_t action;// 0 indicates start,1 indicates stop
    std::string channel;
    long timestamp;
    long UID;	
};



//fixed buffer size of personal signalling
static const int SIGNALLING_BUFFER_SIZE=12+12+64;
//fixed buffer size of opus rtp packet
static const int OPUS_RTP_BUFFER_SIZE=12+40;

//indicates the personal singling rtp packet's payload type
static const int PERSONAL_SINGLING_TYPE=3;
//indicates the opus rtp packet's payload type
static const int OPUS_RTP_PAYLOAD_TYPE = 0;

static const size_t CHANNELS = 1;
static const size_t LSB_DEPTH = 16;
static const size_t USE_VBR = 0;
static const size_t USE_VBR_CONSTANT = 1;


//only for camera when receive its request,then just reply it
struct SERVER_REPLY
{
    int start; //0x55aa indicates sender is speech transfer server
    int status;//0 indicates ok,1 indicates failed
    int end; //also 0x55aa,it must equal to start
};


//the "ID" consists of client'S IP address and port
//example:127.0.0.1:8080
typedef std::string ID;
typedef int PORT;

//singla client's information
struct CONN_CLIENT
{
    PORT conn_port;
    struct sockaddr_in conn_addr;
};


//usefull aliases 
typedef uint8_t * RTP_PACKET;
typedef std::map<ID,CONN_CLIENT> CLIENTS;
typedef std::map<ID,CONN_CLIENT>::iterator client_iter;
typedef std::map<ID,CONN_CLIENT>::iterator & client_referrence_iter;
typedef std::string CHANNEL;
typedef std::map<CHANNEL,CLIENTS>::iterator chat_room_iter;
typedef std::map<CHANNEL,CLIENTS>::iterator & chat_room_referrence_iter;


//function's prototype here
void serve_forever( ssize_t sock_fd, const CONFIG & config );
void handle_udp_msg( int sock_fd );
bool parse_client_request(const uint8_t *packet,size_t size,CLIENT_REQUEST & client_request);
bool client_already_in( const std::string &C_ID, chat_room_referrence_iter crr_iter );
bool client_already_in( const std::string & C_ID );
void broadcast_speech_message( int sock_fd, const uint8_t *rtp_packet,size_t size,
                                                             const std::string &C_ID, chat_room_referrence_iter cr_iter  );

void init_opus_encoder();
void init_opus_decoder();
bool parse_client_request( const uint8_t *packet, size_t size, CLIENT_REQUEST & client_request );
void do_mix_and_broadcast( int udp_sock_fd, chat_room_referrence_iter crr_iter );
void do_mix_and_broadcast_C( int udp_sock_fd, chat_room_referrence_iter crr_iter );
void do_mix_and_broadcast_S( int udp_sock_fd, chat_room_referrence_iter crr_iter );

#endif
