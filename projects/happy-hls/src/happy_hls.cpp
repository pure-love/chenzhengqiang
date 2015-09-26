/*
@file name:happy_hls.h
@author:chenzhengqiang
@start date:2015/9/16
@modified date
@desc:
    receive the stream pushed from streamer'server,parse the flv stream tag by tag
    ,meanhilw find the aac and h264's es stream; when you get just mux them into ts file
    per 10 second for the HLS protocol's sake

    all rights reserved by chenzhengqiang.
*/
#include "errors.h"
#include "common.h"
#include "m3u8.h"
#include "flv.h"
#include "flv_aac.h"
#include "flv_avc.h"
#include "flv_script.h"
#include "ts_muxer.h"
#include "happy_hls.h"
#include "logging.h"
#include <dirent.h>
using std::queue;

static const int NO = 0;
static const int AAC_SAMPLERATES[13]={96000,88200,
64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350};
static const int H264_FRAME_RATE = 30;
static const int FIXED_REQUESTED_BUFFER_SIZE = 1024;
static const int ONLINE=0;
static const int OFFLINE=1;
static const int STREAM_SERVER_PORT = 54321;
static const char *NGINX_ROOT="/share/";

struct TS_PES_FRAME
{
    unsigned char *frame_buffer;
    unsigned long frame_length;
    bool is_key_frame;
    unsigned long pts;
};

struct STREAMER_REQUEST
{
    std::string method;
    std::string path;
    std::string channel;
    std::string token;
    int status;
};

struct VIEWER
{
    struct ev_io *receive_stream_watcher;
    unsigned char flv_header[FLV_HEADER_SIZE];
    size_t flv_header_read_bytes;
    unsigned char  previous_tag_size_buffer[PREVIOUS_TAG_SIZE];
    size_t previous_tag_size_read_bytes;
    unsigned char flv_tag_header[TAG_HEADER_SIZE];
    size_t flv_tag_header_read_bytes;
    unsigned char  *flv_tag_data;
    size_t flv_tag_data_read_bytes;
    size_t flv_tag_data_total_bytes;
    std::list<std::string> ts_urls_pool;
};

typedef std::string CHANNEL;
std::map<CHANNEL,VIEWER> CHANNELS_POOL;

void parse_streamer_request(const char * request, size_t buffer_size, STREAMER_REQUEST & streamer_request )
{
    log_module( LOG_DEBUG, "PARSE_STREAMER_REQUEST","+++++++++++++++START+++++++++++++++");
    ( void ) buffer_size;
    streamer_request.status = -1;  
    while( *request && *request == ' ' )
    {
        ++request;
    }
    
    if( ! *request )
        return;

    std::string http_header(request);
    size_t prev_pos=0,cur_pos=0;
    cur_pos = http_header.find_first_of(' ',prev_pos);
    if( cur_pos == std::string::npos )
    return;
    streamer_request.method = http_header.substr(prev_pos,cur_pos);
    log_module( LOG_DEBUG, "PARSE_STREAMER_REQUEST", "METHOD:%s",streamer_request.method.c_str() );
    prev_pos = cur_pos;
    cur_pos = http_header.find_first_of('?',prev_pos);
    if( cur_pos == std::string::npos )
    return;    

    streamer_request.path = http_header.substr(prev_pos,cur_pos-prev_pos);
    log_module( LOG_DEBUG, "PARSE_STREAMER_REQUEST", "PATH:%s",streamer_request.path.c_str() );
    prev_pos = cur_pos+1;

    cur_pos = http_header.find_first_of('=',prev_pos);
    std::string key;
    std::string value;
    
    while( cur_pos != std::string::npos )
    {
        key="";
        value="";
        key = http_header.substr(prev_pos,cur_pos-prev_pos);
        prev_pos=cur_pos+1;
        cur_pos = http_header.find_first_of('&',prev_pos);
        if( cur_pos == std::string::npos )
        {
            cur_pos = http_header.find_first_of(' ', prev_pos );
            if( cur_pos == std::string::npos )
            return;
        }
        value = http_header.substr( prev_pos, cur_pos-prev_pos);
        log_module( LOG_DEBUG, "PARSE_STREAMER_REQUEST","KEY:%s VALUE:%s",key.c_str(),value.c_str() );
        if( key == "channel" )
        {
            streamer_request.channel=value;
            log_module( LOG_DEBUG, "PARSE_STREAMER_REQUEST","CHANNEL:%s",value.c_str());
        }
        else if( key == "status" )
        {
            streamer_request.status = atoi(value.c_str());
            log_module( LOG_DEBUG, "PARSE_STREAMER_REQUEST","STATUS:%s",value.c_str());
        }
        else if( key == "token" )
        {
            streamer_request.token = value;
            log_module( LOG_DEBUG, "PARSE_STREAMER_REQUEST","TOKEN:%s",value.c_str());
        }
        
        prev_pos=cur_pos+1;
        cur_pos = http_header.find_first_of('=',prev_pos);
    }
    log_module( LOG_DEBUG, "PARSE_STREAMER_REQUEST","+++++++++++++++DONE+++++++++++++++");
}


