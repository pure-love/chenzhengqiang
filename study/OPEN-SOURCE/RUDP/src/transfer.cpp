/*
*@author:chenzhengqiang
*@start date:2015/6/25
*@desc:verify the client's personal singling and transfer speech message
*/

#include "common.h"
#include "rtp.h"
#include "logging.h"
#include "transfer.h"
#include "aes.h"
#include "../include/rudp_api.h" 
#include "../include/event.h" 


//macro specify the max receive buffer
#define MAX_RECV_BUF 65535
//global variable specify the personal singling type
static const int PERSONAL_SINGLING_TYPE=13;
static const int OPUS_RTP_PAYLOAD_TYPE = 97;
static int LOG_LEVEL = 0;

#define CHANNEL_NOT_CREATE "99"
#define CHANNEL_JOIN_REPEAT "98"
#define CHANNEL_NOT_JOIN "97"
#define CHANNEL_CREATE_REPEAT "94"
#define BAD_RTP_PAYLOAD_TYPE "96"
#define BAD_AES_ENCRYPT_LEN "95"
#define SERVER_INNER_ERROR "93"

#define LOG_START(X)  log_module(LOG_DEBUG,X,"###############START###############:%s",LOG_LOCATION)
#define LOG_DONE(X)   log_module(LOG_DEBUG,X,"###############DONE###############:%s",LOG_LOCATION)



typedef std::string IP;
typedef int PORT;
struct CONN_CLIENT
{
    PORT conn_port;
    struct sockaddr_in conn_addr;
};


typedef std::map<IP,CONN_CLIENT> CLIENTS;
typedef std::map<IP,CONN_CLIENT>::iterator client_iter;
typedef std::map<IP,CONN_CLIENT>::iterator & client_referrence_iter;

typedef std::string CHANNEL;
//one channel to multi-clients in a chat rooms
std::map<CHANNEL,CLIENTS> CHAT_ROOM;
typedef std::map<CHANNEL,CLIENTS>::iterator chat_room_iter;
typedef std::map<CHANNEL,CLIENTS>::iterator & chat_room_referrence_iter;



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
                          "this speech transfer server started on june 25 2015 with gcc 4.4.7 in CentOS\n" \
                          "programed by chenzhengqiang,based on udp protocol, which registered at port:%d\n\n" \
                          "now speech transfer server start listening :......\n\n\n\n\n",config.PORT);

    std::cout<<heading<<std::endl;
}



/*
#@args: 
#@returns:true indicates the client has joined the chatroom.false otherwise
#@desc:check if the client alread join the chat room
*/
static bool client_already_in( const char * ip,int port,chat_room_referrence_iter crr_iter)
{
     std::string ip_addr(ip);
     crr_iter = CHAT_ROOM.begin();
     while( crr_iter != CHAT_ROOM.end() )
     {
         client_iter c_iter = crr_iter->second.find( ip_addr );
         if( c_iter != crr_iter->second.end() )
         {
             if( c_iter->second.conn_port == port )
             {
                 return true;
             }   
         }
         ++crr_iter;
     }
     return false;
}



/*
#@args: 
#@returns:true indicates the client has joined the chatroom.false otherwise
#@desc:check if the client alread join the chat room
*/
static bool client_already_in( const char * IP,int PORT)
{
     std::string ip_addr(IP);
     chat_room_iter cr_iter = CHAT_ROOM.begin();
     while( cr_iter != CHAT_ROOM.end() )
     {
         client_iter c_iter = cr_iter->second.find( ip_addr );
         if( c_iter != cr_iter->second.end() )
         {
             if( c_iter->second.conn_port == PORT )
             {
                 return true;
             }   
         }
         ++cr_iter;
     }
     return false;
}



/*
#@args: 
#@returns:true indicates the channel exists,false otherwise
#@desc:check if the speech channel already exists
*/
static inline bool  speech_channel_exists( const std::string & channel )
{
    return CHAT_ROOM.find(channel) != CHAT_ROOM.end();
}



static inline chat_room_iter get_chat_room_iter(const std::string & channel )
{
    return CHAT_ROOM.find(channel);
}



