/*
@author:chenzhengqiang
@version:1.0
@start date:2015/8/26
@modified date:
@desc:providing some apis for handling aac file
*/


#ifndef _CZQ_AAC_H_
#define _CZQ_AAC_H_
#include "ts.h"
#include <cstdio>

//each adts header hold 7 bytes
static const int ADTS_HEADER_LENGTH=7;

//the adts header structure
typedef struct
{
	unsigned int syncword;  //it always 0xfff for the sync's sake
	unsigned int id;       //MPEG Version: 0 for MPEG-4, 1 for MPEG-2
	unsigned int layer;    // always: '00'
	unsigned int protection_absent; //the error check field
	unsigned int profile; /* indicates the aac level
	0:main profile
	1:low complexity profile
	2:scalable sampling rate profile
	3:reserved
	*/
	unsigned int sf_index; /* the index of aac sample rates
	
    0: 96000 Hz
    1: 88200 Hz
    2: 64000 Hz
    3: 48000 Hz
    4: 44100 Hz
    5: 32000 Hz
    6: 24000 Hz
    7: 22050 Hz
    8: 16000 Hz
    9: 12000 Hz
    10: 11025 Hz
    11: 8000 Hz
    12: 7350 Hz
    13: Reserved
    14: Reserved
    15: frequency is written explictly
	*/
	unsigned int private_bit; 
	unsigned int channel_configuration; /*
	indicates the channels of aac frame
	
    0: Defined in AOT Specifc Config
    1: 1 channel: front-center
    2: 2 channels: front-left, front-right
    3: 3 channels: front-center, front-left, front-right
    4: 4 channels: front-center, front-left, front-right, back-center
    5: 5 channels: front-center, front-left, front-right, back-left, back-right
    6: 6 channels: front-center, front-left, front-right, back-left, back-right, LFE-channel
    7: 8 channels: front-center, front-left, front-right, side-left, side-right, back-left, back-right, LFE-channel
    8-15: Reserved
	*/
	unsigned int original; 
	unsigned int home;            
	
	unsigned int copyright_identification_bit; 
	unsigned int copyright_identification_start;
	unsigned int aac_frame_length;               // 13 bslbf  adts header length + aac raw stream
	unsigned int adts_buffer_fullness;     
    
	/*no_raw_data_blocks_in_frame 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧.
	所以说number_of_raw_data_blocks_in_frame == 0 
	表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
    */
	unsigned int no_raw_data_blocks_in_frame;
} ADTS_HEADER;


//the aac supported samperate
static const int MAX_AAC_SAMPLERATE_INDEX=12;
static const int AAC_SAMPLERATES[MAX_AAC_SAMPLERATE_INDEX+1]={96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350};

int obtain_aac_adts_header( FILE * faac_handler, ADTS_HEADER & adts_header, unsigned char * adts_header_buffer );
int read_aac_frame( FILE *faac_handler,unsigned char * aac_frame ,unsigned int & frame_length );
void aac_frame_2_pes( unsigned char *aac_frame, unsigned int frame_length, unsigned long aac_pts,TsPes  & aac_pes );
int   obtain_aac_file_samplerate( const char * aac_file );
#endif