void receive_stream_cb( struct ev_loop * main_event_loop, struct  ev_io *receive_stream_watcher, int revents )
{
       #define DELETE_VIEWER_IF(X) \
       if( (X) <= 0 )\
       {\
            close( receive_stream_watcher->fd );\
            ev_io_stop( main_event_loop, receive_stream_watcher );\
            if( viewer_iter->second.receive_stream_watcher != NULL )\
            delete viewer_iter->second.receive_stream_watcher;\
            if( viewer_iter->second.flv_tag_data != NULL )\
            delete [] viewer_iter->second.flv_tag_data;\
            CHANNELS_POOL.erase( viewer_iter );\
            if( fts_handler )\
            fclose( fts_handler );\
            return;\
       }
       
       if( EV_ERROR & revents )
       {
            log_module(LOG_INFO,"RECEIVE_STREAM_CB","ERROR FOR EV_ERROR,JUST TRY AGAIN");
            return;
       }

        static bool the_first = true;
        static bool read_http_reply_first = true;
        int read_bytes = 0;
        char *channel = static_cast<char *>( receive_stream_watcher->data );
        if( channel == NULL )
        {
            log_module( LOG_INFO, "RECEIVE_STREAM_CB", "THE CHANNEL DATA IS NULL SAVED BY RECEIVE_STREAM_WATCHER,JUST STOP THE WATCHER AND CLOSE THE SOCKET");
            ev_io_stop( main_event_loop, receive_stream_watcher );
            close( receive_stream_watcher->fd);
            delete receive_stream_watcher;
            return;
        }
    
        std::map<CHANNEL,VIEWER>::iterator viewer_iter = CHANNELS_POOL.find(std::string(channel));
        if( viewer_iter == CHANNELS_POOL.end() )
        {
            log_module( LOG_DEBUG, "RECEIVE_STREAM_CB", "THE CHANNEL %s NOT EXIST IN CHANNELS POOL,JUST STOP THE WATCHER EITHER CLOSE THE SOCKET" );
            ev_io_stop( main_event_loop, receive_stream_watcher );
            close( receive_stream_watcher->fd);
            delete receive_stream_watcher;
        }
    
        
        //HLS related initialization
        static FILE *fts_handler = NULL ;
        static bool first_create_ts = false;
        static M3U8_CONFIG m3u8_config;
        static unsigned char  sps_buffer[MAX_FRAME_HEAD_SIZE];
        static unsigned char  pps_buffer[MAX_FRAME_HEAD_SIZE];
        static unsigned int    sps_length = 0;
        static unsigned int    pps_length = 0;
        static unsigned long h264_pts = 0;
        static unsigned long aac_pts = 0;
        static queue<TS_PES_FRAME> aac_es_queue;
        static queue<TS_PES_FRAME> avc_es_queue;
        TS_PES_FRAME es_frame;

        static time_t prev; 
        static time_t now;
        std::string ts_url;
        
        if( the_first )
        {
            if( read_http_reply_first )
            {
                log_module( LOG_DEBUG, "RECEIVE_STREAM_CB", "READ THE HTTP REPLY FIRST FROM STREAM SERVER RELATED TO CHANNEL:%s",channel);
                char server_reply[1024];
                read_bytes = read_http_header(receive_stream_watcher->fd, server_reply, sizeof(server_reply) );
                DELETE_VIEWER_IF(read_bytes);
                server_reply[read_bytes-1]='\0';
                read_http_reply_first = false; 
                log_module( LOG_DEBUG, "RECEIVE_STREAM_CB","THE REPLY FROM STREAM SERVER IS:%s READ BYTES:%d",
                                                  server_reply, read_bytes );
            }

            if( viewer_iter->second.flv_header_read_bytes < FLV_HEADER_SIZE )
            { 
	            read_bytes = read_specify_size( receive_stream_watcher->fd,
                                                         viewer_iter->second.flv_header+viewer_iter->second.flv_header_read_bytes,
                                                         FLV_HEADER_SIZE-viewer_iter->second.flv_header_read_bytes);
                   DELETE_VIEWER_IF(read_bytes);
                   viewer_iter->second.flv_header_read_bytes+=(size_t)read_bytes;
                   if( viewer_iter->second.flv_header_read_bytes < FLV_HEADER_SIZE )
                   return;
                   else if( viewer_iter->second.flv_header_read_bytes == FLV_HEADER_SIZE )
                   {
                        the_first = false;
                        m3u8_config.path=std::string(NGINX_ROOT) + std::string(channel)+"/";
                        DIR *dirp = opendir( m3u8_config.path.c_str() );
                        if( dirp == NULL )
                        {
                            int ret = mkdir( m3u8_config.path.c_str(), 0755 );
                            if( ret == -1 )
                            {
                                log_module( LOG_ERROR, "RECEIVE_STREAM_CB", "FAILED TO CREATE DIRECTORY %s ERROR:%s",
                                                                                                    m3u8_config.path.c_str(),strerror(errno) );
                                DELETE_VIEWER_IF(0);
                            }
                        }
                        else
                        closedir(dirp);
                        m3u8_config.channel=channel;
                        m3u8_config.timestamp=time(NULL);
                        prev=now=m3u8_config.timestamp;
                        m3u8_config.media_prev_sequence = 1;
                        m3u8_config.media_cur_sequence = 1;
                        m3u8_config.target_duration = 15;
                        m3u8_config.playlist_type = 1;
                        m3u8_config.version = 3;
                        m3u8_config.total_ts_urls = 4;
                        fts_handler = create_ts_file( ts_url, m3u8_config );
                        viewer_iter->second.ts_urls_pool.push_back(ts_url);
                        log_module( LOG_DEBUG, "RECEIVE_STREAM_CB", "READ FLV HEADER DONE");
                        
                   }
                   else
                   {
                        log_module( LOG_INFO, "RECEIVE_STREAM_CB", "OVERFLOW WHEN READ THE FLV HEADER RELATED TO CHANNEL %s",channel);
                        DELETE_VIEWER_IF(0);
                   }
            }

         
       }
       
	now=time( NULL );
       static bool init_first = true;
       if( now - prev >= (int)(m3u8_config.target_duration) )
       {
            if( init_first )
            {
                 init_first = false;
                 //fflv_handler = fopen( (m3u8_config.path+channel+".flv").c_str(), "wb" );
                 //fwrite( viewer_iter->second.flv_header,sizeof(unsigned char),FLV_HEADER_SIZE,fflv_handler);
                 log_module( LOG_DEBUG, "RECEIVE_STREAM_CB","CREATE THE M3U8 FILE RELATED TO CHANNEL %s",channel);
                 std::string m3u8_file;
                 create_m3u8_file(  m3u8_config, m3u8_file );
                 log_module( LOG_DEBUG, "RECEIVE_STREAM_CB","M3U8 FILE CREATED:%s RELATED TO CHANNEL %s",m3u8_file.c_str(),channel );
                 first_create_ts = true;
                 add_ts_url_2_m3u8_file( viewer_iter->second.ts_urls_pool, m3u8_config );
            }
            
            log_module( LOG_DEBUG, "RECEIVE_STREAM_CB", "%d SECONDS HAS GONE BY:RECREATE THE TS FILE MEANWHILE UPDATE THE M3U8 FILE",m3u8_config.target_duration );
            prev = now;
            m3u8_config.timestamp = now;
            m3u8_config.media_cur_sequence+=1;
            fclose( fts_handler );
            fts_handler = create_ts_file( ts_url, m3u8_config );
            first_create_ts = true;
            h264_pts = 0;
            aac_pts = 0;
            viewer_iter->second.ts_urls_pool.push_back( ts_url );
            add_ts_url_2_m3u8_file( viewer_iter->second.ts_urls_pool , m3u8_config );
       }

	 //flv file consists of PreviousTagSize(4 bytes)+tag()
	 //read the previous tag size first,it holds 4 bytes
	 if( viewer_iter->second.previous_tag_size_read_bytes < PREVIOUS_TAG_SIZE )
       {   
             read_bytes = read_specify_size( receive_stream_watcher->fd,
                                                           viewer_iter->second.previous_tag_size_buffer+viewer_iter->second.previous_tag_size_read_bytes,
                                                           PREVIOUS_TAG_SIZE-viewer_iter->second.previous_tag_size_read_bytes);
	     DELETE_VIEWER_IF(read_bytes);
             viewer_iter->second.previous_tag_size_read_bytes+= read_bytes;
             if( viewer_iter->second.previous_tag_size_read_bytes != PREVIOUS_TAG_SIZE )
             return;
             //fwrite( viewer_iter->second.previous_tag_size_buffer,sizeof(unsigned char), PREVIOUS_TAG_SIZE, fflv_handler );
       }     

       log_module( LOG_DEBUG, "RECEIVE_STREAM_CB", "PREVIOUS TAG SIZE READ DONE");
       //read the tag header,it always hold 11 bytes
       if( viewer_iter->second.flv_tag_header_read_bytes < TAG_HEADER_SIZE )
       {
             read_bytes = read_specify_size( receive_stream_watcher->fd, 
                                                           viewer_iter->second.flv_tag_header+viewer_iter->second.flv_tag_header_read_bytes,
                                                           TAG_HEADER_SIZE - viewer_iter->second.flv_tag_header_read_bytes );
             
		DELETE_VIEWER_IF(read_bytes);
             viewer_iter->second.flv_tag_header_read_bytes+=read_bytes;
             if( viewer_iter->second.flv_tag_header_read_bytes != TAG_HEADER_SIZE )
             return; 
             viewer_iter->second.flv_tag_data_total_bytes = viewer_iter->second.flv_tag_header[1]  << 16 |viewer_iter->second.flv_tag_header[2]  << 8  | viewer_iter->second.flv_tag_header[3];
             viewer_iter->second.flv_tag_data = new unsigned char[viewer_iter->second.flv_tag_data_total_bytes];
             if( viewer_iter->second.flv_tag_data == NULL )
             DELETE_VIEWER_IF(0);
             //fwrite( viewer_iter->second.flv_tag_header,sizeof(unsigned char), TAG_HEADER_SIZE, fflv_handler );
       }
       
	log_module( LOG_DEBUG, "RECEIVE_STREAM_CB", "FLV TAG HEADER READ DONE");
       //now read the flv tag data for each media type
       if( viewer_iter->second.flv_tag_data_read_bytes != viewer_iter->second.flv_tag_data_total_bytes )
       {
            read_bytes = read_specify_size( receive_stream_watcher->fd,
                                                          viewer_iter->second.flv_tag_data+viewer_iter->second.flv_tag_data_read_bytes,
                                                          viewer_iter->second.flv_tag_data_total_bytes - viewer_iter->second.flv_tag_data_read_bytes );
            DELETE_VIEWER_IF( read_bytes );
            viewer_iter->second.flv_tag_data_read_bytes += read_bytes;
            if( viewer_iter->second.flv_tag_data_read_bytes != viewer_iter->second.flv_tag_data_total_bytes )
            return;
            //fwrite( viewer_iter->second.flv_tag_data,sizeof(unsigned char), viewer_iter->second.flv_tag_data_total_bytes, fflv_handler );
       }
        
       log_module( LOG_DEBUG, "RECEIVE_STREAM_CB", "FLV TAG DATA READ DONE READ BYTES:%d",(int)viewer_iter->second.flv_tag_data_total_bytes );
       unsigned char tag_type = viewer_iter->second.flv_tag_header[0];
       static FLV_AAC_TAG aac_tag;
       static FLV_H264_TAG h264_tag;
	int    aac_tag_payload_size = 0;
	int    avc_tag_payload_size = 0;

       switch ( tag_type )
       {
            case TAG_AUDIO: 
                   log_module( LOG_DEBUG, "RECEIVE_STREAM_CB", "NOW HANDLE THE FLV AUDIO TAG");
	            aac_tag_payload_size = get_flv_aac_tag( viewer_iter->second.flv_tag_header,
                                                                              viewer_iter->second.flv_tag_data,
                                                                              viewer_iter->second.flv_tag_data_total_bytes,aac_tag);
                   
		      if ( aac_tag_payload_size != -1 ) 
			{
			       
				if( aac_tag.AACPacketType == FLV_AAC_RAW_TYPE )
				{
	                           unsigned char  adts_header_buffer[ADTS_HEADER_SIZE] ;
	                           unsigned int  aac_frame_length = aac_tag_payload_size + ADTS_HEADER_SIZE;
	                           adts_header_buffer[0] = 0xFF;
	                           adts_header_buffer[1] = 0xF1;
	                           adts_header_buffer[2] = (0x01 << 6)  | (aac_tag.adts.samplingFrequencyIndex<< 2)  | (aac_tag.adts.channelConfiguration >> 7);
	                           adts_header_buffer[3] = ( aac_tag.adts.channelConfiguration << 6) |  0x00 | 0x00 | 0x00 |0x00 | ((aac_frame_length &  0x1800) >> 11);
	                           adts_header_buffer[4] = ( aac_frame_length & 0x7F8) >> 3;
	                           adts_header_buffer[5] = ( aac_frame_length & 0x7) << 5  |  0x1F;
	                           adts_header_buffer[6] = 0xFC  | 0x00;
	                           
                                  es_frame.frame_length = ADTS_HEADER_SIZE+aac_tag_payload_size;
                                  es_frame.frame_buffer = new unsigned char[es_frame.frame_length];
                                  es_frame.is_key_frame = false;
                                  memcpy(es_frame.frame_buffer,adts_header_buffer,ADTS_HEADER_SIZE);
                                  memcpy(es_frame.frame_buffer+ADTS_HEADER_SIZE,(char *)aac_tag.Payload,aac_tag_payload_size);
                                  aac_es_queue.push(es_frame);
				}
			}
                    else
                    {
                        log_module( LOG_DEBUG, "RECEIVE_STREAM_CB", "FAILED TO GET THE FLV AAC TAG FROM FLV AUDIO TAG");
                    }
			break;
		    case TAG_VIDEO:
                    log_module( LOG_DEBUG, "RECEIVE_STREAM_CB","NOW HANDLE THE FLV VIDEO TAG" );
			avc_tag_payload_size = get_flv_h264_tag( viewer_iter->second.flv_tag_header,
                                                                                 viewer_iter->second.flv_tag_data,
                                                                                 viewer_iter->second.flv_tag_data_total_bytes,h264_tag);
			if ( avc_tag_payload_size != -1 ) 
			{
				if ( h264_tag.AVCPacketType == AVC_SEQUENCE_HEADER_TYPE )
				{
				       log_module( LOG_DEBUG, "RECEIVE_STREAM_CB", "FIND THE AVC SEQUENCE HEADER" );  
					sps_length = h264_tag.adcr.sequenceParameterSetLength;
					pps_length = h264_tag.adcr.pictureParameterSetLength;
					memcpy(sps_buffer,h264_tag.adcr.sequenceParameterSetNALUnit,sps_length);
					memcpy(pps_buffer,h264_tag.adcr.pictureParameterSetNALUnit,pps_length);
				}
				else if( h264_tag.AVCPacketType == FLV_AVC_NALU_TYPE ) 
				{
					
                                 unsigned char * h264_stream_buffer = (unsigned char * )calloc(avc_tag_payload_size+ 1024,sizeof(char));
	                          unsigned char strcode[4];             
	                          unsigned int sei_length = 0;  
	                          
                                 if ( h264_tag.Payload[4] == 0x06 ) 
	                          {
		                        sei_length = h264_tag.Payload[2]  << 8  | h264_tag.Payload[3];
		                        h264_stream_buffer[4 + sei_length]      = 0x00;
		                        h264_stream_buffer[4 + sei_length +1] = 0x00;
		                        h264_stream_buffer[4 + sei_length +2] = 0x00;
		                        h264_stream_buffer[4 + sei_length +3] = 0x01;
	                          }
	                          else
	                          {
		                        memcpy(h264_stream_buffer,h264_tag.Payload,avc_tag_payload_size);
	                          }
    
	                          if ( h264_tag.FrameType == FLV_KEY_FRAME )
	                          {
		                        strcode[0] = 0x00;
		                        strcode[1] = 0x00;
		                        strcode[2] = 0x00;
		                        strcode[3] = 0x01;
                                       
                                     es_frame.frame_length = 4+sps_length;
                                     es_frame.frame_buffer = new unsigned char[es_frame.frame_length];
                                     es_frame.is_key_frame = true;
                                     memcpy(es_frame.frame_buffer,strcode,4);
                                     memcpy(es_frame.frame_buffer+4,sps_buffer,sps_length);
                                     avc_es_queue.push(es_frame);
                                     es_frame.frame_length = 4+pps_length;
                                     es_frame.frame_buffer = new unsigned char[es_frame.frame_length];
                                     es_frame.is_key_frame = true;
                                     memcpy( es_frame.frame_buffer,strcode,4);
                                     memcpy( es_frame.frame_buffer+4,pps_buffer,pps_length );
                                     avc_es_queue.push( es_frame );
	                          }
    
	                          h264_stream_buffer[0] = 0x00;
	                          h264_stream_buffer[1] = 0x00;
	                          h264_stream_buffer[2] = 0x00;
	                          h264_stream_buffer[3] = 0x01;
	                          
                                es_frame.frame_length = avc_tag_payload_size;
                                es_frame.frame_buffer = new unsigned char[es_frame.frame_length];
                                memcpy(es_frame.frame_buffer,h264_stream_buffer,avc_tag_payload_size);
                                avc_es_queue.push( es_frame );  
	                         if ( h264_stream_buffer )
	                         {
		                        free( h264_stream_buffer );
		                        h264_stream_buffer= NULL;
	                         }
                           }
                     }
                     else
                     {
                        log_module( LOG_DEBUG, "RECEIVE_STREAM_CB", "FAILED TO GET FLV AVC TAG FROM FLV VIDEO TAG");
                     }
			break;
		    case TAG_SCRIPT:
			//get_flv_script_tag( flv_tag_header,flv_tag_data,tag_data_size,script_tag);
			break;
		    default:
			;
		}

       Ts_Adaptation_field  ts_adaptation_field_head; 
	Ts_Adaptation_field  ts_adaptation_field_tail;

       if( first_create_ts )
       {
            es_frame = avc_es_queue.front();
            if( es_frame.is_key_frame )
            {
                first_create_ts = false;
                aac_pts = 0;
                h264_pts = 0;
            }
            else
            {
                prev = time( NULL );
                delete [] es_frame.frame_buffer;
                avc_es_queue.pop();
            }
       }

       if( !first_create_ts )
       {
            if( aac_pts < h264_pts && ! aac_es_queue.empty() )
            {
                es_frame = aac_es_queue.front();    
                TsPes aac_pes;
                aac_frame_2_pes(es_frame.frame_buffer,es_frame.frame_length,aac_pts,aac_pes);

                if ( aac_pes.Pes_Packet_Length_Beyond != 0 )
                {
                    write_adaptive_tail_fields(&ts_adaptation_field_head);
                    write_adaptive_tail_fields(&ts_adaptation_field_tail); 
                    pes_2_ts(fts_handler,&aac_pes,TS_AAC_PID ,&ts_adaptation_field_head ,&ts_adaptation_field_tail,h264_pts,aac_pts);
                    aac_pts += 1024*1000* 90/ AAC_SAMPLERATES[aac_tag.adts.samplingFrequencyIndex];
                }

                delete [] es_frame.frame_buffer;
                aac_es_queue.pop(); 
            }
            else if( !avc_es_queue.empty() )
            {
                es_frame = avc_es_queue.front();           
                TsPes h264_pes;
                h264_frame_2_pes( es_frame.frame_buffer,es_frame.frame_length,h264_pts,h264_pes);
                if ( h264_pes.Pes_Packet_Length_Beyond != 0 )
                {
                    write_adaptive_head_fields( &ts_adaptation_field_head,h264_pts ); 
                    write_adaptive_tail_fields( &ts_adaptation_field_tail ); 
                    pes_2_ts( fts_handler,&h264_pes,TS_H264_PID ,&ts_adaptation_field_head ,&ts_adaptation_field_tail,h264_pts,aac_pts);
                    h264_pts += 1000* 90 / H264_FRAME_RATE;   //90khz
                }
                delete [] es_frame.frame_buffer;
                avc_es_queue.pop(); 
            }
       }
       delete [] viewer_iter->second.flv_tag_data;
       viewer_iter->second.flv_tag_data = NULL;
       viewer_iter->second.flv_tag_data_read_bytes = 0;
       viewer_iter->second.flv_tag_data_total_bytes = 0;
       viewer_iter->second.flv_tag_header_read_bytes = 0;
       viewer_iter->second.previous_tag_size_read_bytes = 0;
       ev_io_stop( main_event_loop, receive_stream_watcher );
}



