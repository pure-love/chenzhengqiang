/****************************************
 * @Author:chenzhengqiang
 * @date:2015/3/26
 * @desc:
 ****************************************/
#ifndef _STREAMING_SERVER_H_
#define _STREAMING_SERVER_H
#include "common.h"
#include "streamerutility.h"
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

void do_notify( const int & sock_fd, const NOTIFY_DATA & notify_data );
int   register_streamer_server(const char *host,const char *service);
bool init_workthread_info_pool( CONFIG & config, size_t workthreads_size );
void serve_forever(ssize_t streamer_server_fd, ssize_t state_server_fd ,CONFIG & config);
void * workthread_entry( void * args );
void * notify_server_entry( void * args );
void accept_cb(struct ev_loop * main_event_loop, struct ev_io * listen_watcher, int revents );
void receive_request_cb(struct ev_loop *main_event_loop, struct  ev_io *client_watcher, int revents );
void send_tags_cb(struct ev_loop * workthread_loop, struct  ev_io *viewer_watcher, int revents );
void pull_stream_cb(struct ev_loop * workthread_loop, struct  ev_io *pull_stream_watcher, int revents );
void push_stream_cb(struct ev_loop * workthread_loop, struct  ev_io *push_stream_watcher, int revents );
void reply_200OK_cb(struct ev_loop * workthread_loop, struct  ev_io *reply_watcher, int revents );
void send_backing_resource_cb( struct ev_loop * workthread_loop, struct  ev_io *push_stream_watcher, int revents );
void async_read_cb(struct ev_loop *workthread_loop, struct ev_async *async_watcher, int revents );
void send_backing_source_request_cb(struct ev_loop *, struct ev_io *, int);
void receive_stream_cb(struct ev_loop * main_event_loop, struct  ev_io *camera_watcher, int revents );
void workthread_idle_cb(struct ev_loop * workthread_loop, struct ev_idle *idle_watcher, int revents);
bool dispatch_workthread_ok( const std::string & channel );
bool do_camera_request( struct ev_loop* main_event_loop, struct ev_io *receive_request_watcher, HTTP_REQUEST_INFO & req_info );
bool do_viewer_request( struct ev_loop * main_event_loop, struct ev_io * receive_request_watcher, HTTP_REQUEST_INFO & req_info);
bool do_the_backing_source_request( struct ev_loop *,ssize_t request_fd, HTTP_REQUEST_INFO & req_info );
void attach_camera_info_to_workthread( const string & channel );
void parse_flv_stream( const uint8_t* flv_stream, uint32_t received_bytes );
size_t get_all_online_viewers();
size_t  get_channel_viewers( const std::string & channel );
std::vector<CHANNEL_POOL> get_channel_list();
void do_notify_cb( struct ev_loop *workthread_loop, struct ev_io *notify_watcher, int revents );
#endif
