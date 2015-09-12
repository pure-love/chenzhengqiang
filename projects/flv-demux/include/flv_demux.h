/*
@author:chenzhengqiang
@start date:2015/9/9
@modified date:
*/

#ifndef _CZQ_FLV_DEMUX_H_
#define _CZQ_FLV_DEMUX_H_

#include "h264.h"
#include "aac.h"
#include "script.h"

static const int  FLV_FRAME_SIZE=1024*1024;
static const int  ADTS_HEADER_SIZE = 7;
static const int  MAX_FRAME_HEAD_SIZE = 1024;

int read_flv_tag_frame( FILE * fflv_handler, FILE *f264_handler, FILE * faac_handler );
int read_flv_h264_frame(unsigned char * buf, unsigned int size,unsigned char *spsbuffer,unsigned int spslength,unsigned char * ppsbuffer,unsigned int  ppslength ,unsigned int IsVideo_I_Frame);
int read_flv_aac_frame(unsigned char * buf, unsigned int size,bool is_aac_frame,unsigned char audioObjectType,unsigned char samplerate,unsigned char channelcount);
void flv_demux_2_h264_aac( const char *flv_file, const char *h264_file, const char * aac_file );
#endif