/*
*@args:config[in]
*@returns:void
*@desc:as the function name described,print the welcome info when 
  the software not run as daemon
*/
void print_welcome( const CONFIG & config )
{
    char heading[1024];
    snprintf(heading,1024,"\n" \
                          "\r    *\n"\
                          "\r  *   *  *      *     *  *     *     *  *       *\n" \
                          "\r *        *     *    *    *    *    *   *       *\n"\
                          "\r   *       *    *   *      *   *   *     *      *\n"\
                          "\r      *      * * * *        * * * *       * *  *\n"\
                          "\r  *   *       *   *          *   *             *\n"\
                          "\r    *                                         *\n"\
                          "\r                                            *"\
                          "\n\nsoftware version 1.7.1, Copyleft (c) 2015 SWWY\n" \
                          "this happy hls project started on 2015/9/16 with gcc 4.4.7 in CentOS\n" \
                          "programed by chenzhengqiang,based on tcp protocol, which registered at port:%d\n\n" \
                          "now happly hls server start listening :......\n\n\n\n\n",config.PORT);

    std::cout<<heading<<std::endl;
}


void generate_viewer_request( STREAMER_REQUEST & streamer_request, std::string & viewer_request )
{
    viewer_request="GET swwy/channel=";
    viewer_request+=streamer_request.channel+"&";
    viewer_request+="type=flv&token="+streamer_request.token+" HTTP/1.1\r\n\r\n";     
}


