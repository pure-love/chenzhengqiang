/*
@author:internet
@modified author:chenzhengqiang
@start date:2015/9/9
@modified date:
*/

#ifndef _CZQ_FLV_SCRIPT_H_
#define _CZQ_FLV_SCRIPT_H_

static const int ONE_SCRIPT_FRAME_SIZE = 1024 * 1024;
static const int MAX_ECMAARAY_NAME_LENGH = 100;


//the flv script tag,detailed introduction about these fields like the following
//see the adobe document
typedef struct _FLV_SCRIPT_TAG                         
{
	unsigned char Type ;                      
	unsigned int  DataSize;                    
	unsigned int  Timestamp;               
	unsigned char TimestampExtended;        
	unsigned int  StreamID;                    
	unsigned char Type_1;
	unsigned int  StringLength;                
	unsigned int  ECMAArrayLength;         
	double duration;                             
	double width;							          
	double height;							   
	double videodatarate;					           
	double framerate;						                
	double videocodecid;					   
	double audiosamplerate;					            
	double audiodatarate;					            
	double audiosamplesize;					   
	int    stereo;							        
	double audiocodecid;					    
	double filesize;						          
	double lasttimetamp;			   
	double lastkeyframetimetamp;              
	double filepositions[1000];                
	double times[1000];                      
	unsigned char Data[ONE_SCRIPT_FRAME_SIZE];                    
}FLV_SCRIPT_TAG ;

double char2double(unsigned char * buf,unsigned int size);
void   double2char(unsigned char * buf,double val);
int get_flv_script_tag( unsigned char *flv_tag_header,unsigned char * flv_script_buffer, unsigned int length, FLV_SCRIPT_TAG & script_tag );
#endif