/*
@author:internet
@modified author:chenzhengqiang
@start date:2015/9/9
*/

#ifndef _CZQ_FLV_H_
#define _CZQ_FLV_H_

#include "common.h"



enum{	
PREVIOUS_TAG_SIZE=4,
TAG_HEADER_SIZE=11,
TAG_AUDIO=8,
TAG_VIDEO=9,
TAG_SCRIPT=18,
MEDIA_TYPE_AAC=0x0A,
AAC_SEQUENCE_HEADER_TYPE=0,
FLV_AAC_RAW=1,
FLV_HEADER_SIZE=9
};

typedef struct _FLV_HEADER
{
	unsigned char Signature_1;            
	unsigned char Signature_2;           
	unsigned char Signature_3;            
	unsigned char version  ;                  
	unsigned char TypeFlagsReserved_1;        
	unsigned char TypeFlagsAudio;             
	unsigned char TypeFlagsReserved_2;       
	unsigned char TypeFlagsVideo; 
	unsigned int   DataOffset;                  
	
}FLV_HEADER;


typedef struct _FLV_TAG                    
{
    unsigned char Type ;                     
    unsigned int  DataSize;             
    unsigned int  Timestamp;               
    unsigned char TimestampExtended;      
    unsigned int  StreamID;                
    unsigned char * Data;                  
}FLV_TAG;

int read_flv_header(unsigned char * flv_stream, unsigned int stream_length ,FLV_HEADER & flv_header );
#endif