/*
@author:chenzhengqiang
@start date:2015/8/25
@modified date:
@desc:aac media file related,obtain the adts header, and read a frame from aac file and so on
*/

#ifndef _CZQ_AAC_H_
#define _CZQ_AAC_H_
#include "ts.h"
#include<cstdio>

static const int ADTS_HEADER_LENGTH = 7;

typedef struct
{
	unsigned int syncword; 
       unsigned int id;
	unsigned int layer;    
	unsigned int protection_absent;
	unsigned int profile; 
	unsigned int sf_index;
	unsigned int private_bit;
	unsigned int channel_configuration;
	unsigned int original;          
	unsigned int home;            

    
	unsigned int copyright_identification_bit;  
	unsigned int copyright_identification_start;
	unsigned int aac_frame_length;               // adts header length + aac frame's raw length
	unsigned int adts_buffer_fullness;           
	/*no_raw_data_blocks_in_frame 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧.
	所以说number_of_raw_data_blocks_in_frame == 0 
	表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
    */
	unsigned int no_raw_data_blocks_in_frame;
} ADTS_HEADER;

int obtain_aac_adts_header(FILE *faac_handler,ADTS_HEADER & adts_header);                
int read_aac_frame( FILE *faac_handler,unsigned char * aac_frame, unsigned int & frame_length );                         
#endif