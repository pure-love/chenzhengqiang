/*
 * @Author:chenzhengqiang
 * @Date:2015/3/16
 * @Modified:
 * @desc:
 */
#ifndef _STREAMER_UTILITY_H_
#define _STREAMER_UTILITY_H_


#include<string>
#include "common.h"
using std::string;

int register_tcp_server(const char *IP, int PORT );
int register_udp_server(const char *IP, int PORT);
int  tcp_listen(const char *host, const char *service);
int  tcp_connect(const char *IP, int port);
std::string get_peer_info( int sock_fd , int flag = 2 );


int read_specify_size( int fd, void *buffer, size_t total_size);
int read_specify_size2( int fd, void *buffer, size_t total_size);

ssize_t   write_specify_size(int fd, const void *buffer, size_t total_bytes);
ssize_t   write_specify_size2(int fd, const void *buffer, size_t total_bytes);

int sdk_set_tcpnodelay(int sd);
int sdk_set_nonblocking(int sd);
int sdk_set_sndbuf(int sd, int size);
int sdk_set_rcvlowat(int sd, int size);
int sdk_set_rcvbuf(int sd, int size);
int sdk_get_sndbuf(int sd);
int sdk_get_rcvbuf(int sd);
void sdk_set_keepalive(int sd);

#endif
