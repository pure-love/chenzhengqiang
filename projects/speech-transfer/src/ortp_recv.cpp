/*
@author:chenzhengqiang
@start date:2015/8/11
@modified date:
@desc:
*/

#include "ortp_recv.h"
#include<climits>
RtpSession * oRTP_recv_init(int rtp_port,int rtcp_port,int payload_type)
{
       RtpSession *session;
    	ortp_init();
	ortp_scheduler_init();
	session=rtp_session_new(RTP_SESSION_RECVONLY);	
	rtp_session_set_scheduling_mode(session,1);
	rtp_session_set_blocking_mode(session,0);
	rtp_session_set_local_addr(session,"0.0.0.0",rtp_port,rtcp_port);
	rtp_session_set_connected_mode(session,TRUE);
	rtp_session_set_symmetric_rtp(session,TRUE);
	rtp_session_enable_adaptive_jitter_compensation(session,TRUE);
	rtp_session_set_jitter_compensation(session,40);
	rtp_session_set_payload_type(session,payload_type);
      return session;
}


/*
@args:
@returns:-1 indicates buffer size is too small
*/
int oRTP_recv_now( RtpSession *session,unsigned char *buffer,int buffer_size,unsigned int &time_stamp)
{
     	int have_more = 1;
       int received_bytes = 0;
       int total_bytes = 0;
       int stream_received = 0;
       while (have_more)
       {
                received_bytes = rtp_session_recv_with_ts(session,buffer+total_bytes,
                buffer_size-total_bytes,time_stamp,&have_more);

                if ( received_bytes > 0 ) 
                stream_received=1;
                /* this is to avoid to write to disk some silence before the first RTP packet is returned*/	
                if ((stream_received) && ( received_bytes >0)) 
                total_bytes += received_bytes;
                
                if( total_bytes > buffer_size )
                {
                     return -1;
                }
	 }
       if( time_stamp >= (UINT_MAX -160))
       {
           time_stamp = 0;
       }
       else
       time_stamp+=160;
       return total_bytes;
}

void oRTP_recv_close(RtpSession *session)
{
    rtp_session_destroy(session);
    ortp_exit();
}