//the kernel of happy hls
void serve_forever( ssize_t hls_listen_fd, const CONFIG & config )
{
    //if run_as_daemon is true,then make this speech transfer server run as daemon
    if( config.run_as_daemon )
    {
        daemon(0,0);
    }
    else
    {
        print_welcome(config);
    }

    logging_init( config.log_file.c_str(), config.log_level );    
    //you have to ignore the PIPE's signal when client close the socket
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;//just ignore the sigpipe
    sa.sa_flags = 0;
    if( sigemptyset(&sa.sa_mask) == -1 ||sigaction(SIGPIPE, &sa, 0) == -1 )
    { 
        logging_deinit();  
        log_module(LOG_ERROR,"SERVE_FOREVER","FAILED TO IGNORE SIGPIPE SIGNAL");
    }

    sdk_set_nonblocking( hls_listen_fd );
    int REUSEADDR_OK=1;
    setsockopt(hls_listen_fd,SOL_SOCKET,SO_REUSEADDR,&REUSEADDR_OK,sizeof(REUSEADDR_OK));
    
    if( config.ev_loop == DEFAULT )
    {
          log_module(LOG_DEBUG,"SERVE_FOREVER","SERVER STARTUP DEFAULT EVENT LOOP");
    }
    else if( config.ev_loop == EPOLL )
    {
          log_module(LOG_DEBUG,"SERVE_FOREVER","SERVER STARTUP EPOLL EVENT LOOP");
    }
    else if( config.ev_loop == LIBEV )
    {
            
            struct ev_loop *main_event_loop = EV_DEFAULT;
            struct ev_io *listen_watcher= new ev_io;
            if( main_event_loop == NULL || listen_watcher == NULL )
            {
                 log_module(LOG_INFO,"SERVE_FOREVER","MEMORY ALLOCATE FAILED WHEN INITILIZE THE MAIN EVENT LOOP OR ");
                 if( main_event_loop != NULL )
                {
                    free(main_event_loop);
                    main_event_loop = NULL;
                }
                if( listen_watcher != NULL )
                {
                    delete listen_watcher;
                    listen_watcher = NULL;
                }
               
           }
          else
          {
                log_module(LOG_DEBUG,"SERVE_FOREVER","SERVER STARTUP LIBEV EVENT LOOP");
                sdk_set_tcpnodelay( hls_listen_fd );
                sdk_set_nonblocking( hls_listen_fd );
                sdk_set_keepalive( hls_listen_fd );
                listen_watcher->fd = hls_listen_fd;
                ev_io_init( listen_watcher,listen_cb,hls_listen_fd, EV_READ );
                ev_io_start( main_event_loop, listen_watcher );
                ev_run( main_event_loop,0 );
                close( hls_listen_fd );
                ev_io_stop( main_event_loop, listen_watcher );
                delete listen_watcher;
                listen_watcher = NULL;
                free(main_event_loop);
                main_event_loop = NULL;
          }

    }
    else if( config.ev_loop == SELECT )
    {
          log_module(LOG_DEBUG,"SERVE_FOREVER","SERVER STARTUP SELECT EVENT LOOP");
    }
    close( hls_listen_fd );
}


