/*
@author:chenzhengqiang
@start date:2015/8/5
@modified date:
@desc:algorithm of pcm data's mix
*/
#ifndef _CZQ_SPEECH_MIX_H_
#define _CZQ_SPEECH_MIX_H_
#include<stdint.h>
#include<sys/types.h>

//mix the pcm_buffer1 and pcm_buffer2's data into pcm_buffer3
void generate_speech_mix( const char *pcm_buffer1,const char *pcm_buffer2,
                                                 char *pcm_buffer3,size_t buffer_size);
#endif
