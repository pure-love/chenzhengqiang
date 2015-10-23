/*
*@author:chenzhengqiang
*@start date:2015/6/25
*@modified date:
*@desc:verify the client's personal singling and transfer speech message
*/

#include "common.h"
//#include "ortp_recv.h"
//#include "ortp_send.h"
#include "netutility.h"
#include "rtp.h"
#include "opus.h"
#include "logging.h"
#include "transfer.h"
#include "speech_mix.h"
#include "aes.h"
#include <climits>
#include <sys/time.h>

//for calculating the elapsed time 
static struct timeval PREV;
static struct timeval NOW;
static bool the_first_calculate = true;
static const unsigned long MAX_ELAPSED_USEC= 500000;
static int LOG_LEVEL = -1;
static const int RTP_HEADER_SIZE = 12;

//global rtp header
RTP_HEADER REPLY_RTP_HEADER;

//one channel to multi-clients in a chat rooms
std::map<CHANNEL,CLIENTS> CHAT_ROOM;

std::map<ID,std::map<uint32_t, RTP_PACKET> > SPEAKERS_OPUS_RTP_PACKETS_POOL;
typedef std::map<ID,std::map<uint32_t,RTP_PACKET> >::iterator SORPP_ITER;

struct SPEAKER_RTP_PACKET
{
    ID C_ID;
    uint32_t speak_time;
    RTP_PACKET rtp_packet;
};


//the opus encoder,decoder related
static OpusDecoder *OPUS_DECODER = NULL;
static OpusEncoder *OPUS_ENCODER = NULL;


//for clear the senders's opus rtp packets' pool
void clear_senders_opus_rtp_packets_pool()
{
    ;
}



//init the global REPLY RTP HEADER FOR sync's sake
static inline void init_reply_rtp_header()
{
    REPLY_RTP_HEADER.version=2;
    REPLY_RTP_HEADER.padding=0;
    REPLY_RTP_HEADER.extension=0;
    REPLY_RTP_HEADER.csrc_count=0;	
    REPLY_RTP_HEADER.marker=0;
    REPLY_RTP_HEADER.payload_type= 0;
    REPLY_RTP_HEADER.sequence_no=0;
    REPLY_RTP_HEADER.timestamp=0;
    REPLY_RTP_HEADER.ssrc=0;
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
    snprintf(heading,1024,"\n"\
                          "\r    *\n"\
                          "\r  *   *  *      *     *  *     *     *  *       *\n" \
                          "\r *        *     *    *    *    *    *   *       *\n"\
                          "\r   *       *    *   *      *   *   *     *      *\n"\
                          "\r      *      * * * *        * * * *       * *  *\n"\
                          "\r  *   *       *   *          *   *             *\n"\
                          "\r    *                                         *\n"\
                          "\r                                            *"\
                          "\n\nsoftware version 1.7.1, Copyleft (c) 2015 SWWY\n" \
                          "this speech transfer server started on june 25 2015 with gcc 4.4.7 in CentOS\n" \
                          "programed by chenzhengqiang,based on udp protocol, which registered at port:%d\n\n" \
                          "now speech transfer server start listening :......\n\n\n\n\n",config.PORT );
    
    std::cout<<heading<<std::endl;
}



/*
#@returns:true indicates the client has joined the chatroom.false otherwise
#@desc:check if the client alread join the chat room
*/
bool client_already_in( const std::string &C_ID, chat_room_referrence_iter crr_iter )
{
     crr_iter = CHAT_ROOM.begin();
     while( crr_iter != CHAT_ROOM.end() )
     {
         client_iter c_iter = crr_iter->second.find( C_ID );
         if( c_iter != crr_iter->second.end() )
         {
             return true;   
         }
         ++crr_iter;
     }
     return false;
}



/*
#@args: the client's ID which consist of client's IP and PORT
#@returns:true indicates the client has joined the chatroom.false otherwise
#@desc:check if the client alread join the chat room
*/
bool client_already_in( const std::string & C_ID )
{
     chat_room_iter cr_iter = CHAT_ROOM.begin();
     while( cr_iter != CHAT_ROOM.end() )
     {
         client_iter c_iter = cr_iter->second.find(C_ID);
         if( c_iter != cr_iter->second.end() )
         {
               return true;
         }
         ++cr_iter;
     }
     return false;
}



/*
#@args: the requested channel
#@returns:true indicates the channel exists,false otherwise
#@desc:check if the speech channel already exists
*/
static inline bool  speech_channel_exists( const std::string & channel )
{
    return CHAT_ROOM.find( channel ) != CHAT_ROOM.end();
}


static inline chat_room_iter get_chat_room_iter(const std::string & channel )
{
    return CHAT_ROOM.find( channel );
}



/*
#@returns:void
#@desc:broadcast the speech message to others in a chatroom,except the sender itself
*/
void broadcast_speech_message( int sock_fd, const uint8_t *rtp_packet,size_t size,
                                                             const std::string &C_ID, chat_room_referrence_iter cr_iter  )
{

    client_iter c_iter = cr_iter->second.begin();
    int ret = 0;
    while( c_iter != cr_iter->second.end() )
    {
        if( c_iter->first == C_ID )
        {
            ++c_iter;
            continue;
        }
        
        log_module( LOG_DEBUG,"BROADCAST_SPEECH_MESSAGE","%s SEND SPEECH MESSAGE TO ID:%s SEND BYTES:%d",
                                            C_ID.c_str(),c_iter->first.c_str(), size);
        
        ret = sendto( sock_fd,rtp_packet,size, 0,
                           (struct sockaddr *)&(c_iter->second.conn_addr), 
                           sizeof(c_iter->second.conn_addr) );
        
        if( ret == -1 )
        {
             log_module(LOG_INFO,"BROADCAST_SPEECH_MESSAGE","ERROR OCCURRED WHEN SEND MESSAGE TO:IP=%s PORT=%d"\
                               "ERROR INFORMATION:%s",c_iter->first.c_str(),c_iter->second.conn_port,strerror(errno));                  
        }
        ++c_iter;
    }
    
}



