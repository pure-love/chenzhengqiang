/*
 * @author:chenzhengqiang
 * @company:swwy
 * @date:2015/3/26
 * @modified-date:
 * @version:1.0
 * @desc:
 * this is the implementation of the streamer based on http
*/

#include "streamer.h"
#include "netutility.h"

int main( int ARGC, char ** ARGV )
{
    int streamer_listen_fd;
    int state_listen_fd;
    CMD_OPTIONS cmd_options;
    handle_cmd_options( ARGC, ARGV, cmd_options );
    CONFIG config;
    read_config( cmd_options.config_file.c_str(), config );
    if( cmd_options.need_print_help )
    {
        print_help();
    }

    if( cmd_options.need_print_version )
    {
        print_version( config.version.c_str() );
    }

    streamer_listen_fd = register_tcp_server( config.streamer_ip.c_str(), config.streamer_port );
    state_listen_fd = register_tcp_server( config.state_ip.c_str(), config.state_port );
    serve_forever( streamer_listen_fd, state_listen_fd, config );
    return 0;
}	
