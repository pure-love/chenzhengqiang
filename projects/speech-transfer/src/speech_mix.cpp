/*
@author:chenzhengqiang
@start date:2015/8/5
@modified date:
@desc:
*/

#include "speech_mix.h"
#include<cmath>

//mix the pcm_buffer1 and pcm_buffer2's data into pcm_buffer3
void generate_speech_mix( const char *pcm_buffer1,const char *pcm_buffer2,
                                       char *pcm_buffer3,size_t buffer_size )
{
    for( size_t index=0; index < buffer_size; ++index )
    {
        if ((pcm_buffer1[index]< 0) && (pcm_buffer2[index] < 0)) 
        {
            pcm_buffer3[index]= pcm_buffer1[index]+ pcm_buffer2[index]
                                            -
                              (pcm_buffer1[index]* pcm_buffer2[index]/ - (pow(2,16-1)-1));
        }
        else
        {
            pcm_buffer3[index]= pcm_buffer1[index]+ pcm_buffer2[index]
                                            -
                              (pcm_buffer1[index]* pcm_buffer2[index]/(pow(2,16-1)-1));
        }
    }
}
                                                 

