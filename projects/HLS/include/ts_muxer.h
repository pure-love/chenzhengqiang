#ifndef _CZQ_TS_MUX_H_
#define _CZQ_TS_MUX_H_

#include "aac.h"
#include "h264.h"
#include <cstdio>

int Write_Pat(FILE *,unsigned char * buf);
int Write_Pmt( FILE *, unsigned char * buf);
int Take_Out_Pes(FILE *,TsPes * tspes ,unsigned long time_pts,unsigned int frametype,unsigned int *videoframetype); //0 视频 ，1音频
int WriteAdaptive_flags_Head(Ts_Adaptation_field  * ts_adaptation_field,unsigned int Videopts);              //填写自适应段标志帧头的
int WriteAdaptive_flags_Tail(Ts_Adaptation_field  * ts_adaptation_field);                                    //填写自适应段标志帧尾的
int ts_mux_for_h264_aac( const char *h264_file,const char * aac_file , const char * ts_file );
int PES2TS( FILE *, TsPes * ts_pes,unsigned int Video_Audio_PID ,Ts_Adaptation_field * ts_adaptation_field_Head ,Ts_Adaptation_field * ts_adaptation_field_Tail,
		   unsigned long  Videopts,unsigned long Adudiopts);
int CreateAdaptive_Ts(Ts_Adaptation_field * ts_adaptation_field,unsigned char * buf,unsigned int AdaptiveLength);
#endif