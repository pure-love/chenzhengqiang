#ifndef _TRANSFER_H_
#define _TRANSFER_H_
#include "netutility.h"

static const int CAMERA=0;
static const int PC = 1;
static const int CREATE = 0;
static const int CANCEL  =  1;
static const int JOIN = 0;
static const int LEAVE  =  1;


struct CLIENT_REQUEST
{
    uint8_t type;// 0 indicates CMERA,1 indicates PC
    uint8_t action;// 0 indicates start,1 indicates stop
    std::string channel;
    long timestamp;
    long UID;	
};

void serve_forever(ssize_t sock_fd, const CONFIG & config);
void handle_udp_msg( int sock_fd );
bool parse_client_request(const uint8_t *packet,size_t size,CLIENT_REQUEST & client_request);
#endif