/*
#@args: 
#@returns:void
#@desc:broadcast the speech message to others in a chatroom,except itself
*/
void broadcast_speech_message(int sock_fd, const uint8_t *rtp_packet,size_t size,
                                                             const char *IP,int PORT,chat_room_referrence_iter cr_iter)
{
    client_iter c_iter = cr_iter->second.begin();
    int ret = 0;
    while( c_iter != cr_iter->second.end() )
    {
        if(c_iter->first.compare(IP)==0 && c_iter->second.conn_port == PORT )
        {
            ++c_iter;
            continue;
        }
        
        log_module(LOG_DEBUG,"BROADCAST_SPEECH_MESSAGE","SEND SPEECH MESSAGE TO IP:%s PORT:%d SEND BYTES:%d",
                                           c_iter->first.c_str(),c_iter->second.conn_port,size);
        
        ret = sendto(sock_fd,rtp_packet,size, 0,
                           (struct sockaddr *)&(c_iter->second.conn_addr), 
                           sizeof(c_iter->second.conn_addr));
        
        if( ret == -1 )
        {
             log_module(LOG_INFO,"BROADCAST_SPEECH_MESSAGE","ERROR OCCURRED WHEN SEND MESSAGE TO:IP=%s PORT=%d"\
                               "ERROR INFORMATION:%s",c_iter->first.c_str(),c_iter->second.conn_port,strerror(errno));                  
        }
        ++c_iter;
    }
}



