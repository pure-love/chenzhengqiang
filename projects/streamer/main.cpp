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
#include "serverutility.h"


static const char *DEFAULT_CONFIG_FILE="/etc/streamer/streamer.conf";

int main( int ARGC, char ** ARGV )
{
    int streamer_listen_fd;
    int state_listen_fd;
    CMD_OPTIONS cmd_options;
    handle_cmd_options( ARGC, ARGV, cmd_options );
    SERVER_CONFIG config;

    if( ! cmd_options.config_file.empty() )
    {
		read_config( cmd_options.config_file.c_str(), config );
    }
    else
    {
		read_config( DEFAULT_CONFIG_FILE, config );
    }

    if( cmd_options.need_print_help )
    {
        print_help();
    }

    if( cmd_options.need_print_version )
    {
        print_version( config );
    }

    streamer_listen_fd = register_tcp_server( config.server["bind-address"].c_str(), atoi(config.server["bind-port"].c_str()));
    state_listen_fd = register_tcp_server( config.server["bind-address"].c_str(), atoi(config.server["state-server-port"].c_str()));
    serve_forever( streamer_listen_fd, state_listen_fd, config );
    return 0;
}	
