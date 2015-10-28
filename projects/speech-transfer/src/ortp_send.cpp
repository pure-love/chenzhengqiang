/*
@author:chenzhengqiang
@start date:2015/8/11
@modified date:
@desc:
*/

#include "ortp_send.h"

RtpSession * oRTP_send_init(const char * remote_addr,int rtp_port,int payload_type,int ssrc)
{
       RtpSession *session;
    	ortp_init();
	ortp_scheduler_init();
	session=rtp_session_new(RTP_SESSION_SENDONLY);	
	rtp_session_set_scheduling_mode(session,1);
	rtp_session_set_blocking_mode(session,0);
	rtp_session_set_remote_addr(session,remote_addr,rtp_port);
	rtp_session_set_payload_type(session,payload_type);
	rtp_session_set_ssrc(session,ssrc);
      return session;
}

RtpSession *oRTP_send_init(int payload_type)
{
    RtpSession *session;
    ortp_init();
    ortp_scheduler_init();
    session=rtp_session_new(RTP_SESSION_SENDONLY);	
    rtp_session_set_scheduling_mode(session,1);
    rtp_session_set_blocking_mode(session,0);
    rtp_session_set_payload_type(session,payload_type);
    return session;
}

void oRTP_send_now( RtpSession *session,unsigned char *buffer,int buffer_size,unsigned int time_stamp)
{
     rtp_session_send_with_ts(session,buffer,buffer_size,time_stamp);
}

void oRTP_send_close(RtpSession *session)
{
    rtp_session_destroy(session);
    ortp_exit();
}