/*
*@args:void
*@returns:void,program will'be killed if fail to initialize the opus encoder 
*@desc:initialize the opus encoder with the specified values,
  such as bit rate,sample rate and so on
*/
void init_opus_encoder()
{
        log_module( LOG_DEBUG, "INIT_OPUS_ENCODER", "++++++++++START++++++++++" );  
        OPUS_ENC_INFO opus_enc_info;
        opus_enc_info.application = OPUS_APPLICATION_VOIP;
        opus_enc_info.bandwidth = OPUS_AUTO;
        opus_enc_info.bitrate = BIT_RATE;
        opus_enc_info.channels = CHANNELS;
        opus_enc_info.packet_loss_percentage = 0;
        opus_enc_info.sample_rate = SAMPLE_RATE;
        opus_enc_info.use_vbr = USE_VBR;
        opus_enc_info.use_vbr_constant = USE_VBR_CONSTANT;
        opus_enc_info.lsb_depth = LSB_DEPTH;
        
        OPUS_ENCODER = create_opus_encoder( opus_enc_info );
        if( OPUS_ENCODER == NULL )
        {
            
            log_module( LOG_DEBUG, "INIT_OPUS_ENCODER", "++++++++++DONE++++++++++");  
            abort();
        }
        log_module( LOG_DEBUG, "INIT_OPUS_ENCODER", "++++++++++DONE++++++++++"); 
}



/*
*@args:void
*@returns:void,program will'be killed if fail to initialize the opus decoder
*@desc:initialize the opus decoder with the specified values,
  such as bit rate,sample rate and so on
*/
void init_opus_decoder()
{
    log_module( LOG_DEBUG, "INIT_OPUS_DECODER", "++++++++++START++++++++++");  
    OPUS_DEC_INFO opus_dec_info;
    opus_dec_info.channels=CHANNELS;
    opus_dec_info.sample_rate = SAMPLE_RATE;
    opus_dec_info.use_vbr = USE_VBR;
    opus_dec_info.use_vbr_constant = USE_VBR_CONSTANT;
    opus_dec_info.lsb_depth = LSB_DEPTH;
    
    OPUS_DECODER = create_opus_decoder( opus_dec_info );

    if( OPUS_DECODER == NULL )
    {
         log_module( LOG_DEBUG, "INIT_OPUS_DECODER", "++++++++++DONE++++++++++");  
         abort();
    }
    log_module( LOG_DEBUG, "INIT_OPUS_DECODER", "++++++++++DONE++++++++++"); 
}


/*
#@args:the udo sock fd, the global configure's information 
#@returns:void
#@desc:startup a event loop based on udp
*/
void serve_forever( ssize_t sock_fd, const CONFIG & config )
{
    //if run_as_daemon is true,then make this speech transfer server run as daemon
    if( config.run_as_daemon )
    {
        daemon(0,0);
    }
    else
    {
        print_welcome( config );
    }

    //initialize the logger
    //ortp_scheduler_init();
    logging_init(config.log_file.c_str(),config.log_level);
    LOG_LEVEL = config.log_level;
    struct rlimit rt;
    rt.rlim_max = rt.rlim_cur = MAX_OPEN_FDS;
    if ( setrlimit( RLIMIT_NOFILE, &rt ) == -1 ) 
    {
        log_module(LOG_ERROR,"SERVE_FOREVER","SETRLIMIT FAILED:%s",strerror(errno));
    }
    
    //you have to ignore the PIPE's signal when client close the socket
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;//just ignore the sigpipe
    sa.sa_flags = 0;
    if( sigemptyset( &sa.sa_mask ) == -1 ||sigaction( SIGPIPE, &sa, 0 ) == -1 )
    { 
        logging_deinit();  
        log_module(LOG_ERROR,"SERVE_FOREVER","FAILED TO IGNORE SIGPIPE SIGNAL:%s", strerror( errno ) );
    }

    sdk_set_nonblocking( sock_fd );
    int REUSEADDR_OK=1;
    setsockopt( sock_fd, SOL_SOCKET, SO_REUSEADDR , &REUSEADDR_OK , sizeof( REUSEADDR_OK ) );
    sdk_set_rcvbuf( sock_fd, 65535 );
    
    //init the opus encoder and decoder for mix's sake
    init_opus_encoder();
    init_opus_decoder();
    //init the global REPLY RTP HEADER
    init_reply_rtp_header();
    
    if( config.ev_loop == DEFAULT || config.ev_loop == EPOLL )
    {
         log_module(LOG_DEBUG,"SERVE_FOREVER","+++++EPOLL CONFIGURED,TRANSFER SERVER RUN EPOLL EVENT LOOP+++++");
         int epoll_fd, events;
         struct epoll_event epoll_ev;
         struct epoll_event epoll_events[1024];
         epoll_fd = epoll_create(  MAX_OPEN_FDS );
         epoll_ev.events = EPOLLIN | EPOLLET;
         epoll_ev.data.fd = sock_fd;
         
         if( epoll_ctl( epoll_fd, EPOLL_CTL_ADD, sock_fd, &epoll_ev ) < 0 ) 
        {
             log_module( LOG_ERROR, "SERVE_FOREVER","EPOLL_CTL_ADD ERROR FOR:FD=%d", sock_fd );
             goto BYEBYE;
        }

        while ( true ) 
        {
             events = epoll_wait( epoll_fd, epoll_events, sizeof( epoll_events ), -1 );
             if ( events == -1 )
             {
                 log_module( LOG_ERROR,"SERVE_FOREVER","EPOLL_WAIT FAILED:%s", strerror(errno) );
                 break;
             }
             
             for ( int event = 0; event < events; ++event )
             {
                 if ( epoll_events[event].data.fd == sock_fd && ( epoll_events[event].events & EPOLLIN ) ) 
                 {
                      log_module( LOG_DEBUG, "SERVE_FOREVER","UDP SOCK FD:%d CAN READ !NOW GO TO HANDLE_UDP_MSG ROUTINE",sock_fd );
                      handle_udp_msg( sock_fd );
                      log_module( LOG_DEBUG, "SERVE_FOREVER","HANDLE_UDP_MSG ROUTINE DONE,RETURN BACK TO MAIN EVENT LOOP");
                 } 
             }
        }

    }
    else if( config.ev_loop == SELECT )
    {
          log_module( LOG_DEBUG,"SERVE_FOREVER","SELECT CONFIGURED,SERVER STARTUP SELECT EVENT LOOP");
          fd_set udp_read_fds;
          FD_ZERO( &udp_read_fds );
          FD_SET( sock_fd, &udp_read_fds );
          while( true )
          {
                int ret = select( sock_fd+1, &udp_read_fds, NULL, NULL, NULL );
                if(  ret ==-1 )
                {
                    log_module( LOG_ERROR, "SERVE_FOREVER", "SELECT FAILED:%s", strerror( errno ) );
                    goto BYEBYE;
                }
                
                if( FD_ISSET( sock_fd, &udp_read_fds ) )
                {
                      log_module( LOG_DEBUG, "SERVE_FOREVER","UDP SOCK FD:%d CAN READ !NOW GO TO HANDLE_UDP_MSG ROUTINE",sock_fd );
                      handle_udp_msg( sock_fd );
                      log_module( LOG_DEBUG, "SERVE_FOREVER","HANDLE_UDP_MSG ROUTINE DONE,RETURN BACK TO MAIN EVENT LOOP" );
                }
          }
    }
    BYEBYE:
    close(sock_fd);
    logging_deinit();  
}




