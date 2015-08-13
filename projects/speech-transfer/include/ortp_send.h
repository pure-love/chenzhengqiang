/*
@author:chenzhengqiang
@start date:2015/8/11
@modified date:
@desc:encapsulation of oRTP
*/
#ifndef _CZQ_ORP_H_
#define _CZQ_ORTP_H_
#include<ortp/ortp.h>

RtpSession * oRTP_send_init(const char * remote_addr,int rtp_port, int payload_type, int ssrc);
RtpSession *oRTP_send_init(int payload_type);
void oRTP_send_now( RtpSession *session,unsigned char *buffer,int buffer_size,unsigned int time_stamp);
void oRTP_send_close(RtpSession *session);
#endif
