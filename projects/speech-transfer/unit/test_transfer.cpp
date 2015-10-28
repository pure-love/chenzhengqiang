#include"ortp_recv.h"
#include"ortp_send.h"
#include <signal.h>
#include <ortp/telephonyevents.h>


void payload_type_cb(RtpSession *session)
{
	printf("hey, the payload_type has changed !\n");
}

int main( int argc, char ** argv )
{
   rtp_profile_set_payload(&av_profile,13,&payload_type_telephone_event);
   RtpSession *session_recv = oRTP_recv_init(54320,54321,13);
   unsigned int timestamp = 0;
   unsigned char buffer[1024];
   int total_bytes = oRTP_recv_now(session_recv,buffer,1024,timestamp);
   buffer[total_bytes]='\0';
   printf("recv %s from 1314 %d bytes",(char *)buffer);

   RtpSession *session_send = oRTP_send_init("127.0.0.1",1316,0,12345);
   rtp_session_signal_connect(session_send,"payload_type_changed",(RtpCallback)payload_type_cb,0);
   rtp_session_signal_connect(session_send,"payload_type_changed",(RtpCallback)rtp_session_reset,0);
   while( true )
   {
       const char *send_buffer="hello world";
       oRTP_send_now(session_send,(unsigned char *)send_buffer,strlen(send_buffer),timestamp);
   }
   return 0;
}