/*
#@args: udp server's sock fd
#@returns:void
#@desc:handle the personal signalling message and
    transfer the opus message, which are encapsulated to rtp packet,
    the playload type 3 indicates personal signalling type,
    and the playload type 0 indicates opus message type in rtp header 
*/
void handle_udp_msg( int sock_fd )
{   
     #define MSG_MODULE_1 "HANDLE_UDP_MSG"
     
     log_module( LOG_DEBUG,MSG_MODULE_1,"+++++++++++++++++START++++++++++++++++++++" );
     
     ssize_t received_bytes;
     uint8_t *rtp_packet = new uint8_t[SIGNALLING_BUFFER_SIZE];
     if( rtp_packet == NULL )
     {
        log_module( LOG_ERROR, MSG_MODULE_1, "ALLOCATE MEMORY FAILED RELATED TO new uint8_t[SIGNALLING_BUFFER_SIZE]");
        return;
     }
     
     CONN_CLIENT conn_client;
     socklen_t addr_len = sizeof( conn_client.conn_addr );
     memset( rtp_packet,0,sizeof( rtp_packet ) );
     while( ( received_bytes = recvfrom( sock_fd, rtp_packet, SIGNALLING_BUFFER_SIZE, MSG_DONTWAIT, 
                (struct sockaddr *)&(conn_client.conn_addr), &addr_len )  ) <=0 )
     {
         if( errno == EAGAIN || errno == EWOULDBLOCK )
         {
             continue;
         }
         log_module( LOG_INFO,MSG_MODULE_1," RECVFROM FAILED:%s",strerror(errno) );
         break;
     }
     
     if(  received_bytes > 0 )
     {
         //get the ip,port of connected client
         char IP[INET_ADDRSTRLEN];
         int PORT = ntohs( conn_client.conn_addr.sin_port );
         conn_client.conn_port = PORT;
         inet_ntop( AF_INET, &( conn_client.conn_addr.sin_addr), IP , INET_ADDRSTRLEN );

         
         std::string ip_str( IP );
         std::ostringstream OSS_ID;
         OSS_ID<<IP<<":"<<PORT;
         std::string C_ID=OSS_ID.str();
         log_module( LOG_INFO, MSG_MODULE_1, "RECEIVE MESSAGE FROM ID:%s RECEIVED_BYTES:%d",
                          C_ID.c_str(),received_bytes );
         
         RTP_HEADER rtp_header;
         int ret = rtp_header_parse( rtp_packet+RTP_HEADER_SIZE, ( size_t )received_bytes , rtp_header );
         if( ret == -1 )
         {
             log_module( LOG_INFO, MSG_MODULE_1, "RTP_PACKET_HEADER_PARSE FAILED RELATED TO ID:%s", C_ID.c_str() );
             delete [] rtp_packet;
             rtp_packet = NULL;
             return;
         }
         
         log_module( LOG_DEBUG,MSG_MODULE_1,"RECEIVED RTP PACKET:PAYLOAD TYPE:%d SEQUENCE NO:%d "\
                                                                    "TIMESTAMP:%u SSRC:%u RELATED TO ID:%s",
                                                                    rtp_header.payload_type,rtp_header.sequence_no,
                                                                    rtp_header.timestamp,rtp_header.ssrc, C_ID.c_str() );
       
         if( rtp_header.payload_type != PERSONAL_SINGLING_TYPE )
         {
             if( rtp_header.payload_type != OPUS_RTP_PAYLOAD_TYPE )
             {
                 log_module( LOG_INFO,MSG_MODULE_1,"INVALID RTP PLAYLOAD TYPE:%d RELATED TO ID:%s",
                                                                         rtp_header.payload_type, C_ID.c_str() );
             }
             else
             {

                log_module(  LOG_DEBUG, MSG_MODULE_1, "SPEAKER SEND THE OPUS RTP PACKET DETECTED RELATED TO ID:%s", C_ID.c_str() );
                chat_room_iter cr_iter;
                if( client_already_in( C_ID,cr_iter ) )
                {
                    log_module( LOG_DEBUG, MSG_MODULE_1, "CLIENT %s ALREADY CONNECTED", C_ID.c_str() );
                    std::map<ID,std::map<uint32_t, RTP_PACKET> >::iterator sorpp_iter = SPEAKERS_OPUS_RTP_PACKETS_POOL.find( C_ID );
                    if( sorpp_iter == SPEAKERS_OPUS_RTP_PACKETS_POOL.end() )
                    {
                        log_module( LOG_DEBUG, MSG_MODULE_1, "SPEAKER %s IS THE FIRST TIME OF SENDING OPUS RTP PACKET,ALLOCATE A OPUS RTP PACKETS POOL FOR MIX'S SAKE", C_ID.c_str() );
                        std::map<uint32_t, RTP_PACKET> rtp_packets_pool;
                        rtp_packets_pool.insert( std::make_pair( rtp_header.timestamp, rtp_packet ) );
                        SPEAKERS_OPUS_RTP_PACKETS_POOL.insert( std::make_pair( C_ID, rtp_packets_pool ) );
                    }
                    else
                    {
                        log_module( LOG_DEBUG, MSG_MODULE_1, "SENDER %s ALREADY HAVE OPUS RTP PACKETS POOL, JUST PUSK BACK THE RTP PACKET", C_ID.c_str() );
                        sorpp_iter->second.insert( std::make_pair(rtp_header.timestamp,rtp_packet) );
                    }
                    
                    //just broadcast this speech message to all clients in chatroom,including the camera
                    log_module( LOG_DEBUG, MSG_MODULE_1, "BROADCAST SPEECH MESSAGE START" );
                    
                    if( the_first_calculate )
                    {
                        gettimeofday( &PREV, NULL );
                        the_first_calculate = false;
                    }
                    else
                    {
                        gettimeofday( &NOW, NULL );
                        unsigned  long elapsed_usec = 1000000 * ( NOW.tv_sec-PREV.tv_sec )+ NOW.tv_usec-PREV.tv_usec;
                        if( elapsed_usec >= MAX_ELAPSED_USEC )
                        {
                            do_mix_and_broadcast( sock_fd, cr_iter );
                            PREV = NOW;
                        }
                    }
                }
                else
                {
                    //it might be hotile attack,just ignore it 
                    log_module( LOG_DEBUG, MSG_MODULE_1, "IT MIGHT BE HOTILE ATTACK OR NOT JOIN THE SPEECH CHANNEL YET RELATED TO ID:%s",
                                                                                C_ID.c_str() );
                    delete [] rtp_packet;
                    rtp_packet = NULL;
                }
             }
         }
         else 
         {
             if( ( size_t )( received_bytes-rtp_header.offset ) != FIXED_AES_ENCRYPT_SIZE )
             {
                 
                 log_module( LOG_INFO, MSG_MODULE_1, "INVALID AES ENCRYPTED LENGTH:%d IT SHOULD BE %d RELATED TO ID:%s",
                                  received_bytes-rtp_header.offset, FIXED_AES_ENCRYPT_SIZE, C_ID.c_str() );
             }
             else
             {
                
                CLIENT_REQUEST client_request;   
                bool parse_ok = parse_client_request( rtp_packet+rtp_header.offset,received_bytes-rtp_header.offset,client_request);
                if( !parse_ok )
                { 
                    log_module( LOG_INFO,MSG_MODULE_1,"PARSE_CLIENT_REQUEST FAILED RELATED TO ID:%s", C_ID.c_str() );
                }
                else
                {
                    log_module( LOG_DEBUG, MSG_MODULE_1,"PARSE_CLIENT_REQUEST OK RELATED TO ID:%s", C_ID.c_str() );
                    if( client_request.type == PC )
                    {
                        log_module( LOG_DEBUG, MSG_MODULE_1, "THIS IS PC 'S REQUEST RELATED TO ID:%s", C_ID.c_str() );
                        if( speech_channel_exists( client_request.channel ) )
                        {
                            log_module( LOG_DEBUG, MSG_MODULE_1,"REQUESTED SPEECH CHANNEL %s EXISTS RELATED TO ID:%s",
                                                                                      client_request.channel.c_str(), C_ID.c_str() );
                            chat_room_iter cr_iter = get_chat_room_iter(client_request.channel);
                            if( client_request.action == JOIN )
                            {
                                log_module( LOG_DEBUG, MSG_MODULE_1, "THIS IS PC'S JOIN REQUEST RELATED TO ID:%s", C_ID.c_str() );
                                if( client_already_in( C_ID ) )
                                {
                                    log_module( LOG_DEBUG, MSG_MODULE_1, "REPEATED JOIN REQUEST CAUGHT FROM ID:", C_ID.c_str() );
                                }
                                else
                                {
                                    log_module( LOG_DEBUG, MSG_MODULE_1, "CLIENT %s WANT JOIN THE SPEECH CHANNEL:%s",C_ID.c_str(),client_request.channel.c_str());
                                    cr_iter->second.insert( std::make_pair(C_ID,conn_client) );
                                    REPLY_RTP_HEADER.timestamp = (uint32_t) time( NULL );

                                    //send 3 times,in case of udp packet's loss
                                    sendto( sock_fd, &REPLY_RTP_HEADER, sizeof(REPLY_RTP_HEADER),0,
                                          (struct sockaddr *)&conn_client.conn_addr,sizeof(conn_client.conn_addr) );
                                    sendto( sock_fd, &REPLY_RTP_HEADER, sizeof(REPLY_RTP_HEADER),0,
                                          (struct sockaddr *)&conn_client.conn_addr, sizeof(conn_client.conn_addr) );
                                    sendto( sock_fd, &REPLY_RTP_HEADER, sizeof(REPLY_RTP_HEADER),0,
                                          (struct sockaddr *)&conn_client.conn_addr,sizeof(conn_client.conn_addr) );
                                    
                                    log_module( LOG_DEBUG, MSG_MODULE_1, "CLIENT %s JOIN THE SPEECH CHANNEL %s SUCCEDED",C_ID.c_str(),client_request.channel.c_str());
                                }
                            }
                            else if( client_request.action == LEAVE )
                            {
                                log_module( LOG_DEBUG, MSG_MODULE_1, "THIS IS PC'S LEAVE REQUEST RELATED TO ID:%s", C_ID.c_str() );
                                if( !client_already_in( C_ID ) )
                                {
                                    log_module( LOG_DEBUG, MSG_MODULE_1, "CLIENT %s CAN NOT QUIT THE SPEECH CHANNEL:%s UNLESS JOIN IT",IP,client_request.channel.c_str());
                                 
                                }
                                else
                                {
                                    log_module( LOG_DEBUG, MSG_MODULE_1, "CLIENT %s WANT LEAVE THE SPEECH CHANNEL:%s",C_ID.c_str(),client_request.channel.c_str());
                                    cr_iter->second.erase( C_ID );
                                    log_module( LOG_DEBUG, MSG_MODULE_1, "CLIENT %s LEAVE THE SPEECH CHANNEL %s SUCCEDED",C_ID.c_str(),client_request.channel.c_str());
                                }
                            }
                        }
                        else
                        { 
                            log_module( LOG_INFO, MSG_MODULE_1, "CLIENT %s CAN NOT ACCESS THE NOT EXISTENT SPEECH CHANNEL %s",C_ID.c_str(),client_request.channel.c_str());
                        }
                    }
                    else if( client_request.type == CAMERA )
                    {
                        if( speech_channel_exists( client_request.channel ) )
                        {
                            chat_room_iter cr_iter = get_chat_room_iter( client_request.channel );
                            if(client_request.action == CREATE )
                            {
                                log_module( LOG_DEBUG,MSG_MODULE_1,"THIS IS CAMERA'S CREATE REQUEST RELATED TO ID:%s", C_ID.c_str() );
                                log_module( LOG_DEBUG,MSG_MODULE_1,
                                "RECEIVED THE REPEATED CREATE REQUEST FROM %s ,JUST REPLY 0",C_ID.c_str() );

                                struct SERVER_REPLY server_reply;
                                server_reply.start = REPLY_FLAG;
                                server_reply.status = 0;
                                server_reply.end=REPLY_FLAG;
                                sendto( sock_fd,&server_reply,sizeof(server_reply),0,
                                          (struct sockaddr *)&conn_client.conn_addr,sizeof(conn_client.conn_addr) );
                            }
                            else if( client_request.action == CANCEL )
                            {
                                //just cancel the speech channel
                                log_module( LOG_DEBUG,MSG_MODULE_1,"THIS IS CAMERA'S CANCEL REQUEST RELATED TO ID:%s", C_ID.c_str() );
                                log_module( LOG_DEBUG,MSG_MODULE_1,"CAMERA %s WANT CANCEL THE SPEECH CHANNEL:%s",IP,client_request.channel.c_str());
                                cr_iter->second.clear();
                                CHAT_ROOM.erase( cr_iter );
                                log_module( LOG_INFO,MSG_MODULE_1,"CAMERA CANCEL THE CHANNEL SUCCEDED RELATED TO ID:%s", C_ID.c_str() );

                                struct SERVER_REPLY server_reply;
                                server_reply.start = REPLY_FLAG;
                                server_reply.status = 0;
                                server_reply.end=REPLY_FLAG;
                                sendto( sock_fd,&server_reply,sizeof(server_reply),0,
                                          (struct sockaddr *)&conn_client.conn_addr,sizeof(conn_client.conn_addr) );
                            }
                        }
                        else
                        {
                            if( client_request.action == CANCEL )
                            {
                                log_module( LOG_DEBUG,MSG_MODULE_1,"THIS IS CAMERA'S CANCEL REQUEST RELATED TO ID:%s", C_ID.c_str() );
                                log_module( LOG_INFO,MSG_MODULE_1,"INVALID CLIENT REQUEST,CAN NOT CANCEL THE CHANNEL:%s THAT NOT EXISTS"\
                                                                                      " RELATED TO ID:%s",
                                                                                      client_request.channel.c_str(), C_ID.c_str() );
                             
                            }
                            else if( client_request.action  == CREATE )
                            {
                                //just create the speech channel
                                log_module( LOG_DEBUG,MSG_MODULE_1, "THIS IS CAMERA'S CREATE REQUEST RELATED TO ID:%s", C_ID.c_str() );
                                log_module( LOG_DEBUG,MSG_MODULE_1, "CAMERA %s WANT CREATE THE SPEECH CHANNEL:%s",C_ID.c_str(),client_request.channel.c_str() );
                                CLIENTS clients;
                                clients.insert(std::make_pair(C_ID,conn_client));
                                CHAT_ROOM.insert(std::make_pair(client_request.channel,clients));
                                struct SERVER_REPLY server_reply;
                                server_reply.start = REPLY_FLAG;
                                server_reply.status = 0;
                                server_reply.end = REPLY_FLAG;
                                sendto( sock_fd,&server_reply,sizeof(server_reply),0,
                                          (struct sockaddr *)&conn_client.conn_addr,sizeof(conn_client.conn_addr));
                                log_module( LOG_DEBUG,MSG_MODULE_1,"CAMERA %s CREATE THE CHANNEL %s SUCCEDED",C_ID.c_str(),client_request.channel.c_str() );
                            }
                        }
                    }
                }
             }
             
             delete [] rtp_packet;
             rtp_packet = NULL;
         }
     }
     else
     {
        delete [] rtp_packet;
        rtp_packet = NULL;
        log_module( LOG_ERROR, MSG_MODULE_1, "RECVFROM ERROR:%s", strerror(errno) );
     }
     log_module(LOG_DEBUG,"HANDLE_UDP_MSG","+++++++++++++++++DONE++++++++++++++++++++");
}


