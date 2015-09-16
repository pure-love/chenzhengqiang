/*
@author:chenzhengqiang
@start date:2015/9/11
@modified date:
@desc:
*/

#ifndef _CZQ_TS_MUX_H_
#define _CZQ_TS_MUX_H_
#include "ts.h"
#include <cstdio>
static unsigned int write_packet_no = 0;
int write_ts_pat_2_file( FILE *,unsigned char * ts_pat_buffer );
int write_ts_pmt_2_file( FILE *, unsigned char * ts_pmt_buffer);
int write_adaptive_head_fields(Ts_Adaptation_field  * ts_adaptation_field,unsigned int Videopts);              //填写自适应段标志帧头的
int write_adaptive_tail_fields(Ts_Adaptation_field  * ts_adaptation_field);                                    //填写自适应段标志帧尾的
int ts_mux_for_h264_aac( const char *h264_file,const char * aac_file , const char * ts_file );
int pes_2_ts( FILE *, TsPes * ts_pes,unsigned int Video_Audio_PID ,Ts_Adaptation_field * ts_adaptation_field_Head ,Ts_Adaptation_field * ts_adaptation_field_Tail,
		   unsigned long  Videopts,unsigned long Adudiopts);
int create_adaptive_ts(Ts_Adaptation_field * ts_adaptation_field,unsigned char * buf,unsigned int AdaptiveLength);
void aac_frame_2_pes( unsigned char *aac_frame, unsigned int frame_length, unsigned long aac_pts,TsPes  & aac_pes );
int h264_frame_2_pes(unsigned char *h264_frame,unsigned int frame_length,unsigned long h264_pts,TsPes & h264_pes);
#endif
