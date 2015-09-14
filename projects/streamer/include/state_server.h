/****************************************
 * @Author:chenzhengqiang
 * @Company:swwy
 * @date:2015/5/7
 * @desc:providing another server listing on 9090 to handle the
 request of streamer's stat,etc.g:streamer's memory occupy
 cpu occupy and so on
 ****************************************/
#ifndef _STREAMER_STAT_H_
#define _STREAMER_STAT_H_

#include<ev++.h>
#include<cstddef>
int register_state_server();
void accept_request_cb(struct ev_loop *, struct ev_io *, int);
void receive_cb(struct ev_loop *, struct ev_io *, int);
void send_system_info_cb(struct ev_loop *, struct ev_io *, int);
void * state_server_entry( void * args );
bool start_by_pthread();
bool startup_state_server( ssize_t listen_fd );

bool start_by_main_loop();
#endif
