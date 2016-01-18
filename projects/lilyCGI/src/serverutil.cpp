/*
*@file name:serverutility
*@author:chenzhengqiang
*@start date:2015/11/23
*@modified date:
*@desc:server's utility functions
*/

#include "serverutil.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
extern int errno;

using std::cerr;
using std::endl;



namespace czq
{
	void ServerUtil::handleCmdOptions( int ARGC, char * * ARGV, CmdOptions & cmdOptions )
	{
    		(void)ARGC;
    		cmdOptions.needPrintHelp = false;
		cmdOptions.needPrintVersion = false;
    		cmdOptions.runAsDaemon = false;
    		cmdOptions.configFile="";

    		++ARGV;
    		while ( *ARGV != NULL )
    		{
        		if ((*ARGV)[0]=='-')
        		{
            			if ( strcmp( *ARGV, "-v" ) == 0 || strcmp( *ARGV, "--version" ) == 0 )
            			{
                			cmdOptions.needPrintVersion = true;
            			}
            			else if ( strcmp( *ARGV, "-h" ) ==0 || strcmp( *ARGV, "--help" ) == 0 )
            			{
                			cmdOptions.needPrintHelp = true;
            			}
            			else if ( strcmp( *ARGV, "-d" ) == 0 || strcmp( *ARGV, "--daemon" ) == 0 )
            			{
                			cmdOptions.runAsDaemon = true;
            			}
            			else if ( strcmp( *ARGV, "-f" ) == 0 || strcmp( *ARGV, "--config-file" ) == 0 )
            			{
                			++ARGV;
                			if ( *ARGV == NULL )
                			{
                    			std::cerr<<"You must enter the config file when you specify the -f or --config-file option"<<std::endl;
                    			exit( EXIT_FAILURE );
                			}
                			cmdOptions.configFile.assign( *ARGV );
            			}
        		}
        		++ARGV;
    		}
	}



	int ServerUtil::setNonBlocking(int fd)
	{
		int flags = fcntl(fd, F_GETFL, 0);
    		if (flags < 0)
    		{
        		return -1;
    		}
    		return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	}

	
	void ServerUtil::readConfig( const char * configFile, ServerConfig & serverConfig )
	{
		std::ifstream ifile( configFile );
		if( ! ifile )
		{
          		std::cerr<<"FAILED TO OPEN CONFIG FILE:"<<configFile<<" ERROR:"<<strerror(errno)<<std::endl;
          		exit( EXIT_FAILURE );
    		}

		std::string line;
		std::string::size_type curPos;
		std::string heading,key,value;
		bool isReadingMeta = false;
		bool isReadingServer = false;
		bool isReadingUsage = false;
		serverConfig.usage="\r";
	
		while( getline( ifile, line ) )
		{
			if( line.empty() )
			continue;
			if( line[0] == '#' )
			continue;
			if( line[0] == '[')
			{
				if( ( curPos = line.find_last_of(']') ) == std::string::npos )
				{
					std::cerr<<"INVALID CONFIG FILE:"<<configFile<<std::endl;
          				exit( EXIT_FAILURE );
				}
			
				heading = line.substr( 1, curPos-1 );
				if( heading == "META" )
				{
					if( isReadingServer )
					{
						isReadingServer = false;
					}
					else if( isReadingUsage )
					{
						isReadingUsage = false;
					}
				
					isReadingMeta = true;
				}
				else if( heading == "SERVER" )
				{
					if( isReadingMeta )
					{
						isReadingMeta =false;
					}
					else if( isReadingUsage )
					{
						isReadingUsage = false;
					}
					isReadingServer = true;
				}
				else if( heading == "USAGE" )
				{
					if( isReadingMeta )
					{
						isReadingMeta = false;
					}
					else if( isReadingServer )
					{
						isReadingServer = false;
					}
					isReadingUsage = true;
				}
			}
			else if( line[0] == ' ' )
			{
				std::cerr<<"INVALID CONFIG FILE:"<<configFile<<std::endl;
          			exit( EXIT_FAILURE );
			}
			else
			{
				if( isReadingMeta || isReadingServer )
				{
					curPos = line.find_first_of('=');
					if( curPos == std::string::npos )
					continue;
				
					key = line.substr( 0, curPos );
					value = line.substr( curPos+1, line.length()-curPos-2 );

					if( isReadingMeta )
					{
						serverConfig.meta.insert( std::make_pair( key, value ) );
					}
					else
					{
						serverConfig.server.insert( std::make_pair( key, value ) );
					}
				}
				else if( isReadingUsage )
				{
					serverConfig.usage+=line+"\n\r";
				}
			}
		}
	}

	void ServerUtil::generateSimpleRandomValue(unsigned char*buffer, unsigned int bufferLen)
	{
		int tmp=0;
		for (unsigned int index = 0; index < bufferLen; ++index )
		{
			buffer[index]=static_cast<unsigned char>(index*tmp+1986 % 256);
			tmp = buffer[index];
		}
	}


	namespace Epoll
	{
		int IOSet(int epollFd, int fd, const EpollEvents & epollEvents, bool nonBlocking)
		{
			//ignore the error handling temporarily
			epoll_event event;
			event.data.fd = fd;
			event.events = epollEvents;
			epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
			if ( nonBlocking )
			{
				ServerUtil::setNonBlocking(fd);
			}
			return 0;
		}


		int IODel(int epollFd, int fd)
		{
			//ignore the error handling temporarily
			epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, 0);
			close( fd );
			return 0;
		}
	};

	namespace Signal
	{
		int registerHandler(int signum, sighandler_t sigHandler, bool restart)
		{
			//ignore the error handling temporarily
			struct sigaction sa;
			sa.sa_handler = sigHandler;
			if ( restart )
			{
				sa.sa_flags |= SA_RESTART;
			}

			sigfillset( &sa.sa_mask );
			sigaction(signum, &sa, NULL);
			return 0;	
		}
	};
	
};