/*
#@args: 
#@returns:
#@desc:
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
        print_welcome(config);
    }

    //initialize the logger
    LOG_LEVEL = config.log_level;
    logging_init(config.log_file.c_str(),config.log_level);
    LOG_START("SERVE_FOREVER");
    struct rlimit rt;
    rt.rlim_max = rt.rlim_cur = MAX_OPEN_FDS;
    if ( setrlimit(RLIMIT_NOFILE, &rt) == -1 ) 
    {
        log_module(LOG_ERROR,"SERVE_FOREVER","SETRLIMIT FAILED:%s",strerror(errno));
    }
    
    //you have to ignore the PIPE's signal when client close the socket
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;//just ignore the sigpipe
    sa.sa_flags = 0;
    if( sigemptyset(&sa.sa_mask) == -1 ||sigaction(SIGPIPE, &sa, 0) == -1 )
    { 
        logging_deinit();  
        log_module(LOG_ERROR,"SERVE_FOREVER","Failed To Ignore SIGPIPE Signal");
    }

    sdk_set_nonblocking( sock_fd );
    int REUSEADDR_OK=1;
    setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&REUSEADDR_OK,sizeof(REUSEADDR_OK));
    
    if( config.ev_loop == DEFAULT )
    {
        log_module(LOG_DEBUG,"SERVE_FOREVER","SERVER STARTUP DEFAULT EVENT LOOP");
    }
    else if(config.ev_loop == EPOLL )
    {
         log_module(LOG_DEBUG,"SERVE_FOREVER","SERVER STARTUP EPOLL EVENT LOOP");
         int epoll_fd, events;
         struct epoll_event epoll_ev;
         struct epoll_event epoll_events[MAX_OPEN_FDS];
         epoll_fd = epoll_create(MAX_OPEN_FDS);
         epoll_ev.events = EPOLLIN | EPOLLET;
         epoll_ev.data.fd = sock_fd;
         if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &epoll_ev) < 0) 
        {
             log_module(LOG_ERROR,"SERVE_FOREVER","EPOLL_CTL_ADD ERROR FOR:FD=%d",sock_fd);
             exit(EXIT_FAILURE);
        }
        while ( true) 
        {
             events = epoll_wait(epoll_fd, epoll_events, MAX_OPEN_FDS, -1);
             if (events == -1)
             {
                 log_module(LOG_INFO,"SERVE_FOREVER","EPOLL_WAIT FAILED:%s",strerror(errno));
                 break;
             }
             for ( int event = 0; event < events; ++event )
             {
                 if (epoll_events[event].data.fd == sock_fd ) 
                 {
                      handle_udp_msg(sock_fd);
                 } 
             }
        }

    }
    else if( config.ev_loop == SELECT )
    {
          log_module(LOG_DEBUG,"SERVE_FOREVER","SERVER STARTUP SELECT EVENT LOOP");
    }
    close(sock_fd);
    LOG_DONE( "SERVE_FOREVER" );
}



/*
#@args: 
#@returns:
#@desc:
*/
void handle_udp_msg( int sock_fd )
{   
     log_module(LOG_DEBUG,"HANDLE_UDP_MSG","+++++++++++++++++START++++++++++++++++++++");
     ssize_t received_bytes;
     uint8_t rtp_packet[MAX_RECV_BUF];
     CONN_CLIENT conn_client;
     socklen_t addr_len = sizeof(conn_client.conn_addr);
     memset(rtp_packet,0,sizeof(rtp_packet));
     while( (received_bytes = recvfrom(sock_fd,rtp_packet, MAX_RECV_BUF, MSG_DONTWAIT, 
                (struct sockaddr *)&(conn_client.conn_addr), &addr_len)  ) <=0 )
     {
         if( errno == EAGAIN || errno == EWOULDBLOCK )
         {
             continue;
         }
         log_module(LOG_INFO,"HANDLE_UDP_MSG"," RECVFROM FAILED:%s",strerror(errno));
         break;
     }

     if( LOG_LEVEL >= LOG_DEBUG )
     {
         sendto(sock_fd,rtp_packet,received_bytes,0,(struct sockaddr *)&(conn_client.conn_addr),sizeof(conn_client.conn_addr));
     }
     
     if( received_bytes > 0 )
     {
         //get the ip,port of connected client
         char IP[INET_ADDRSTRLEN];
         int PORT = ntohs(conn_client.conn_addr.sin_port);
         conn_client.conn_port = PORT;
         inet_ntop(AF_INET,&(conn_client.conn_addr.sin_addr),IP,INET_ADDRSTRLEN);

         
         std::string ip_str(IP);
         log_module(LOG_INFO,"HANDLE_UDP_SERVER","RECEIVED MESSAGE FROM CLIENT:IP=%s PORT=%d;RECEIVED_BYTES:%d",
                          IP,PORT,received_bytes);
         
         RTP_HEADER rtp_header;
         int ret = rtp_header_parse(rtp_packet,(size_t)received_bytes,rtp_header);
         if( ret == -1 )
         {
             sendto(sock_fd,SERVER_INNER_ERROR,strlen(SERVER_INNER_ERROR)+1,
             0,(struct sockaddr *)&(conn_client.conn_addr),sizeof(conn_client.conn_addr));
             
             log_module(LOG_INFO,"HANDLE_UDP_MESSAGE","RTP_PACKET_HEADER_PARSE FAILED");
             return;
         }
         
         if( rtp_header.payload_type != PERSONAL_SINGLING_TYPE )
         {
             if( rtp_header.payload_type != OPUS_RTP_PAYLOAD_TYPE )
             {
                 sendto(sock_fd,BAD_RTP_PAYLOAD_TYPE,strlen(BAD_RTP_PAYLOAD_TYPE)+1,
                 0,(struct sockaddr *)&(conn_client.conn_addr),sizeof(conn_client.conn_addr));
                 
                 log_module(LOG_INFO,"HANDLE_UDP_MESSAGE","INVALID RTP PLAYLOAD TYPE:%d",rtp_header.payload_type);
                 goto END;
             }
             
             chat_room_iter cr_iter;
             if(client_already_in(IP,PORT,cr_iter))
             {
                 //just broadcast this speech message to all clients in chatroom,including the camera
                 log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","BROADCAST SPEECH MESSAGE TO OTHERS START");
                 broadcast_speech_message(sock_fd,rtp_packet,received_bytes,IP,PORT,cr_iter);
                 log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","BROADCAST SPEECH MESSAGE TO OTHERS DONE");
             }
             else
             {
                 //it might be hotile attack,just ignore it
                 sendto(sock_fd,CHANNEL_NOT_JOIN,strlen(CHANNEL_NOT_JOIN)+1,
                 0,(struct sockaddr *)&(conn_client.conn_addr),sizeof(conn_client.conn_addr));
                 
                 log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","IT MIGHT BE HOTILE ATTACK OR NOT JOIN THE SPEECH CHANNEL YET");
                 goto END;
             }
         }
         else
         {
             if( (size_t)(received_bytes-rtp_header.offset) != FIXED_AES_ENCRYPT_SIZE )
             {
                 sendto(sock_fd,BAD_AES_ENCRYPT_LEN,strlen(BAD_AES_ENCRYPT_LEN)+1,
                 0,(struct sockaddr *)&(conn_client.conn_addr),sizeof(conn_client.conn_addr));
                  
                 log_module(LOG_INFO,"HANDLE_UDP_MESSAGE","INVALID AES ENCRYPTED LENGTH:%d IT SHOULD BE %d",
                                  received_bytes-rtp_header.offset,FIXED_AES_ENCRYPT_SIZE);
                 goto END;
             }
             
             CLIENT_REQUEST client_request;   
             bool parse_ok = parse_client_request(rtp_packet+rtp_header.offset,received_bytes-rtp_header.offset,client_request);
             if( !parse_ok )
             {
                  sendto(sock_fd,SERVER_INNER_ERROR,strlen(SERVER_INNER_ERROR)+1,
                 0,(struct sockaddr *)&(conn_client.conn_addr),sizeof(conn_client.conn_addr));
                  
                 log_module(LOG_INFO,"HANDLE_UDP_MESSAGE","PARSE_CLIENT_REQUEST FAILED");
             }
             else
             {
                 log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","PARSE_CLIENT_REQUEST OK");
                 if( client_request.type == PC )
                 {
                     log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","THIS IS PC'S REQUEST");
                     if(speech_channel_exists(client_request.channel))
                     {
                          log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","REQUESTED SPEECH CHANNEL %s EXISTS",client_request.channel.c_str());
                          chat_room_iter cr_iter = get_chat_room_iter(client_request.channel);
                          if( client_request.action == JOIN )
                          {
                              log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","THIS IS PC'S JOIN REQUEST");
                              if( client_already_in(IP,PORT) )
                              {
                                  sendto(sock_fd,CHANNEL_JOIN_REPEAT,strlen(CHANNEL_JOIN_REPEAT)+1,
                                  0,(struct sockaddr *)&(conn_client.conn_addr),sizeof(conn_client.conn_addr));
                                  
                                  log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","REPEATED JOIN REQUEST CAUGHT FROM CLIENT %s PORT %d",IP,PORT);
                                  goto END;  
                              }
                              log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","CLIENT %s WANT JOIN THE SPEECH CHANNEL:%s",IP,client_request.channel.c_str());
                              cr_iter->second.insert(std::make_pair(ip_str,conn_client));
                              log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","CLIENT %s JOIN THE SPEECH CHANNEL %s SUCCEDED",IP,client_request.channel.c_str());
                          }
                          else if( client_request.action == LEAVE )
                          {
                              log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","THIS IS PC'S LEAVE REQUEST");
                              if( !client_already_in(IP,PORT))
                              {
                                  sendto(sock_fd,CHANNEL_NOT_JOIN,strlen(CHANNEL_NOT_JOIN)+1,
                                  0,(struct sockaddr *)&(conn_client.conn_addr),sizeof(conn_client.conn_addr));
                                   
                                  log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","CLIENT %s CAN NOT QUIT THE SPEECH CHANNEL:%s UNLESS JOINED IT",IP,client_request.channel.c_str());
                                  goto END;
                              }
                              log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","CLIENT %s WANT LEAVE THE SPEECH CHANNEL:%s",IP,client_request.channel.c_str());
                              cr_iter->second.erase(ip_str);
                              log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","CLIENT %s LEAVE THE SPEECH CHANNEL %s SUCCEDED",IP);
                          }
                     }
                     else
                     {
                           sendto(sock_fd,CHANNEL_NOT_CREATE,strlen(CHANNEL_NOT_CREATE)+1,
                           0,(struct sockaddr *)&(conn_client.conn_addr),sizeof(conn_client.conn_addr));
                           
                          log_module(LOG_INFO,"HANDLE_UDP_MESSAGE","CLIENT %s CAN NOT ACCESS THE NOT EXISTENT SPEECH CHANNEL %s",IP,client_request.channel.c_str());
                     }
                 }
                 else if( client_request.type == CAMERA )
                 {
                     if( speech_channel_exists(client_request.channel))
                     {
                         chat_room_iter cr_iter = get_chat_room_iter(client_request.channel);
                         if(client_request.action == CREATE )
                         {
                             log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","THIS IS CAMERA'S CREATE REQUEST");
                             sendto(sock_fd,CHANNEL_CREATE_REPEAT,strlen(CHANNEL_CREATE_REPEAT)+1,
                             0,(struct sockaddr *)&(conn_client.conn_addr),sizeof(conn_client.conn_addr));
                             
                             log_module(LOG_INFO,"HANDLE_UDP_MESSAGE","INVALID CLIENT REQUEST,CAN NOT CREATE THE CHANNEL:%s"\
                                                             "THAT ALREADY EXISTS",client_request.channel.c_str());
                         }
                         else if( client_request.action == CANCEL )
                         {
                             //just cancel the speech channel
                             log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","THIS IS CAMERA'S CANCEL REQUEST");
                             log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","CAMERA %s WANT CANCEL THE SPEECH CHANNEL:%s",IP,client_request.channel.c_str());
                             cr_iter->second.clear();
                             CHAT_ROOM.erase(cr_iter);
                             log_module(LOG_INFO,"HANDLE_UDP_MESSAGE","CAMERA CANCEL THE CHANNEL SUCCEDED");
                         }
                     }
                     else
                     {
                         if( client_request.action == CANCEL )
                         {
                             log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","THIS IS CAMERA'S CANCEL REQUEST");
                             sendto(sock_fd,CHANNEL_NOT_CREATE,strlen(CHANNEL_NOT_CREATE)+1,
                             0,(struct sockaddr *)&(conn_client.conn_addr),sizeof(conn_client.conn_addr));
                             
                             log_module(LOG_INFO,"HANDLE_UDP_MESSAGE","INVALID CLIENT REQUEST,CAN NOT CANCEL THE CHANNEL:%s THAT NOT EXISTS",
                                              client_request.channel.c_str());
                         }
                         else if( client_request.action  == CREATE )
                         {
                             //just create the speech channel
                             log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","THIS IS CAMERA'S CREATE REQUEST");
                             log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","CAMERA %s WANT CREATE THE SPEECH CHANNEL:%s",IP,client_request.channel.c_str());
                             CLIENTS clients;
                             clients.insert(std::make_pair(ip_str,conn_client));
                             CHAT_ROOM.insert(std::make_pair(client_request.channel,clients));
                             log_module(LOG_DEBUG,"HANDLE_UDP_MESSAGE","CAMERA %s CREATE THE CHANNEL %s SUCCEDED",IP,client_request.channel.c_str());
                         }
                     }
                 }
             }
         }
     }
     END:
     log_module(LOG_DEBUG,"HANDLE_UDP_MSG","+++++++++++++++++DONE++++++++++++++++++++");
}


