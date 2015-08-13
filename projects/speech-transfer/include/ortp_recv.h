/*
@author:chenzhengqiang
@start date:2015/8/11
@modified date:
@desc:encapsulation of oRTP
*/
#ifndef _CZQ_ORP_H_
#define _CZQ_ORTP_H_
#include<ortp/ortp.h>

RtpSession * oRTP_recv_init(int rtp_port,int rtcp_port, int payload_type);
int oRTP_recv_now( RtpSession *session,unsigned char *buffer,int buffer_size,unsigned int &time_stamp);
void oRTP_recv_close(RtpSession *session);
#endif
