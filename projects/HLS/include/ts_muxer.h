#ifndef _CZQ_TS_MUXER_H_
#define _CZQ_TS_MUXER_H_

#include "ts.h"
#include <cstdio>
extern unsigned char m_One_Frame_Buf[MAX_ONE_FRAME_SIZE];

extern unsigned int WritePacketNum;               //一共的包数目


int Write_Pat(FILE *,unsigned char * buf);
int Write_Pmt(FILE *,unsigned char * buf);
int Take_Out_Pes(PES_PACKET * tspes ,unsigned long time_pts,unsigned int frametype,unsigned int *videoframetype); //0 视频 ，1音频
int WriteAdaptive_flags_Head(Ts_Adaptation_field  * ts_adaptation_field,unsigned int Videopts);              //填写自适应段标志帧头的
int WriteAdaptive_flags_Tail(Ts_Adaptation_field  * ts_adaptation_field);                                    //填写自适应段标志帧尾的
int ts_mux_for_h264_aac(const char *h264_file,const char * aac_file, const char * ts_file );
int aac_frame_2_pes(unsigned char * aac_frame,unsigned int frame_length, 
                                      unsigned long aac_pts,PES_PACKET & aac_pes);
int h264_frame_2_pes( unsigned char * h264_frame,unsigned int frame_length,
                            unsigned int frame_type,unsigned long h264_pts,PES_PACKET & h264_pes);

int PES2TS(FILE *fts_handler,PES_PACKET * ts_pes,unsigned int Video_Audio_PID ,Ts_Adaptation_field * ts_adaptation_field_head ,Ts_Adaptation_field * ts_adaptation_field_tail,
		   unsigned long  Videopts,unsigned long Adudiopts);
int CreateAdaptive_Ts(Ts_Adaptation_field * ts_adaptation_field,unsigned char * buf,unsigned int AdaptiveLength);
#endif