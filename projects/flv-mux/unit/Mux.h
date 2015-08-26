#ifndef _CZQ_MUX_H_
#define _CZQ_MUX_H_
#include "AudioTag.h"
#include "VideoTag.h"
#include "ScriptTag.h"

extern unsigned char m_File_Header[FILE_HEADER_LENGTH];
extern unsigned char Video_Tag_Buf[MAX_VIDEO_TAG_BUF_SIZE];
extern unsigned char Audio_Tag_Buf[MAX_AUDIO_TAG_BUF_SIZE];
extern unsigned char Script_Tag_Buf[MAX_SCRIPT_TAG_BUF_SIZE];

int Write_File_Header(unsigned char * buf);
int Write_Audio_Tag(unsigned char * Buf,unsigned int  Timestamp,unsigned char AACPacketType/*AAC序列头部*/);
int Wtire_Video_Tag(unsigned char * buf,unsigned int  Timestamp,unsigned char AACPacketType/*AAC序列头部*/,unsigned int * video_frame_type);
int Write_Script_Tag(unsigned char * buf,double duration,double width,double height,double framerate,double audiosamplerate,int stereo,double filesize,
					 unsigned int filepositions_times_length ,double * filepositions,double *times);
int WriteBuf2File(double width ,double height,double framerate);
#endif