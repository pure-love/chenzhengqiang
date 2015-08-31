#include "rtt.h"
#include <stdint.h>
#include <setjmp.h>
#include <sys/types.h>

#define RTT_DEBUG

static struct RTT_INFO rtt_info;
static int rtt_init = 0;
static struct msghdr msgsend,msgrecv;

static struct hdr
{
    uint32_t sequence;
    uint32_t timestamp;	
} sendhdr,recvhdr;

static void sig_alrm( int signo );
static sigjmp_buf jmpbuf;

ssize_t dg_send_recv( int fd, const void *outbuff, size_t outbytes, void *inbuff, 
	                                 size_t inbytes, const struct sockaddr_in * dest_addr, socklen_t dest_len )
{
        ssize_t n;
	 struct iovec iovsend[2], iovrecv[2];
	 if( rtt_init == 0 )
	 {
	     rtt_init(&rtt_info);
	     rtt_init = 1;
	     rtt_d_flag = 1;	 
	 }

	 sendhdr.sequence++;
	 msgsend.msg_name = dest_addr;
	 msgsend.msg_namelen = dest_len;
	 msgsend.msg_iov = iovsend;
	 msgsend.msg_iovlen = 2;

	 iovsend[0].iov_base = &sendhdr;
	 iovsend[0].iov_len = sizeof( struct hdr);
	 iovsend[1].iov_base = outbuff;
	 iovsend[1].iov_len = outbytes;

	 msgrecv.msg_name = NULL;
	 msgrecv.msg_namelen = 0;
	 msgrecv.msg_iov = iovrecv;
	 msgrecv.msg_iovlen = 2;
	 iovrecv[0].iov_base = &recvhdr;
	 iovrecv[0].iov_len = sizeof( struct hdr );
	 iovrecv[1].iov_base = inbuff;
	 iovrecv[1].iov_len = inbytes;

	 signal(SIGALRM, sig_alrm);
	 rtt_newpack( &rtt_info );//reset the RTP as 0

	 sendagain:
	 	sendhdr.timestamp = rtt_ts( &rtt_info );//retrieve the current timestamp
		sendmsg(fd, &msgsend, 0);
		alarm(rtt_start(&rtt_info));
		if( sigsetjmp( jmpbuf, 1) != 0)
	      {
			if( rtt_timeout( &rtt_info) < 0 )
		      {
		      	      rtt_init = 0;
			      errno = ETIMEDOUT;
			      return -1;   
			}
			goto sendagain;
		}
		do
	       {
	       	n = recvmsg( fd, &msgrecv, 0);	
		}while( n < sizeof( struct hdr) || recvhdr.sequence != sendhdr.sequence );

		alarm(0);

		rtt_stop( &rtt_info, rtt_ts(&rtt_info) - recvhdr.timestamp);

		return (n -sizeof(struct hdr));
}


static void sig_alrm( int signo )
{
    siglongjmp( jmpbuf, 1);
}


