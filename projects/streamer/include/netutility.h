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



struct CMD_OPTIONS
{
    bool run_as_daemon;
    bool need_print_help;
    bool need_print_version;
    std::string config_file;	
};

struct CONFIG
{
    std::string version;
    std::string log_file;
    std::string lock_file;
    std::string notify_server_file;	
    int log_level;
    std::string streamer_ip;
    int streamer_port;	
    std::string state_ip;
    int state_port;
    int run_as_daemon;	
};

int register_tcp_server(const char *IP, int PORT );
int register_udp_server(const char *IP, int PORT);
int  tcp_listen(const char *host, const char *service);
int  tcp_connect(const char *IP, int port);
void err_exit( const char *errmsg,...);
void log_info( const char *ident,...);
void make_fd_nonblock( int listen_fd );
void get_src_info( const std::string & src, string & IP, int & PORT);
void print_welcome(CONFIG & config);
void print_error(void);
void read_config( const char *config_file, CONFIG & config);
void read_config2( const char * config_file,std::map<std::string, int> & notify_server_pool );
void handle_cmd_options(int,char **, struct CMD_OPTIONS &);
void print_help(void);
void print_version(const char *version);

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
std::string get_peer_info( int sock_fd );
#endif
