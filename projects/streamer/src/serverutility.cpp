/*
*@file name:serverutility
*@author:chenzhengqiang
*@start date:2015/11/23
*@modified date:
*@desc:server's utility functions
*/

#include "serverutility.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <errno.h>
extern int errno;

using std::cout;
using std::cerr;
using std::endl;

void print_usage( const SERVER_CONFIG & server_config )
{
    cout<<server_config.usage<<endl;
    exit( EXIT_SUCCESS );
}


void handle_cmd_options( int ARGC, char * * ARGV, struct CMD_OPTIONS & cmd_options )
{
    (void)ARGC;
    cmd_options.need_print_version = false;
    cmd_options.need_print_help = false;
    cmd_options.run_as_daemon = false;
    cmd_options.config_file="";

    ++ARGV;
    while( *ARGV != NULL )
    {
        if((*ARGV)[0]=='-')
        {
            if( strcmp( *ARGV, "-v" ) == 0 || strcmp( *ARGV, "--version" ) == 0 )
            {
                cmd_options.need_print_version = true;
            }
            else if( strcmp( *ARGV, "-h" ) ==0 || strcmp( *ARGV, "--help" ) == 0 )
            {
                cmd_options.need_print_help = true;
            }
            else if( strcmp( *ARGV, "-d" ) == 0 || strcmp( *ARGV, "--daemon" ) == 0 )
            {
                cmd_options.run_as_daemon = true;
            }
            else if( strcmp( *ARGV, "-f" ) == 0 || strcmp( *ARGV, "--config-file" ) == 0 )
            {
                ++ARGV;
                if( *ARGV == NULL )
                {
                    std::cerr<<"You must enter the config file when you specify the -f or --log-file option"<<std::endl;
                    exit( EXIT_FAILURE );
                }
                cmd_options.config_file.assign( *ARGV );
            }
        }
        ++ARGV;
    }

}



/*
 *@args:config_file[IN],config[OUT]
 *@returns:
 *@desc:read config information from config file
 */
void read_config( const char * config_file, SERVER_CONFIG & server_config )
{
	std::ifstream ifile( config_file );
	if( ! ifile )
	{
          std::cerr<<"FAILED TO OPEN CONFIG FILE:"<<config_file<<" ERROR:"<<strerror(errno)<<std::endl;
          exit( EXIT_FAILURE );
    }

	std::string line;
	std::string::size_type cur_pos;
	std::string heading,key,value;
	bool is_reading_meta = false;
	bool is_reading_server = false;
	bool is_reading_usage = false;
	server_config.usage="\r";
	
	while( getline( ifile, line ) )
	{
		if( line.empty() )
		continue;
		if( line[0] == '#' )
		continue;
		if( line[0] == '[')
		{
			if( ( cur_pos = line.find_last_of(']') ) == std::string::npos )
			{
				std::cerr<<"INVALID CONFIG FILE:"<<config_file<<std::endl;
          			exit( EXIT_FAILURE );
			}
			
			heading = line.substr( 1, cur_pos-1 );
			if( heading == "META" )
			{
				if( is_reading_server )
				{
					is_reading_server = false;
				}
				else if( is_reading_usage )
				{
					is_reading_usage = false;
				}
				
				is_reading_meta = true;
			}
			else if( heading == "SERVER" )
			{
				if( is_reading_meta )
				{
					is_reading_meta =false;
				}
				else if( is_reading_usage )
				{
					is_reading_usage = false;
				}
				is_reading_server = true;
			}
			else if( heading == "USAGE" )
			{
				if( is_reading_meta )
				{
					is_reading_meta = false;
				}
				else if( is_reading_server )
				{
					is_reading_server = false;
				}
				is_reading_usage = true;
			}
		}
		else if( line[0] == ' ' )
		{
			std::cerr<<"INVALID CONFIG FILE:"<<config_file<<std::endl;
          		exit( EXIT_FAILURE );
		}
		else
		{
			if( is_reading_meta || is_reading_server )
			{
				cur_pos = line.find_first_of('=');
				if( cur_pos == std::string::npos )
				continue;
				
				key = line.substr( 0, cur_pos );
				value = line.substr( cur_pos+1, line.length()-cur_pos-2 );

				if( is_reading_meta )
				{
					server_config.meta.insert( std::make_pair( key, value ) );
				}
				else
				{
					server_config.server.insert( std::make_pair( key, value ) );
				}
			}
			else if( is_reading_usage )
			{
				server_config.usage+=line+"\n\r";
			}
		}
	}
}

