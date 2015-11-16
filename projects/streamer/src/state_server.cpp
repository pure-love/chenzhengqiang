/***********************************************************
 * @File Name:state_server.cpp
 * @Author:chenzhengqiang
 * @company:swwy
 * @Date 2015/5/7
 * @Modified:
 * @Version:1.0
 * @Desc: providing another server listing on 9090 to handle the
 request of streamer's stat,etc.g:streamer's memory occupy
 cpu occupy and so on
 **********************************************************/


#include"common.h"
#include"state_server.h"
#include"netutility.h"
#include"streamer.h"
#include"system_info.h"
#include"logging.h"
#include"parson.h"
#include"rosehttp.h"


system_info SI;

void * state_server_entry( void * args )
{
    //convert the args to long,cause it might be 64 ocet pointer
    int listen_fd = (long)args;
    log_module(LOG_DEBUG,"STATE_SERVER_ENTRY START","ID:%u",pthread_self());
    struct ev_loop *state_server_loop = ev_loop_new( EVBACKEND_SELECT );
    if( state_server_loop == NULL )
    {
        log_module( LOG_ERROR,"STATE_SERVER","EV_LOOP_NEW FAILED:%s",LOG_LOCATION );
        close( listen_fd );
        return NULL;
    }
    
    struct ev_io *listen_watcher = new struct ev_io;
    if( listen_watcher == NULL )
    {
        free( state_server_loop );
        state_server_loop = NULL;
        log_module( LOG_ERROR, "STATE_SERVER", "ALLOCATE MEMORY FAILED:%s", LOG_LOCATION );
        close( listen_fd );
        return NULL;
    }
    
    ev_io_init( listen_watcher, accept_request_cb, listen_fd, EV_READ );
    ev_io_start( state_server_loop, listen_watcher );
    ev_run( state_server_loop, 0 );

    close( listen_watcher->fd );
    delete listen_watcher;
    listen_watcher =NULL;
    free( state_server_loop );
    state_server_loop = NULL;
    log_module( LOG_DEBUG,"STATE_SERVER_ENTRY DONE","ID:%u",pthread_self() );
    return NULL;
}



bool start_by_pthread( ssize_t listen_fd )
{
    pthread_t tid;
    pthread_attr_t thread_attr;
    pthread_attr_init( &thread_attr );
    pthread_attr_setdetachstate( &thread_attr, PTHREAD_CREATE_DETACHED );
    int ret = pthread_create( &tid, &thread_attr, state_server_entry, (void *)listen_fd );
    if( ret == -1 )
    {
         log_module( LOG_INFO, "STATE_SERVER", "START_BY_PTHREAD FAILED:%s", LOG_LOCATION );
         return false;
    }
    return true;
}



