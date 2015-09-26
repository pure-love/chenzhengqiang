/*
@file name:main.cpp
@author:chenzhengqiang
@start date:2015/9/16
@modified date:
@desc:unit test for the entire hls project
    :receive the flv stream pushed from streamer server,
    then parsed it tag by tag.when you get the tag,just
    calling the api to encapsulate the es stream into ts
*/

#include "netutility.h"
#include "happy_hls.h"
int main( int argc, char ** argv )
{
    int hls_listen_fd;
    CMD_OPTIONS cmd_options;
    handle_cmd_options(argc,argv,cmd_options);
    CONFIG config;
    read_config(cmd_options.config_file.c_str(),config);
    hls_listen_fd = register_tcp_server(config.IP.c_str(),config.PORT);
    serve_forever(hls_listen_fd,config);
    return 0;
}
