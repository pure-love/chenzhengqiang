/*
@file name:flv_aac.h
@author:chenzhengqiang
@start date:2015/9/9
@modified date:2015/9/13
@desc:
*/


#ifndef _CZQ_FLV_AAC_H_
#define _CZQ_FLV_AAC_H_

static const int ONE_AUDIO_FRAME_SIZE  = 1024*100;
//that parse the flv aac sequence header, you can get those fields like the following
//detailed information see the aac adts format
typedef struct _AAC_ADTS
{
	unsigned char audioObjectType;              
	unsigned char samplingFrequencyIndex;      
	unsigned char channelConfiguration;        
	unsigned char framelengthFlag;              
	unsigned char dependsOnCoreCoder;           
	unsigned char extensionFlag;             
}AAC_ADTS;


//the flv's audio tag
//detailed information about these fields see the adobe document related to flv
typedef struct _FLV_AAC_TAG                           
{
	unsigned char Type ;                       
	unsigned int   DataSize;                  
	unsigned int   Timestamp;              
	unsigned char TimestampExtended;          
	unsigned int  	StreamID;                  
	unsigned char SoundFormat ;               
	unsigned char SoundRate ;                
	unsigned char SoundSize;                 
	unsigned char SoundType;                  
	unsigned char AACPacketType;               
	unsigned char Payload[ONE_AUDIO_FRAME_SIZE]; 
	AAC_ADTS adts;//parsed from the flv aac's sequence header
}FLV_AAC_TAG;

int get_flv_aac_tag( unsigned char *flv_tag_header,unsigned char * flv_tag_data, unsigned int tag_data_size,FLV_AAC_TAG & aac_tag);
#endif
