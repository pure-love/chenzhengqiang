/*
*@author:chenzhengqiang
*@start-date:2015/6/13
*@modified-date:
*@desc:providing the rtp packet's encapsulate and parse
*/
#include "rtp.h"
#include "netutility.h"
#include "logging.h"


/*
*@args:
*@returns:-1 indicates error,0 indicates success
*@desc:encapsulate the data as rtp packet
*/
int rtp_packet_encapsulate(uint8_t *rtp_packet,size_t packet_size,
    const uint8_t *rtp_payload,size_t payload_size, const RTP_HEADER & rtp_header )
{
    size_t total_size = RTP_HEADER_LEAST_SIZE+rtp_header.csrc_count*4;
    if( rtp_header.extension == 1 )
    {
         total_size+=4+rtp_header.extension_length*4;
    }
    
    if( packet_size < total_size )
    {
         //rtp_packet size is too small to encapsulate the data
         return -1;
    }

    memset(rtp_packet,0,packet_size);
    rtp_packet[0]   = rtp_header.version << 6;
    rtp_packet[0] |= rtp_header.padding << 5;
    rtp_packet[0] |= rtp_header.extension << 4;
    rtp_packet[0] |= rtp_header.csrc_count;
    rtp_packet[1] |= rtp_header.marker << 7;
    rtp_packet[1] |= rtp_header.payload_type;

    if(sizeof(rtp_header.sequence_no) != 2)
    {
        return -1;
    }
    
    memset( rtp_packet+2, rtp_header.sequence_no, sizeof( rtp_header.sequence_no ) );
    memset( rtp_packet+4, rtp_header.timestamp , sizeof( rtp_header.timestamp ) );
    memset( rtp_packet+8, rtp_header.ssrc, sizeof(rtp_header.ssrc) );
    
    int offset=RTP_HEADER_LEAST_SIZE;
    for( int index = 0; index < rtp_header.csrc_count; ++index )
    {
        memset(rtp_packet+offset,rtp_header.csrcs[index],RTP_SSRC_SIZE);
        offset+=RTP_SSRC_SIZE;
    }
    
    if( rtp_header.extension == 1 )
    {
        rtp_packet[offset]=rtp_header.extension_length;
        offset+=4+rtp_header.extension_length*4;
    }

    if( rtp_payload != 0 && payload_size > 0 )
    {
        memcpy( rtp_packet+offset,rtp_payload,payload_size );
    }
    return 0;
    
}



 /*
 *@args:
 *@returns:-1 indicates error,0 indicates success
 *@desc:parse the rtp packet's header and store those fields into self-defined RTP_HEADER
 */
 int rtp_header_parse( const uint8_t *rtp_packet, size_t size, RTP_HEADER & rtp_header) 
{  
    if( size < RTP_HEADER_LEAST_SIZE ) 
    {  
        //invalid arguments,it must be 12 at least  
        log_module(LOG_DEBUG,"RTP_PACKET_PARSE","INVALID ARGUMENTS@SIZE:%s",LOG_LOCATION);
        return -1;  
    }  

    //firstly check if it's the right rtp version
    if ((rtp_packet[0] >> 6) != (int) RTP_VERSION ) 
    {    
        //invalid rtp version
        log_module(LOG_DEBUG,"RTP_PACKET_PARSE","INVALID RTP VERSION:%d THE RIGHT VERSION IS %d;%s",
        rtp_packet[0]>>6,RTP_VERSION,LOG_LOCATION);
        return -1;  
    }  

    rtp_header.version = rtp_packet[0] >> 6;
    //now check if the padding bit is set
    if (rtp_packet[0] & 0x20) 
   {  
        //paddind bit is present
        rtp_header.padding = 1;
        size_t padding_size = rtp_packet[size - 1];  
        if (padding_size + RTP_HEADER_LEAST_SIZE > size )
        {  
            log_module(LOG_DEBUG,"RTP_PACKET_PARSE","INVALID ARGUMENTS@SIZE:%s",LOG_LOCATION);
            return -1;  
        }  
        size -= padding_size;  
    }  
  
    rtp_header.csrc_count = rtp_packet[0] & 0x0f;  
    rtp_header.offset = 12 + 4 * rtp_header.csrc_count;  
  
    if ( size < rtp_header.offset ) 
    {  
        // not enough data to fit the basic header and all the CSRC entries.
        log_module(LOG_DEBUG,"RTP_PACKET_PARSE","INVALID ARGUMENTS@SIZE:%s",LOG_LOCATION);
        return -1;  
    }  
  
    if ( rtp_packet[0] & 0x10 ) 
    {  
        // Header extension is present.  
        if (size < (size_t)rtp_header.offset + 4)
        {  
            // not enough data to fit the basic header, all CSRC entries and the first 4 bytes of the extension header.
            log_module(LOG_DEBUG,"RTP_PACKET_PARSE","INVALID ARGUMENTS@SIZE:%s",LOG_LOCATION);
            return -1;  
        }  
  
        const uint8_t *extension_data = &rtp_packet[rtp_header.offset];  
        rtp_header.extension_length = 4 * (extension_data[2] << 8 | extension_data[3]);  
        if (size < (size_t)(rtp_header.offset+ 4 + rtp_header.extension_length))
        {  
            log_module(LOG_DEBUG,"RTP_PACKET_PARSE","INVALID ARGUMENTS@SIZE:%s",LOG_LOCATION);
            return -1;  
        }  
        rtp_header.offset += (4 + rtp_header.extension_length);  
    }
    
    rtp_header.payload_type = rtp_packet[1]&0x7f;
    rtp_header.sequence_no = rtp_packet[2] << 8 | rtp_packet[3];  
    rtp_header.timestamp = rtp_packet[4] << 24 | rtp_packet[5] << 16 | rtp_packet[6] << 8 | rtp_packet[7];  
    rtp_header.ssrc = rtp_packet[8] << 24 | rtp_packet[9] << 16 | rtp_packet[10] << 8 | rtp_packet[11];  
  
    return 0;  
}  