void listen_cb( struct ev_loop * main_event_loop, struct ev_io * listen_watcher, int revents )
{
    log_module( LOG_DEBUG, "LISTEN_CB","++++++++++START++++++++++");
    if( EV_ERROR & revents )
    {
        log_module(LOG_INFO,"LISTEN_CB","LIBEV ERROR FOR EV_ERROR:%d--%s",EV_ERROR,LOG_LOCATION);
        return;
    }

    ssize_t client_fd;
    struct sockaddr_in *client_addr = new struct sockaddr_in;
    if( client_addr == NULL )
    {
        log_module(LOG_INFO,"LISTEN_CB","ALLOCATE MEMORY FAILED");   
        return;
    }
    socklen_t len = sizeof(struct sockaddr_in);
    client_fd = accept( listen_watcher->fd, (struct sockaddr *)client_addr, &len );
    if( client_fd < 0 )
    {
        delete client_addr;
        log_module(LOG_INFO,"LISTEN_CB","ACCEPT ERROR:%d-->%s--%s",errno,strerror(errno),LOG_LOCATION);   
        return;
    }
    


    struct ev_io * receive_request_watcher = new struct ev_io;
    if( receive_request_watcher == NULL )
    {
        log_module(LOG_INFO,"LISTEN_CB","ALLOCATE MEMORY FAILED:%s--%s",strerror(errno),LOG_LOCATION);
        delete client_addr;
        client_addr = NULL;
        close(client_fd);
        return;
    }
    
    receive_request_watcher->data = ( void *)client_addr;
    log_module(LOG_DEBUG,"LISTEN_CB","CLIENT %s:%d CONNECTED",
                     inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    sdk_set_nonblocking(client_fd);
    sdk_set_rcvbuf(client_fd, 65535);
    sdk_set_tcpnodelay(client_fd);
    sdk_set_keepalive(client_fd);

    //register the socket io callback for reading client's request    
    ev_io_init(receive_request_watcher,receive_request_cb,client_fd, EV_READ);
    ev_io_start(main_event_loop,receive_request_watcher);
    log_module( LOG_DEBUG, "LISTEN_CB", "++++++++++DONE++++++++++");
}


/*
#@args:also seeing the detailed information from the libev api document
#@returns:
#@desc:
#called asynchronously when streaming server received the data written by client
*/
void receive_request_cb( struct ev_loop * main_event_loop, struct  ev_io *receive_request_watcher, int revents )
{
    #define CLEAN_RECEIVE_WATCHER() \
    ev_io_stop(main_event_loop,receive_request_watcher);\
    close(receive_request_watcher->fd);\
    delete [] static_cast<char *>(receive_request_watcher->data);\
    receive_request_watcher->data = NULL;\
    delete receive_request_watcher;\
    receive_request_watcher = NULL;\
    return;
        
    if( EV_ERROR & revents )
    {
        log_module(LOG_INFO,"RECEIVE_REQUEST_CB","ERROR FOR EV_ERROR");
        CLEAN_RECEIVE_WATCHER();
    }

    struct sockaddr_in * client_addr = static_cast<struct sockaddr_in *>(receive_request_watcher->data);
    //used to store those key-values parsed from http request
    char http_request[1024];
    int read_bytes = read_http_header(receive_request_watcher->fd, http_request, sizeof(http_request) );
    if(  read_bytes == -1  )
    {     
          log_module( LOG_DEBUG, "RECEIVE_REQUEST_CB","READ_HTTP_HEADER FAILED RELATED TO %s:%d",
                                                inet_ntoa( client_addr->sin_addr),ntohs( client_addr->sin_port));
          CLEAN_RECEIVE_WATCHER();
    }

    http_request[read_bytes]='\0';
    log_module(LOG_DEBUG,"RECEIVE_REQUEST_CB","HTTP REQUEST HEADER IS:%s",http_request);
    
    STREAMER_REQUEST streamer_request;
    parse_streamer_request(http_request,read_bytes,streamer_request);
    if( streamer_request.method != "POST" )
    {
            log_module(LOG_DEBUG,"RECEIVE_REQUEST_CB","THE %s:%d'S REQUESTED METHOD IS %s: WE ONLY SUPPORT THE POST METHOD",
                                               inet_ntoa( client_addr->sin_addr),ntohs( client_addr->sin_port),streamer_request.method.c_str());
            
    }
    else if( streamer_request.channel.empty() || streamer_request.channel.length() > 128 )
    {
            log_module(LOG_DEBUG,"RECEIVE_REQUEST_CB","RECEIVE THE INVALID REQUESTED CHANNEL:%s OR IT'S TOO LONG",
                                             streamer_request.channel.c_str());
            
    }
    else if( streamer_request.status != 0 && streamer_request.status != 1 )
    {
            log_module(LOG_DEBUG,"SERVE_FOREVER","GOT THE STATUS:%d THE CHANNEL'S STATUS IS ERROR",streamer_request.status);                       
           
    }
    /*
   else if( streamer_request.token.empty() || streamer_request.token.length() > 128 )
   {
     log_module(LOG_DEBUG,"SERVE_FOREVER","RECEIVE THE INVALID REQUESTED TOKEN:%s OR IT'S TOO LONG",streamer_request.status);
     FD_CLR(client_fd,&read_fds);
     client_fd = -1;
   }
  */
    else if( streamer_request.status == OFFLINE )
    {
        //do_token_varify();
        std::map<CHANNEL,VIEWER>::iterator it = CHANNELS_POOL.find(streamer_request.channel);
        if( it == CHANNELS_POOL.end() )
        {
            log_module( LOG_DEBUG, "RECEIVE_REQUEST_CB","CHANNEL %s NOT EXISTS RELATED TO %s:%d",
                              streamer_request.channel.c_str(),inet_ntoa(client_addr->sin_addr),ntohs( client_addr->sin_port));
            
        }
        else
        {
            log_module( LOG_DEBUG, "RECEIVE_REQUEST_CB", "CHANNEL %s OFFLINE, JUST ERASE IT FROM CHANNEL POOL MEANWHILE STOP THE IO EVENT",
                                                                                      streamer_request.channel.c_str());

            if( it->second.receive_stream_watcher != NULL )
            {
                ev_io_stop( main_event_loop, it->second.receive_stream_watcher );
                delete it->second.receive_stream_watcher;
                it->second.receive_stream_watcher = NULL;

                if( it->second.flv_tag_data != NULL )
                delete [] it->second.flv_tag_data;    
            }
            CHANNELS_POOL.erase(it);
        }
    }
    else if( streamer_request.status == ONLINE )
    {
        if( CHANNELS_POOL.find( streamer_request.channel ) == CHANNELS_POOL.end() )
        {
            int viewer_sock_fd = tcp_connect(inet_ntoa(client_addr->sin_addr), STREAM_SERVER_PORT );
            if( viewer_sock_fd == -1 )
            {
                log_module( LOG_DEBUG, "RECEIVE_REQUEST_CB", "TCP_CONNECT FAILED RELATED TO %s:%d",
                                                  inet_ntoa( client_addr->sin_addr ),ntohs( client_addr->sin_port ) );
                CLEAN_RECEIVE_WATCHER();
            }
            
            sdk_set_rcvbuf( viewer_sock_fd , 65535 );
            sdk_set_tcpnodelay(viewer_sock_fd);
            sdk_set_keepalive(viewer_sock_fd);

            VIEWER viewer;
            viewer.receive_stream_watcher= new ev_io;
            if( viewer.receive_stream_watcher == NULL )
            {
                log_module( LOG_DEBUG, "RECEIVE_REQUEST_CB", "ALLOCATE MEMORY FAILED RELATED TO %s:%d",
                                                      inet_ntoa( client_addr->sin_addr ),ntohs( client_addr->sin_port ));
            }

            viewer.flv_tag_data = NULL;
            viewer.flv_header_read_bytes = 0;
            viewer.previous_tag_size_read_bytes = 0;
            viewer.flv_tag_header_read_bytes = 0;
            viewer.flv_tag_data_read_bytes = 0;
            viewer.flv_tag_data_total_bytes = 0;
            CHANNELS_POOL.insert( std::make_pair(streamer_request.channel, viewer) );
            viewer.receive_stream_watcher->fd = viewer_sock_fd;
            viewer.receive_stream_watcher->data = (void *) streamer_request.channel.c_str();
            std::string viewer_request;
            
            log_module( LOG_DEBUG, "RECEIVE_REQUEST_CB","BEGIN TO GENERATE THE VIEWER REQUEST");
            generate_viewer_request(streamer_request,viewer_request);
            log_module( LOG_DEBUG, "RECEIVE_REQUEST_CB","THE VIEWER REQUEST GENERATED:%s",viewer_request.c_str());

            log_module( LOG_DEBUG, "RECEIVE_REQUEST_CB","SEND THE VIEWER REQUEST TO STREAM SERVER %s:%d",
                                                  inet_ntoa(client_addr->sin_addr), ntohs( client_addr->sin_port ) );
            write_specify_size2( viewer_sock_fd, viewer_request.c_str(),viewer_request.length());
            log_module( LOG_DEBUG, "RECEIVE_REQUEST_CB","SEND THE VIEWER REQUEST DONE RELATED TO STREAM SERVER %s:%d",
                                                  inet_ntoa(client_addr->sin_addr), ntohs( client_addr->sin_port ) );
            
            ev_io_init( viewer.receive_stream_watcher,receive_stream_cb,viewer_sock_fd, EV_READ);
            ev_io_start(main_event_loop,viewer.receive_stream_watcher );
       }
    }
    CLEAN_RECEIVE_WATCHER();
}


