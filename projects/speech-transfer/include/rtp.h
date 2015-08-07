/*
*@author:chenzhengqiang
*@start-date:2015/6/13
*@modified-date:
*@desc:
*/

#ifndef _RTP_H_
#define _RTP_H_
#include<sys/types.h>
#include<stdint.h>
static const size_t RTP_HEADER_LEAST_SIZE=12;
static const size_t RTP_VERSION=2;
static const size_t RTP_SSRC_SIZE=4;

typedef struct   
{  
     /*big-endian*/
     
    /**//* byte 0 */  
    unsigned char version:2;        
    unsigned char padding:1;        
    unsigned char extension:1;        
    unsigned char csrc_count:4;  
    unsigned char marker:1;    
    /**//* byte 1 */  
    unsigned char payload_type:7;
    /**//* bytes 2, 3 */
    unsigned short extension_length;
    unsigned short offset;	
    uint32_t *csrcs;
    unsigned short sequence_no;              
    /**//* bytes 4-7 */  
    uint32_t timestamp;          
    /**//* bytes 8-11 */  
    uint32_t ssrc;
    	
} RTP_HEADER; 

int rtp_packet_encapsulate(uint8_t *rtp_packet,size_t packet_size,
	const uint8_t *rtp_payload,size_t payload_size, const RTP_HEADER & rtp_header);
int rtp_header_parse( const uint8_t *rtp_packet, size_t size, RTP_HEADER & rtp_header); 
#endif

