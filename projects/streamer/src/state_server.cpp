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
#include"streamer.h"
#include"streamerutility.h"
#include"system_info.h"
#include"logging.h"
#include"parson.h"



system_info SI;

int register_state_server( const char *HOST, const char *SERVICE )
{
    log_module( LOG_DEBUG, "STATE_SERVER" , "REGISTER_STATE_SERVER START" );
    int listenfd = tcp_listen( HOST, SERVICE );
    
    if( listenfd == -1 )
    {
        log_module(LOG_INFO,"REGISTER_STATE_SERVER","FAILED: TCP_LISTEN:%s",LOG_LOCATION);
    }
    return listenfd;
    log_module(LOG_DEBUG,"STATE_SERVER","REGISTER_STATE_SERVER DONE");
}



void * state_server_entry( void * args )
{
    //convert the args to long,cause it might be 64 ocet pointer
    int listen_fd = (long)args;
    log_module(LOG_DEBUG,"STATE_SERVER_ENTRY START","ID:%u",pthread_self());
    struct ev_loop *state_server_loop = ev_loop_new( EVBACKEND_SELECT );
    if( state_server_loop == NULL )
    {
        log_module( LOG_ERROR,"STATE_SERVER","EV_LOOP_NEW FAILED:%s",LOG_LOCATION );
        return NULL;
    }
    
    struct ev_io *listen_watcher = new struct ev_io;
    if( listen_watcher == NULL )
    {
        free( state_server_loop );
        state_server_loop = NULL;
        log_module( LOG_ERROR, "STATE_SERVER", "ALLOCATE MEMORY FAILED:%s", LOG_LOCATION );
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



bool start_by_pthread(ssize_t listen_fd )
{
    pthread_t tid;
    pthread_attr_t thread_attr;
    pthread_attr_init( &thread_attr );
    pthread_attr_setdetachstate( &thread_attr, PTHREAD_CREATE_DETACHED );
    int ret = pthread_create( &tid, &thread_attr, state_server_entry, (void *)listen_fd );
    if( ret == -1 )
    {
         log_module(LOG_INFO,"STATE_SERVER","START_BY_PTHREAD FAILED:%s",LOG_LOCATION);
         return false;
    }
    return true;
}



void accept_request_cb(struct ev_loop * state_server_loop, struct ev_io *listen_watcher, int revents )
{
    log_module(LOG_DEBUG,"STATE SERVER","ACCEPT_REQUEST_CB START");
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(struct sockaddr_in);
 
    if( EV_ERROR & revents )
    {
        log_module( LOG_ERROR,"STATE SERVER","LIBEV ERROR FOR EV_ERROR:%d--%s",EV_ERROR,LOG_LOCATION);
        return;
    }
    
    int client_fd = accept( listen_watcher->fd, (struct sockaddr *)&client_addr, &len );
    if( client_fd < 0 )
    {
        log_module( LOG_ERROR, "STATE SERVER", "ACCEPT ERROR:%s--%s", strerror(errno), LOG_LOCATION );
        return;
    }
    
    char *client_ip = new char[INET_ADDRSTRLEN];
    if( client_ip == NULL )
    {
         close( client_fd );
         log_module( LOG_ERROR, "STATE SERVER", "ALLOCATE MEMORY FAILED:%s", LOG_LOCATION );
         return;
    }
    
    inet_ntop( AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN );
    log_module( LOG_DEBUG,"STATE SERVER","ACCEPT_CB:CLIENT %s CONNECTED", client_ip );
    //register the socket io events for reading
    struct ev_io * receive_request_watcher = (struct ev_io *) malloc(sizeof(struct ev_io) );
    
    if( receive_request_watcher == NULL )
    {
         delete [] client_ip;
         client_ip = NULL;
         close( client_fd );
         log_module( LOG_INFO,"STATE SERVER","ALLOCATE MEMORY FAILED:%s--%s", strerror(errno), LOG_LOCATION );
         return;
    }

    sdk_set_nonblocking( client_fd );
    receive_request_watcher->data = (void *)client_ip;
    ev_io_init(  receive_request_watcher, receive_cb, client_fd, EV_READ );
    ev_io_start(  state_server_loop, receive_request_watcher );
    log_module( LOG_DEBUG, "STATE SERVER", "ACCEPT_CB DONE" );
    
}


void receive_cb(struct ev_loop *state_server_loop, struct ev_io * receive_request_watcher, int revents )
{
    #define DO_STATE_SERVER_RECEIVE_CLEAN() \
    ev_io_stop( state_server_loop,receive_request_watcher );\
    delete [] static_cast<char *>( receive_request_watcher->data );\
    receive_request_watcher->data = NULL;\
    delete receive_request_watcher;\
    receive_request_watcher = NULL;\
    return;
          
    log_module( LOG_DEBUG,"STATE SERVER","RECEIVE_CB START");
    if( EV_ERROR & revents )
    {
        log_module( LOG_ERROR,"STATE SERVER","RECEIVE_CB:ERROR FOR EV_ERROR:%s",LOG_LOCATION);
        close( receive_request_watcher->fd );
        DO_STATE_SERVER_RECEIVE_CLEAN();
    }

    //HTTP_REQUEST_INFO req_info;
    char http_request[1024];
    int received_bytes = read_http_header( receive_request_watcher->fd,http_request,sizeof(http_request) );
    if( received_bytes <= 0 )
    {     
          if( received_bytes == 0 )
          {
              log_module( LOG_INFO,"STATE SERVER","RECEIVE_CB ERROR:CLIENT %s DISCONNECTED",
              static_cast<char *>(receive_request_watcher->data) );
          }
          else if( received_bytes == -1 )
          {
              log_module( LOG_ERROR, "STATE SERVER","RECEIVE_CB ERROR--READ_HTTP_HEADER:BUFFER SIZE IS TOO SMALL");
          }
          close( receive_request_watcher->fd );
          DO_STATE_SERVER_RECEIVE_CLEAN();
    }
    
    http_request[received_bytes]='\0';
    log_module( LOG_DEBUG,"STATE SERVER", "RECEIVE_CB--HTTP REQUEST IS:%s", http_request );

    std::string http_header( http_request );
    if( (http_header.find("GET") == std::string::npos && http_header.find("get") == std::string::npos )
        || http_header.find("streamer") == std::string::npos
        || http_header.find("stat.do") == std::string::npos 
      )
      
    {
        log_module( LOG_ERROR, "STATE SERVER","INVALID HTTP REQUEST:%s", http_header.substr(0,http_header.find("\r\n")));
        const char *http_400_badrequest="HTTP/1.1 400 Bad Request\r\n\r\n";
        write_specify_size2(receive_request_watcher->fd, http_400_badrequest, strlen(http_400_badrequest) );
        close( receive_request_watcher->fd );
        DO_STATE_SERVER_RECEIVE_CLEAN();
    }
    
    log_module( LOG_DEBUG,"STATE SERVER","READY TO SEND SYSTEM INFO TO CLIENT:%s",static_cast<char*>(receive_request_watcher->data));
    struct ev_io *send_system_info_watcher = new struct ev_io;
    if( send_system_info_watcher == NULL )
    {
            log_module(LOG_INFO,"STATE SERVER","RECEIVE_CB:Failed To Allocate Memory:%s",LOG_LOCATION );
            const char *http_500_internal="HTTP/1.1 500 Internal Server Error\r\n\r\n";
            write_specify_size2(receive_request_watcher->fd, http_500_internal, strlen( http_500_internal ));
            close( receive_request_watcher->fd );
            DO_STATE_SERVER_RECEIVE_CLEAN();
            return;
    }
        
    ev_io_init( send_system_info_watcher, send_system_info_cb, receive_request_watcher->fd, EV_WRITE );
    ev_io_start( state_server_loop,send_system_info_watcher );
    log_module( LOG_DEBUG, "STATE_SERVER", "RECEIVE_CB DONE" );
    DO_STATE_SERVER_RECEIVE_CLEAN();
}


void send_system_info_cb(struct ev_loop *state_server_loop, struct ev_io * send_system_info_watcher, int revents )
{
    if( EV_ERROR & revents )
    {
        log_module(LOG_INFO,"STATE SERVER","SEND_SYSTEM_INFO_CB:ERROR OCCURRED FOR EV_ERROR %s",LOG_LOCATION);
        close( send_system_info_watcher->fd );
        ev_io_stop( state_server_loop,send_system_info_watcher );
        delete send_system_info_watcher;
        send_system_info_watcher = NULL;
        return;
    }

    double cpu_occupy = SI.get_cpu_occupy();
    double mem_occupy = SI.get_mem_occupy();
    NET_INFO net_info;
    bool ok = SI.get_net_occupy(net_info);
    
    if( cpu_occupy < 0 || mem_occupy < 0 ||!ok )
    {
         const char * http_200_ok = "HTTP/1.1 200 OK\r\nContent-type:application/json\r\n\r\n{\"code\":10006,\"message\":\"server error\"}";
         write_specify_size2(send_system_info_watcher->fd,http_200_ok,strlen(http_200_ok));
         close( send_system_info_watcher->fd );
         ev_io_stop( state_server_loop,send_system_info_watcher );
         delete send_system_info_watcher;
         send_system_info_watcher = NULL;
         return;
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
    close( send_system_info_watcher->fd );
    ev_io_stop( state_server_loop,send_system_info_watcher );
    delete send_system_info_watcher;
    send_system_info_watcher = NULL;

}


bool startup_state_server( ssize_t listen_fd )
{
    log_module( LOG_DEBUG, "STATE SERVER" , "STARTUP_STATE_SERVER START:%s" ,LOG_LOCATION );
    //int listen_fd = register_state_server(NULL,"state_server");
    if( listen_fd < 0 )
    {
        log_module( LOG_ERROR, "STATE SERVER", "STARTUP_STATE_SERVER", "REGISTER_STATE_SERVER:FAILED" );
        return false;
    }
    
    start_by_pthread( listen_fd );
    log_module(LOG_DEBUG,"STATE SERVER","STARTUP_STATE_SERVER DONE:%s",LOG_LOCATION);
    return true;
}