bool parse_client_request( const uint8_t *packet, size_t size, CLIENT_REQUEST & client_request )
{
    if( size != FIXED_AES_DECRYPT_SIZE )
    {
        log_module(LOG_INFO,"PARSE_CLIENT_REQUEST","INVALID CLIENT REQUEST FOR BAD ENCRYPT SIZE");
        return false;
    }
    
    unsigned char decrypt_string[FIXED_AES_DECRYPT_SIZE];
    obtain_aes_decrypt_string(decrypt_string,packet);
    std::string request((char *)decrypt_string);
    log_module(LOG_DEBUG,"PARSE_CLIENT_REQUEST","CLIENT REQUEST:%s",request.c_str());
    std::string::iterator it = request.begin();
    std::vector<std::string> keys;
    std::string key;
    while( it != request.end())
    {
       while( it!= request.end() && *it != '_' )
       {
           key.push_back(*it);
           ++it;
       }
       keys.push_back(key);
       key = "";
       if( it == request.end())
       break; 
       ++it;
    }
    if( keys.size() < 4 )
    {
         log_module(LOG_INFO,"PARSE_CLIENT_REQUEST","KEYS.SIZE() < 4:INVALID CLIENT REQUEST");
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
         log_module(LOG_INFO,"PARSE_CLIENT_REQUEST","INVALID CLIENT REQUEST:BAD TYPE");
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
        log_module(LOG_INFO,"PARSE_CLIENT_REQUEST","INVALID CLIENT REQUEST:BAD ACTION %s",keys[1].c_str());
        return false;
    }
    client_request.channel = keys[2];
    client_request.timestamp = atol(keys[3].c_str());
    return true;
}