//parse the client request from personal signalling
//each field saved in "CLIENT_REQUEST"
bool parse_client_request( const uint8_t *packet, size_t size, CLIENT_REQUEST & client_request )
{
    if( size != FIXED_AES_DECRYPT_SIZE )
    {
        log_module( LOG_ERROR,"PARSE_CLIENT_REQUEST","INVALID CLIENT REQUEST FOR BAD ENCRYPT SIZE:%d", (int) size);
        return false;
    }
    
    unsigned char decrypt_string[FIXED_AES_DECRYPT_SIZE];
    obtain_aes_decrypt_string(decrypt_string,packet);
    std::string request( (char *)decrypt_string );
    log_module( LOG_DEBUG,"PARSE_CLIENT_REQUEST","CLIENT REQUEST:%s",request.c_str() );
    std::string::iterator it = request.begin();
    std::vector<std::string> keys;
    std::string key;
    
    while(  it != request.end() )
    {
       while( it != request.end() && *it != '_' )
       {
           key.push_back( *it );
           ++it;
       }
       
       keys.push_back(key);
       key = "";
       
       if( it == request.end() )
       break; 
       ++it;
    }

    if( keys.size() < 4 )
    {
         log_module( LOG_ERROR,"PARSE_CLIENT_REQUEST","KEYS.SIZE() < 4:INVALID CLIENT REQUEST" );
         return false;
    }

    if( keys[0] == "CAMERA" )
    {
        client_request.type = 0;
    }
    else if( keys[0] == "PC" )
    {
        client_request.type = 1;
    }
    else
    {
         log_module( LOG_INFO,"PARSE_CLIENT_REQUEST","INVALID CLIENT REQUEST:BAD TYPE");
         return false;
    }
    
    if( keys[1] == "CREATE" || keys[1] == "JOIN" )
    {
        client_request.action = 0;
    }
    
    else if( keys[1] == "LEAVE" || keys[1] == "CANCEL" )
    {
        client_request.action = 1;
    }
    else
    {
        log_module( LOG_ERROR,"PARSE_CLIENT_REQUEST","INVALID CLIENT REQUEST:BAD ACTION %s",keys[1].c_str());
        return false;
    }
    
    client_request.channel = keys[2];
    client_request.timestamp = atol( keys[3].c_str() );

    if( keys.size() == 5 )
    {
        client_request.UID=atol( keys[4].c_str() );
    }
    return true;
}



