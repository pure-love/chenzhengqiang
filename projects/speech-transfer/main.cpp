/*
*@author:chenzhengqiang
*@company:swwy
*@start-date:2015/6/25
*@desc:the implementation of speech transfer server based on udp
*/

#include "transfer.h"
#include "netutility.h"

int main( int ARGC, char ** ARGV )
{
    CMD_OPTIONS cmd_options;
    handle_cmd_options( ARGC, ARGV ,cmd_options );
    CONFIG config;
    read_config( cmd_options.config_file.c_str(), config );
    int transfer_sock_fd = register_udp_server( config.IP.c_str(),config.PORT );
    serve_forever( transfer_sock_fd, config );
    return 0;
}
