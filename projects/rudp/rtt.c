#include "rtt.h"

int rtt_d_flag = 0;

#define RTT_RTOCALC(ptr) ((ptr)->rtt_srtt+(4.0 *(ptr)->rtt_rttvar))

static float rtt_minmax( float rto )
{
	if( rto < RTT_RXTMIN )
		rto = RTT_RXTMIN;
	else if( rto > RTT_RXTMAX )
		rto = RTT_RXTMAX;
	return rto;
}

void rtt_init( struct rtt_info * ptr )
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ptr->rtt_base = tv_sec;

	ptr->rtt_rtt = 0;
	ptr->rtt_srtt = 0;
	ptr->rtt_rttvar = 0.75;
	ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
}


uint32_t rtt_ts( struct rtt_info *ptr )
{
	uint32_t timestamp;
	struct timeval tv;
	gettimeofday(&tv,NULL);
	timestamp = ((tv.tv_sec-ptr->rtt_base) * 1000) + ( tv.tv_usec / 1000 );
	return (timestamp);
}


void rtt_newpack( struct rtt_info * ptr )
{
	ptr->rtt_nrexmt = 0;
}


int rtt_start( struct rtt_info * ptr )
{
	return ((int) (ptr->rtt_rto+ 0.5));
	//return value can be used as:alarm(rtt_start(&foo))
}


void rtt_stop( struct rtt_info * ptr, uint32_t ms)
{
	double delta;
	ptr->rtt_rtt = ms /1000.0;

	delta = ptr->rtt_rtt - ptr->rtt_srtt;
	ptr->rtt_srtt += delta / 8;

	if( delta < 0. 0 )
		delta = -delta;
	ptr->rtt_rttvar += ( delta - ptr->rtt_rttvar) /4;
	ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
	
}


int rtt_timeout( struct rtt_info * ptr )
{
	ptr->rtt_rto *=2;
	if( ++ptr->rtt_nrexmt > RTT_MAXNREXMT )
		return -1;
	return 0;
}