void accept_request_cb( struct ev_loop * state_server_loop, struct ev_io *listen_watcher, int revents )
{
    log_module( LOG_DEBUG,"STATE SERVER","ACCEPT_REQUEST_CB START" );
    struct sockaddr_in client_addr;
    socklen_t len = sizeof( struct sockaddr_in );
 
    if( EV_ERROR & revents )
    {
        log_module( LOG_ERROR,"STATE SERVER","LIBEV ERROR FOR EV_ERROR:%d--%s",EV_ERROR,LOG_LOCATION);
        return;
    }
    
    int client_fd = accept( listen_watcher->fd, ( struct sockaddr *)&client_addr, &len );
    if( client_fd < 0 )
    {
        log_module( LOG_ERROR, "STATE SERVER", "ACCEPT ERROR:%s--%s", strerror(errno), LOG_LOCATION );
        return;
    }

    log_module( LOG_DEBUG,"STATE SERVER","CLIENT %s:%d CONNECTED SOCK FD IS:%d", 
                                inet_ntoa( client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd );
    //register the socket io events for reading
    struct ev_io * receive_request_watcher = new struct ev_io;
    
    if( receive_request_watcher == NULL )
    {
         close( client_fd );
         log_module( LOG_ERROR, "STATE SERVER", "ALLOCATE MEMORY FAILED:%s--%s", strerror(errno), LOG_LOCATION );
         return;
    }

    sdk_set_nonblocking( client_fd );
    ev_io_init(  receive_request_watcher, receive_cb, client_fd, EV_READ );
    ev_io_start(  state_server_loop, receive_request_watcher );
    log_module( LOG_DEBUG, "STATE SERVER", "ACCEPT_CB DONE" );
    
}


void receive_cb( struct ev_loop *state_server_loop, struct ev_io * receive_request_watcher, int revents )
{
    #define DO_STATE_SERVER_RECEIVE_CLEAN() \
    ev_io_stop( state_server_loop,receive_request_watcher );\
    delete receive_request_watcher;\
    receive_request_watcher = NULL;\
    return;
          
    if( EV_ERROR & revents )
    {
        log_module( LOG_ERROR,"STATE SERVER","RECEIVE_CB:ERROR FOR EV_ERROR:%s",LOG_LOCATION );
        close( receive_request_watcher->fd );
        DO_STATE_SERVER_RECEIVE_CLEAN();
    }

    //HTTP_REQUEST_INFO req_info;
    char http_request[1024];
    memset( http_request, 0, sizeof( http_request ));
    int received_bytes = read_rosehttp_header( receive_request_watcher->fd,http_request,sizeof(http_request) );
    if( received_bytes <= 0 )
    {     
          if( received_bytes == 0 )
          {
              log_module( LOG_INFO,"STATE SERVER","RECEIVE_CB ERROR:READ 0 BYTE CLIENT DISCONNECTED" );
          }
          else if( received_bytes == LENGTH_OVERFLOW )
          {
              log_module( LOG_ERROR, "STATE SERVER", "REQUESTED HTTP HEADER IS TOO LONG" );
          }
          close( receive_request_watcher->fd );
          DO_STATE_SERVER_RECEIVE_CLEAN();
    }
    
    http_request[received_bytes]='\0';
    log_module( LOG_DEBUG,"STATE SERVER", "HTTP REQUEST IS:%s", http_request );

    std::string http_header( http_request );
    if( (http_header.find("GET") == std::string::npos && http_header.find("get") == std::string::npos )
        || http_header.find("streamer") == std::string::npos
        || http_header.find("stat.do") == std::string::npos 
      )
      
    {
        log_module( LOG_ERROR, "STATE SERVER","INVALID HTTP REQUEST:%s", (http_header.substr(0,http_header.find("\r\n"))).c_str());
        const char *http_400_badrequest="HTTP/1.1 400 Bad Request\r\n\r\n";
        write_specify_size2( receive_request_watcher->fd, http_400_badrequest, strlen(http_400_badrequest) );
        close( receive_request_watcher->fd );
        DO_STATE_SERVER_RECEIVE_CLEAN();
    }
    
    struct ev_io *send_system_info_watcher = new struct ev_io;
    if( send_system_info_watcher == NULL )
    {
            log_module( LOG_INFO,"STATE SERVER","RECEIVE_CB:FAILED TO ALLOCATE MEMORY:%s", LOG_LOCATION );
            const char *http_500_internal="HTTP/1.1 500 Internal Server Error\r\n\r\n";
            write_specify_size2( receive_request_watcher->fd, http_500_internal, strlen( http_500_internal ) );
            close( receive_request_watcher->fd );
            DO_STATE_SERVER_RECEIVE_CLEAN();
            return;
    }
        
    ev_io_init( send_system_info_watcher, send_system_info_cb, receive_request_watcher->fd, EV_WRITE );
    ev_io_start( state_server_loop,send_system_info_watcher );
    DO_STATE_SERVER_RECEIVE_CLEAN();
}


void send_system_info_cb(struct ev_loop *state_server_loop, struct ev_io * send_system_info_watcher, int revents )
{
    #define DO_SEND_SYSTEM_INFO_CLEAN() \
    close( send_system_info_watcher->fd );\
    ev_io_stop( state_server_loop,send_system_info_watcher );\
    delete send_system_info_watcher;\
    send_system_info_watcher = NULL;\
    return;
        
    if( EV_ERROR & revents )
    {
        log_module( LOG_ERROR,"STATE SERVER","SEND_SYSTEM_INFO_CB:ERROR OCCURRED FOR EV_ERROR %s",LOG_LOCATION);
        DO_SEND_SYSTEM_INFO_CLEAN();
    }

    double cpu_occupy = SI.get_cpu_occupy();
    double mem_occupy = SI.get_mem_occupy();
    NET_INFO net_info;
    bool ok = SI.get_net_occupy(net_info);
    
    if( cpu_occupy < 0 || mem_occupy < 0 ||!ok )
    {
         const char * http_200_ok = "HTTP/1.1 200 OK\r\nContent-type:application/json\r\n\r\n{\"code\":10006,\"message\":\"server error\"}";
         write_specify_size2(send_system_info_watcher->fd,http_200_ok,strlen(http_200_ok));
         DO_SEND_SYSTEM_INFO_CLEAN();
    }

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    std::vector<CHANNEL_POOL> channel_pool = get_channel_list();
    int total_channels = channel_pool.size();
    int total_viewers = get_all_online_viewers();
    int cameras = 0;
    int resources = 0;

    
    json_object_set_string(root_object, "Company", "SWWY");
    json_object_set_string(root_object, "Author", "ChenZhengQiang");
    json_object_set_string(root_object, "Date", "2015/5/12");
    json_object_set_string(root_object, "Version", "v1.7.1");
    json_object_set_number(root_object,"cpu_occupy", cpu_occupy);
    json_object_set_number(root_object, "mem_occupy", mem_occupy);
    json_object_dotset_number(root_object,"net.received_bytes",net_info.received_bytes);
    json_object_dotset_number(root_object,"net.transmited_bytes",net_info.transmited_bytes);
    json_object_dotset_number(root_object,"net.received_packets",net_info.received_packets);
    json_object_dotset_number(root_object,"net.transmited_packets",net_info.transmited_packets);
    json_object_dotset_number(root_object, "viewers.total",total_viewers);
    json_object_dotset_number(root_object,"channels.total",total_channels);

    std::ostringstream OSS_channels,OSS_viewers,OSS_cameras,OSS_resources;
    std::vector<CHANNEL_POOL>::iterator cur = channel_pool.begin();
    OSS_channels<<"[";
    OSS_cameras<<"[";
    OSS_resources<<"[";
    while( cur != channel_pool.end() )
    {
        if( cur->is_camera )
        {
            ++cameras;
            OSS_cameras<<"\"{"<<cur->channel<<":"<<cur->IP<<"}"<<"\",";
        }
        else
        {
            ++resources;
            OSS_resources<<"\"{"<<cur->channel<<":"<<cur->IP<<"}"<<"\",";
        }
        OSS_viewers<<"channels.viewers."<<cur->channel;
        json_object_dotset_number(root_object,OSS_viewers.str().c_str(),get_channel_viewers(cur->channel));
        OSS_viewers.str("");
        OSS_viewers.clear();
        OSS_channels<<"\""<<cur->channel<<"\",";
        ++cur;
    }
    
    OSS_cameras<<"\"\"";
    OSS_channels<<"\"\"";
    OSS_resources<<"\"\"";
    OSS_channels<<"]";
    OSS_cameras<<"]";
    OSS_resources<<"]";
    json_object_dotset_value( root_object,"channels.set",json_parse_string(OSS_channels.str().c_str()));
    json_object_dotset_number( root_object,"cameras.total",cameras);
    json_object_dotset_value( root_object,"cameras.set",json_parse_string(OSS_cameras.str().c_str()));

    json_object_dotset_number(root_object,"resources.total",resources);
    json_object_dotset_value(root_object,"resources.set",json_parse_string(OSS_resources.str().c_str()));

    serialized_string = json_serialize_to_string(root_value);
    const char * http_200_ok = "HTTP/1.1 200 OK\r\nContent-type:application/json\r\n\r\n";
    write_specify_size2( send_system_info_watcher->fd,http_200_ok,strlen(http_200_ok) );
    write_specify_size2( send_system_info_watcher->fd,serialized_string,strlen(serialized_string) );

    json_free_serialized_string( serialized_string );
    json_value_free( root_value );
    root_value = NULL;
    log_module( LOG_DEBUG, "STATE SERVER","STREAMER STAT INFO ALREADY SENT,SOCK FD:%d CLOSED", send_system_info_watcher->fd );
    DO_SEND_SYSTEM_INFO_CLEAN();
}


bool startup_state_server( ssize_t listen_fd )
{
    log_module( LOG_DEBUG, "STATE SERVER" , "STARTUP_STATE_SERVER LISTEN FD:%d" ,listen_fd );
    //int listen_fd = register_state_server(NULL,"state_server");
    if( listen_fd < 0 )
    {
        log_module( LOG_ERROR, "STATE SERVER", "STARTUP_STATE_SERVER", "REGISTER_STATE_SERVER:FAILED" );
        return false;
    }
    
    start_by_pthread( listen_fd );
    log_module( LOG_DEBUG,"STATE SERVER","STARTUP_STATE_SERVER DONE",LOG_LOCATION);
    return true;
}

