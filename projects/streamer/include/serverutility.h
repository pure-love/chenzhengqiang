/*
*@file name:serverutility.h
*@author:chenzhengqiang
*@start date:2015/11/23
*/

#ifndef CZQ_SERVERUTILITY_H_
#define CZQ_SERVERUTILITY_H_
#include<string>
#include<map>
using std::string;

struct SERVER_CONFIG
{
	std::map<std::string, std::string> meta;
	std::map<std::string, std::string> server;
	std::string usage;
};


struct CMD_OPTIONS
{
    bool run_as_daemon;
    bool need_print_help;
    bool need_print_version;
    std::string config_file;	
};


void handle_cmd_options( int ARGC, char * * ARGV, struct CMD_OPTIONS & cmd_options );
void read_config( const char * config_file, SERVER_CONFIG & server_config );

#endif