/*
*@args:the udp_sock_fd indicates the udp server's file descriptor
       the crr_iter specify the C++ 's iterator of global CHAT_ROOM
*@desc:
    decode the SPEAKERS_OPUS_RTP_PACKETS_POOL's opus data to pcm data,and then mix them;
    when mixed,enocode the mixed buffer to opus and do broadcast to all listeners in chatroom
*/
void do_mix_and_broadcast( int udp_sock_fd, chat_room_referrence_iter crr_iter )
{
    #define MSG_MODULE_5 "DO_MIX_AND_BROADCAST"
    
    log_module( LOG_DEBUG, MSG_MODULE_5, "++++++++++START++++++++++");
    if ( SPEAKERS_OPUS_RTP_PACKETS_POOL.empty() )
    {
        log_module( LOG_DEBUG, MSG_MODULE_5, "++++++++++DONE++++++++++");
        return;
    }

    SORPP_ITER sorpp_iter = SPEAKERS_OPUS_RTP_PACKETS_POOL.begin();
    if( SPEAKERS_OPUS_RTP_PACKETS_POOL.size() == 1 )
    {
        log_module( LOG_DEBUG, MSG_MODULE_5, "THERE IS ONLY ONE SPEAKER, JUST BROADCAST ALL OPUS RTP PACKETS TO OTHERS RELATED TO %s", 
                                                                     sorpp_iter->first.c_str() );
        
        //just broadcast the rtp packet from single speaker
        std::map<uint32_t, RTP_PACKET>::iterator rp_iter = sorpp_iter->second.begin();
        
        while( rp_iter != sorpp_iter->second.end() )
        {
            if( rp_iter->second != NULL )
            {
                broadcast_speech_message( udp_sock_fd, rp_iter->second+RTP_HEADER_SIZE, OPUS_RTP_BUFFER_SIZE,
                                                           sorpp_iter->first, crr_iter );
                delete [] rp_iter->second;
                rp_iter->second = NULL;
            }
            else
            {
                log_module( LOG_ERROR, MSG_MODULE_5, "RTP PACKET POINTER DETECTED IS NULL RELATED TO ID:%s", 
                                  sorpp_iter->first.c_str() );
            }
            ++rp_iter;
        }
        
        sorpp_iter->second.clear();
        log_module( LOG_DEBUG, MSG_MODULE_5, "THERE IS ONLY ONE SPEAKER, BROADCAST ALL OPUS RTP PACKETS DONE RELATED TO %s", 
                                                                     sorpp_iter->first.c_str() );
    }
    else
    {
         
       log_module( LOG_DEBUG, MSG_MODULE_5, "THERE ARE MORE THAN ONE SPEAKER,JUST MERGE THESE BUFFERS TO ONE BUFFER AND MIX");
       std::vector<SPEAKER_RTP_PACKET> merged_rtp_packets_pool;
       std::vector<SPEAKER_RTP_PACKET> tmp_rtp_packets_pool;
       SPEAKER_RTP_PACKET speaker_rtp_packet; 
       SORPP_ITER curr_sorpp_iter = sorpp_iter;
       SORPP_ITER next_sorpp_iter = ++sorpp_iter;
       
       std::map<uint32_t, RTP_PACKET>::iterator curr_rp_iter = curr_sorpp_iter->second.begin();
       log_module( LOG_DEBUG, MSG_MODULE_5, "THE FIRST OPUS RTP PACKETS BUFFER RELATED TO %s", curr_sorpp_iter->first.c_str() );
       std::map<uint32_t, RTP_PACKET>::iterator next_rp_iter = next_sorpp_iter->second.begin();
       log_module( LOG_DEBUG, MSG_MODULE_5, "THE SECOND OPUS RTP PACKETS BUFFER RELATED TO %s", next_sorpp_iter->first.c_str() ); 
       
       while( curr_rp_iter != curr_sorpp_iter->second.end() 
                 &&
                 next_rp_iter != next_sorpp_iter->second.end() )
       {
            if( curr_rp_iter->first <= next_rp_iter->first )
            {
                speaker_rtp_packet.C_ID = curr_sorpp_iter->first;
                speaker_rtp_packet.speak_time = curr_rp_iter->first;
                speaker_rtp_packet.rtp_packet = curr_rp_iter->second;
                merged_rtp_packets_pool.push_back( speaker_rtp_packet );
                ++curr_rp_iter;
            }
            else
            {
                speaker_rtp_packet.C_ID = next_sorpp_iter->first;
                speaker_rtp_packet.speak_time = next_rp_iter->first;
                speaker_rtp_packet.rtp_packet = next_rp_iter->second;
                merged_rtp_packets_pool.push_back( speaker_rtp_packet );
                ++next_rp_iter;
            }
       }

       while( curr_rp_iter != curr_sorpp_iter->second.end() )
       {
            speaker_rtp_packet.C_ID = curr_sorpp_iter->first;
            speaker_rtp_packet.speak_time = curr_rp_iter->first;
            speaker_rtp_packet.rtp_packet = curr_rp_iter->second;
            merged_rtp_packets_pool.push_back( speaker_rtp_packet );
            ++curr_rp_iter;
       }

       while( next_rp_iter != next_sorpp_iter->second.end() )
       {
            speaker_rtp_packet.C_ID = next_sorpp_iter->first;
            speaker_rtp_packet.speak_time = next_rp_iter->first;
            speaker_rtp_packet.rtp_packet = next_rp_iter->second;
            merged_rtp_packets_pool.push_back( speaker_rtp_packet );
            ++next_rp_iter;
       }
       
       ++next_sorpp_iter;
       while( next_sorpp_iter != SPEAKERS_OPUS_RTP_PACKETS_POOL.end() )
       {
            log_module( LOG_DEBUG, MSG_MODULE_5, "THE LATER OPUS RTP PACKETS BUFFER RELATED TO %s", next_sorpp_iter->first.c_str() );
            next_rp_iter = next_sorpp_iter->second.begin();
            std::vector<SPEAKER_RTP_PACKET>::iterator srp_iter = merged_rtp_packets_pool.begin();
            while( next_rp_iter != next_sorpp_iter->second.end() 
                      &&
                      srp_iter != merged_rtp_packets_pool.end() )
            {
                if( next_rp_iter->first <= srp_iter->speak_time )
                {
                    speaker_rtp_packet.C_ID = next_sorpp_iter->first;
                    speaker_rtp_packet.speak_time = next_rp_iter->first;
                    speaker_rtp_packet.rtp_packet = next_rp_iter->second;
                    tmp_rtp_packets_pool.push_back( speaker_rtp_packet );
                    ++next_rp_iter;
                }
                else
                {
                    speaker_rtp_packet.C_ID = srp_iter->C_ID;
                    speaker_rtp_packet.speak_time = srp_iter->speak_time;
                    speaker_rtp_packet.rtp_packet = srp_iter->rtp_packet;
                    tmp_rtp_packets_pool.push_back( speaker_rtp_packet );
                    ++srp_iter;
                }
            }

            while( next_rp_iter != next_sorpp_iter->second.end() )
            {
                    speaker_rtp_packet.C_ID = next_sorpp_iter->first;
                    speaker_rtp_packet.speak_time = next_rp_iter->first;
                    speaker_rtp_packet.rtp_packet = next_rp_iter->second;
                    tmp_rtp_packets_pool.push_back( speaker_rtp_packet );
                    ++next_rp_iter;
            }

            while( srp_iter != merged_rtp_packets_pool.end() )
            {
                    speaker_rtp_packet.C_ID = srp_iter->C_ID;
                    speaker_rtp_packet.speak_time = srp_iter->speak_time;
                    speaker_rtp_packet.rtp_packet = srp_iter->rtp_packet;
                    tmp_rtp_packets_pool.push_back( speaker_rtp_packet );
                    ++srp_iter;
            }

            merged_rtp_packets_pool.swap(tmp_rtp_packets_pool);
            tmp_rtp_packets_pool.clear();
            ++next_sorpp_iter;
       }

       log_module( LOG_DEBUG, MSG_MODULE_5, "ALL SPEAKERS' RTP PACKETS BUFFER MERGED DONE" );
       if( LOG_LEVEL == LOG_DEBUG )
       {
            log_module( LOG_DEBUG, MSG_MODULE_5, "++++++++++PRINT ALL SPEAKERS INFO START++++++++++" );
            std::vector<SPEAKER_RTP_PACKET>::iterator mrpp_iter = merged_rtp_packets_pool.begin();
            while( mrpp_iter != merged_rtp_packets_pool.end() )
            {
                log_module( LOG_DEBUG, MSG_MODULE_5, "SPEAKER ID:%s SPEAKER TIME:%u", 
                                                                              mrpp_iter->C_ID.c_str(), mrpp_iter->speak_time );
                ++mrpp_iter;
            }
            log_module( LOG_DEBUG, MSG_MODULE_5, "++++++++++PRINT ALL SPEAKERS INFO DONE++++++++++" );
       }

       log_module( LOG_DEBUG, MSG_MODULE_5, "++++++++++DO MIX AND BROADCAST START++++++++++" ); 
       std::vector<SPEAKER_RTP_PACKET>::iterator mrpp_iter = merged_rtp_packets_pool.begin();
       std::vector<SPEAKER_RTP_PACKET>::iterator curr_mrpp_iter;
       std::vector<SPEAKER_RTP_PACKET>::iterator next_mrpp_iter;
       opus_int16 decoded_pcm_buffer1[320];
       opus_int16 decoded_pcm_buffer2[320];
       opus_int16 mixed_pcm_buffer[320];
       bool already_mixed = false;
       opus_int32 encode_bytes = 0;
       unsigned char encoded_opus_buffer[MAX_PACKET];
       uint8_t rtp_packet[OPUS_RTP_BUFFER_SIZE];
       opus_int32 decode_bytes;
       
       while( mrpp_iter != merged_rtp_packets_pool.end() )
       {
            curr_mrpp_iter = mrpp_iter;
            next_mrpp_iter=++mrpp_iter;
            
            if( next_mrpp_iter == merged_rtp_packets_pool.end() )
            {
                if( already_mixed )
                {
                    encode_bytes = encode_pcm_stream( OPUS_ENCODER,mixed_pcm_buffer,
                                                                        MIN_FRAME_SAMP, encoded_opus_buffer, MAX_PACKET );
                    if( encode_bytes > 0 )
                    {
                        REPLY_RTP_HEADER.timestamp =( uint32_t ) time( NULL ); 
                        rtp_packet_encapsulate( rtp_packet,RTP_HEADER_LEAST_SIZE+encode_bytes,
                                                           encoded_opus_buffer, encode_bytes , REPLY_RTP_HEADER );
                        
                        broadcast_speech_message( udp_sock_fd, rtp_packet, OPUS_RTP_BUFFER_SIZE,
                                                                    "MIX", crr_iter );
                        memset( mixed_pcm_buffer, 0, sizeof( mixed_pcm_buffer ) );
                    
                    }
                    else
                    {
                        log_module( LOG_ERROR, MSG_MODULE_5, "ENCODE PCM STREAM FAILED RELATED TO %s", curr_mrpp_iter->C_ID.c_str() );
                    }
                }
                break;
            }
            
            if( curr_mrpp_iter->speak_time != next_mrpp_iter->speak_time )
            {
                if( !already_mixed )
                {
                    broadcast_speech_message( udp_sock_fd, curr_mrpp_iter->rtp_packet+RTP_HEADER_SIZE,
                                                                OPUS_RTP_BUFFER_SIZE, curr_mrpp_iter->C_ID, crr_iter );
                    delete [] curr_mrpp_iter->rtp_packet;
                    curr_mrpp_iter->rtp_packet = NULL;
                }
                else
                {
                    encode_bytes = encode_pcm_stream( OPUS_ENCODER, mixed_pcm_buffer,
                                                                         MIN_FRAME_SAMP, encoded_opus_buffer, MAX_PACKET );
                    if( encode_bytes > 0 )
                    {
                        REPLY_RTP_HEADER.timestamp =(uint32_t) time( NULL ); 
                        rtp_packet_encapsulate( rtp_packet,RTP_HEADER_LEAST_SIZE+encode_bytes,
                                                           encoded_opus_buffer, encode_bytes , REPLY_RTP_HEADER );
                        
                        broadcast_speech_message( udp_sock_fd, rtp_packet, OPUS_RTP_BUFFER_SIZE,
                                                                    "MIX", crr_iter );
                        memset( mixed_pcm_buffer, 0, sizeof( mixed_pcm_buffer ) );
                    
                    }
                    else
                    {
                        log_module( LOG_ERROR, MSG_MODULE_5, "ENCODE PCM STREAM FAILED RELATED TO %s", curr_mrpp_iter->C_ID.c_str() );
                    }
                }
                already_mixed = false;
            }
            else
            {
                if( !already_mixed )
                {
                    decode_bytes = decode_opus_stream( OPUS_DECODER,curr_mrpp_iter->rtp_packet+RTP_HEADER_SIZE+12,
                                                                                 40,decoded_pcm_buffer1, 320 );
                    log_module( LOG_DEBUG, MSG_MODULE_5, "DECODE %d BYTES RELATED TO %s", curr_mrpp_iter->C_ID.c_str() );
                    decode_bytes = decode_opus_stream( OPUS_DECODER, next_mrpp_iter->rtp_packet+12,
                                                                                 40, decoded_pcm_buffer2, 320 );
                    log_module( LOG_DEBUG, MSG_MODULE_5, "DECODE %d BYTES RELATED TO %s", next_mrpp_iter->C_ID.c_str() );
                    
                    generate_speech_mix( (char *)decoded_pcm_buffer1, (char *)decoded_pcm_buffer2, (char *)mixed_pcm_buffer, 640 );
                    memset( decoded_pcm_buffer2, 0, sizeof( decoded_pcm_buffer2 ) );
                    memset( decoded_pcm_buffer1, 0, sizeof( decoded_pcm_buffer1 ) );

                    delete [] curr_mrpp_iter->rtp_packet;
                    delete [] next_mrpp_iter->rtp_packet;
                    
                }
                else
                {
                    decode_bytes = decode_opus_stream( OPUS_DECODER, next_mrpp_iter->rtp_packet+RTP_HEADER_SIZE+12,
                                                                                 40, decoded_pcm_buffer2, 320 );
                    log_module( LOG_DEBUG, MSG_MODULE_5, "DECODE %d BYTES RELATED TO %s", next_mrpp_iter->C_ID.c_str() );
                    
                    generate_speech_mix( (char *) decoded_pcm_buffer2, (char *)mixed_pcm_buffer, (char *)decoded_pcm_buffer1, 640 );
                    memcpy( mixed_pcm_buffer, decoded_pcm_buffer1, sizeof(mixed_pcm_buffer) );
                    memset( decoded_pcm_buffer2, 0, sizeof( decoded_pcm_buffer2 ) );
                    memset( decoded_pcm_buffer1, 0, sizeof( decoded_pcm_buffer1 ) );

                    delete [] next_mrpp_iter->rtp_packet;
                }
                already_mixed = true;
            }
            mrpp_iter = next_mrpp_iter;
       }
       
       merged_rtp_packets_pool.clear();
       log_module( LOG_DEBUG, MSG_MODULE_5, "++++++++++DO MIX AND BROADCAST DONE++++++++++" ); 
    }
    
    SPEAKERS_OPUS_RTP_PACKETS_POOL.clear();
}

