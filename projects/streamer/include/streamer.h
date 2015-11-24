/****************************************
 * @Author:chenzhengqiang
 * @date:2015/3/26
 * @desc:
 ****************************************/
#ifndef _STREAMING_SERVER_H_
#define _STREAMING_SERVER_H
#include "common.h"
#include "serverutility.h"
#include "netutility.h"
#include "rosehttp.h"
#include<pthread.h>
using std::string;


/*
 *@args:host name,service name
 *@returns: return the file descriptor for listening the request of clients
 *@desc:encapsulate the socket api:socket,bind,listen operations
 */

struct CHANNEL_POOL
{
    std::string channel;
    bool is_camera;
    char IP[INET_ADDRSTRLEN];	
};

struct NOTIFY_DATA
{
    char channel[64];
    int flag;//0 stands for start,stop otherwise
};


bool init_workthread_info_pool( size_t workthreads_size );
void serve_forever(ssize_t streamer_server_fd, ssize_t state_server_fd , SERVER_CONFIG & config);
void * workthread_entry( void * args );
void * notify_server_entry( void * args );
void get_notify_servers( const char * config_file,std::map<std::string, int> & notify_server_pool );
void print_help( void );
void print_version( SERVER_CONFIG & server_config );
void print_welcome( int port );
void print_error( void );
void get_src_info( const std::string & src, string & IP, int & PORT );





void accept_cb(struct ev_loop * main_event_loop, struct ev_io * listen_watcher, int revents );
void receive_request_cb(struct ev_loop *main_event_loop, struct  ev_io *client_watcher, int revents );
void send_tags_cb(struct ev_loop * workthread_loop, struct  ev_io *viewer_watcher, int revents );
void pull_stream_cb(struct ev_loop * workthread_loop, struct  ev_io *pull_stream_watcher, int revents );
void push_stream_cb(struct ev_loop * workthread_loop, struct  ev_io *push_stream_watcher, int revents );
void reply_200OK_cb(struct ev_loop * workthread_loop, struct  ev_io *reply_watcher, int revents );
void send_backing_resource_cb( struct ev_loop * workthread_loop, struct  ev_io *push_stream_watcher, int revents );
void async_read_cb(struct ev_loop *workthread_loop, struct ev_async *async_watcher, int revents );
void send_backing_source_request_cb(struct ev_loop *, struct ev_io *, int);
void receive_stream_cb( struct ev_loop * main_event_loop, struct  ev_io *camera_watcher, int revents );
void do_notify_cb( struct ev_loop *workthread_loop, struct ev_io *notify_watcher, int revents );


void do_notify( const int & sock_fd, const NOTIFY_DATA & notify_data );
bool do_camera_request( struct ev_loop* main_event_loop, struct ev_io *receive_request_watcher, const std::string & channel );
bool do_viewer_request( struct ev_loop * main_event_loop, struct ev_io * receive_request_watcher, SIMPLE_ROSEHTTP_HEADER & req_info);
bool do_the_backing_source_request( struct ev_loop *,ssize_t request_fd, SIMPLE_ROSEHTTP_HEADER & req_info );

size_t get_all_online_viewers();
size_t  get_channel_viewers( const std::string & channel );
std::vector<CHANNEL_POOL> get_channel_list();

#endif
