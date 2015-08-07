#ifndef _RUDP_COMMON_DEFINE_H_
#define _RUDP_COMMON_DEFINE_H_
static struct hand{
	int (*h)(rudp_socket_t, rudp_event_t,struct sockaddr_in *);
	int fd;
	struct hand* Next;
} *handlers;

static struct socketnode{  
	int socket_fd;
	struct sockaddr_in* to;
	int state;
	int seq;
	int retransmit;
	int close_ask;
	int nextpacket;
	int* i;
	struct buffer_data_list* buff_data;
	struct socketnode *Next;  
}* sockets, *iterator;  
#endif
