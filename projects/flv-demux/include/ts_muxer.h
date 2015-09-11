/*
@author:chenzhengqiang
@start date:2015/9/11
@modified date:
@desc:
*/

#ifndef _CZQ_TS_MUX_H_
#define _CZQ_TS_MUX_H_

#include "ts_mux_aac.h"
#include "ts_mux_h264.h"
#include <cstdio>

static unsigned int write_packet_no = 0;
int write_ts_pat( FILE *,unsigned char * buf);
int write_ts_pmt( FILE *, unsigned char * buf);
int write_adaptive_head_fields(Ts_Adaptation_field  * ts_adaptation_field,unsigned int Videopts);              //填写自适应段标志帧头的
int write_adaptive_tail_fields(Ts_Adaptation_field  * ts_adaptation_field);                                    //填写自适应段标志帧尾的
int ts_mux_for_h264_aac( const char *h264_file,const char * aac_file , const char * ts_file );
int pes_2_ts( FILE *, TsPes * ts_pes,unsigned int Video_Audio_PID ,Ts_Adaptation_field * ts_adaptation_field_Head ,Ts_Adaptation_field * ts_adaptation_field_Tail,
		   unsigned long  Videopts,unsigned long Adudiopts);
int create_adaptive_ts(Ts_Adaptation_field * ts_adaptation_field,unsigned char * buf,unsigned int AdaptiveLength);
#endif