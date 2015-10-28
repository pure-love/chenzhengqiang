#include "ortp_send.h"
#include "ortp_recv.h"
#include <ortp/ortp.h>
#include <cstdio>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <ortp/telephonyevents.h>
#include <signal.h>


static const char *help="usage: rtpsend  remote_addr remote_port\n";
static const int BUFFER_SIZE = 160;

void payload_type_cb(RtpSession *session)
{
	printf("hey, the payload_type has changed !\n");
}

int main(int argc, char*argv[])
{
        if( argc != 3)
       {
           printf(help);
           return -1;
       }
	RtpSession *send_session;
	const char *buffer="hello world";
       unsigned int timestamp = 0;
       unsigned int timestam_enc = 160;
       ortp_init();
	ortp_scheduler_init();
       //rtp_profile_set_payload(&av_profile,13,&payload_type_telephone_event);
       RtpSession *session_recv = oRTP_recv_init(54320,54321,PAYLOAD_OTHER);
       unsigned char recv_buffer[1024];
       while(true)
       {
            int total_bytes = oRTP_recv_now(session_recv,recv_buffer,1024,timestamp);
            if( total_bytes <=0 )
                continue;
            recv_buffer[total_bytes]='\0';
            printf("recv %s from 1314 %d bytes",(char *)recv_buffer);
            getchar();
            break;
       }
       
      
       send_session = oRTP_send_init(argv[1],atoi(argv[2]),0,12345);
       while( true )
       {
           oRTP_send_now(send_session,(unsigned char *)buffer,strlen(buffer),timestamp);
           if( timestamp >= (UINT_MAX -160) )
           {
               timestamp = 0;
           }
           else
           timestamp+=timestam_enc;
           printf("session0:send 11 bytes timestamp:%u\n",timestamp);
       }
       oRTP_send_close(send_session);
	return 0;
}

