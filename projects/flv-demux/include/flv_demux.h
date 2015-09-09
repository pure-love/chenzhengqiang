/*
@author:internet
@modified author:chenzhengqiang
@start date:2015/9/9
@modified date:
*/

#ifndef _CZQ_FLV_DEMUX_H_
#define _CZQ_FLV_DEMUX_H_

#include "h264.h"
#include "aac.h"
#include "script.h"

#define  ONE_FRAME_SIZE        1024*1024
#define  ADTS_HEADER_LENGTH    7
#define  MAX_FRAME_HEAD_SIZE   1024

extern File_Header * m_fileheadr;
extern Audio_Tag   * m_audio_tag;
extern Video_Tag   * m_video_tag;
extern Script_Tag  * m_script_tag;

int AllocStruct();
int FreeStruct();
int ReadStruct( FILE * fflv_handler, FILE *f264_handler, FILE * faac_handler );
int And_Head_H264(unsigned char * buf, unsigned int size,unsigned char *spsbuffer,unsigned int spslength,unsigned char * ppsbuffer,unsigned int  ppslength ,unsigned int IsVideo_I_Frame);
int And_Head_Aac(unsigned char * buf, unsigned int size,unsigned int IsAacFrame,unsigned char audioObjectType,unsigned char samplerate,unsigned char channelcount);
void flv_demux_2_h264_aac( const char *flv_file, const char *h264_file, const char * aac_file );
#endif
