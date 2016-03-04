/*
 *@Author:chenzhengqiang
 *@company:swwy
 *@date:2015/12/23
 *@modified:
 *@desc:
 @version:1.0
 */

#include "errors.h"
#include "common.h"
#include "netutil.h"


namespace czq
{
	namespace NetUtil
	{
		int registerTcpServer(ServerUtil::ServerConfig & serverConfig)
		{
			int listenFd=-1;
			struct sockaddr_in serverAddr;
			if (( listenFd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0)
			{
	    			return MISS_SYSTEM_ERROR;
			}

			setReuseAddr(listenFd);
			if ( serverConfig.server["non-blocking"].empty() || serverConfig.server["non-blocking"]=="yes" )
			{
				setNonBlocking(listenFd);
			}

			if ( serverConfig.server["nodelay"] == "yes" )
			{
				setTcpNodelay(listenFd);
			}
			
			if ( serverConfig.server["keep-alive"] =="yes" )
			{
				int keepIdle = serverConfig.server["keep-idle"].empty() ? 
								 30:atoi(serverConfig.server["keep-idle"].c_str());
				int keepInterval = serverConfig.server["keep-interval"].empty() ? 
								 5:atoi(serverConfig.server["keep-interval"].c_str());
				int keepCount = serverConfig.server["keep-count"].empty() ? 
								 3:atoi(serverConfig.server["keep-count"].c_str());
				
				setTcpKeepAlive(listenFd, keepIdle, keepInterval, keepCount);
			}


			if ( !serverConfig.server["recv-buffer-size"].empty() )
			{
				int recvBufferSize = atoi(serverConfig.server["recv-buffer-size"].c_str());
				if ( recvBufferSize >= 0 )
				{
					setRecvBufferSize(listenFd, recvBufferSize);
				}
			}

			if ( !serverConfig.server["send-buffer-size"].empty() )
			{
				int sendBufferSize = atoi(serverConfig.server["send-buffer-size"].c_str());
				if ( sendBufferSize >= 0 )
				{
					setSendBufferSize(listenFd, sendBufferSize);
				}
			}

			
			bzero(serverAddr.sin_zero, sizeof(serverAddr.sin_zero));
			serverAddr.sin_family = AF_INET;

			
			if ( serverConfig.server["bind-port"].empty() )
			{
				return MISS_CONFIG_ERROR;
			}

			int bindPort = atoi(serverConfig.server["bind-port"].c_str());
			if ( bindPort <= 0 || bindPort > 65535 )
			{
				return MISS_CONFIG_ERROR;
			}
			
			serverAddr.sin_port = htons(static_cast<uint16_t>(bindPort));
			serverAddr.sin_addr.s_addr = inet_addr(serverConfig.server["bind-address"].empty() ? "0.0.0.0":
											    serverConfig.server["bind-address"].c_str());
												
			if (bind(listenFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
			{
	    			return MISS_SYSTEM_ERROR;
			}

			int backlog = atoi(serverConfig.server["backlog"].c_str());
			if ( backlog <= 0 )
			{
				backlog = 10;
			}
			
			if ( listen(listenFd, backlog ) < 0)
			{
	    			return MISS_SYSTEM_ERROR;
			}

			return listenFd;
		}


		std::string getPeerInfo(int sockFd, int flag)
		{
			std::ostringstream OSSPeerInfo;
			if ( sockFd < 0 )    
			return OSSPeerInfo.str();
			struct sockaddr_in peer_addr;
			socklen_t sock_len = sizeof( peer_addr );
			int ret = getpeername( sockFd, (struct sockaddr *)&peer_addr, &sock_len );
			if ( ret != 0 )
			return OSSPeerInfo.str();

			char *ip = inet_ntoa( peer_addr.sin_addr );
			int port = htons( peer_addr.sin_port );

			if ( flag == 0 )
			{
				OSSPeerInfo<<ip;
			}
			else if ( flag == 1 )
			{
				OSSPeerInfo<<port;
			}
			else if ( flag == 2 )
			{
				OSSPeerInfo<<ip<<":"<<port;
			}
			return OSSPeerInfo.str();
		}


		int setReuseAddr( int listenFd )
		{
			int reuse=1;
			return setsockopt( listenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse) );	
		}


		int setTcpNodelay( int listenFd )
		{
			int nodelay;
			socklen_t len;
			nodelay = 1;
			len = sizeof(nodelay);
			return setsockopt(listenFd, IPPROTO_TCP, TCP_NODELAY, &nodelay, len);
		}

		
		int setTcpKeepAlive( int listenFd, int idle, int interval, int count)
		{
    			int keepalive =1;
    			setsockopt(listenFd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepalive ,sizeof(keepalive ));
    			setsockopt(listenFd, SOL_TCP, TCP_KEEPIDLE, (void*)&idle ,sizeof(idle ));
    			setsockopt(listenFd, SOL_TCP, TCP_KEEPINTVL, (void*)&interval ,sizeof(interval ));
    			setsockopt(listenFd, SOL_TCP, TCP_KEEPCNT, (void*)&count ,sizeof(count ));
			return MISS_OK;	
		}

		
		int setNonBlocking(int sockFd)
		{
			int flags = fcntl(sockFd, F_GETFL, 0);
			return fcntl(sockFd, F_SETFL, flags | O_NONBLOCK);
		}

		int setSendBufferSize(int sockFd, unsigned int sendBufferSize)
		{
			return setsockopt(sockFd, SOL_SOCKET, SO_SNDBUF, &sendBufferSize, sizeof(sendBufferSize));
		}

		int setRecvBufferSize(int sockFd, unsigned int recvBufferSize)
		{
			return setsockopt(sockFd, SOL_SOCKET, SO_RCVBUF, &recvBufferSize, sizeof(recvBufferSize));
		}

		
		size_t readSpecifySize( int fd, void *buffer, size_t totalBytes)
		{
    			size_t  leftBytes;
    			ssize_t receivedBytes;
    			uint8_t *bufferForward;
    			bufferForward = (uint8_t *)buffer;
    			leftBytes = totalBytes;

    			while ( true )
    			{
        			if ( (receivedBytes = read(fd, bufferForward, leftBytes)) <= 0)
        			{
            				if (  receivedBytes < 0 )
            				{
                				if( errno == EINTR )
                				{
                    				receivedBytes = 0;
                				}
                				else if( errno == EAGAIN || errno == EWOULDBLOCK )
                				{
		        				if( (totalBytes-leftBytes) == 0 )
		        				continue;		
		        				return ( totalBytes-leftBytes );
                				}
                				else
		   				{
                     				return 0;
		   				}
            				}
            				else
	     				{
                 				return 0; // it indicates the camera has  stoped to push 
	     				}
        			}
        
        			leftBytes -= receivedBytes;
        			if( leftBytes == 0 )
            			break;
        			bufferForward   += receivedBytes;
    			}

    			return ( totalBytes-leftBytes );
		}

		
		size_t readSpecifySize2( int fd, void *buffer, size_t totalBytes)
		{
			size_t  leftBytes;
			ssize_t receivedBytes;
			uint8_t *bufferForward;
			bufferForward = (uint8_t *)buffer;
			leftBytes = totalBytes;
			while (true)
			{
		    		if ( (receivedBytes = read(fd, bufferForward, leftBytes)) <= 0)
		    		{
		        		if ( receivedBytes < 0)
		        		{
		            			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
		            			{
		                			receivedBytes = 0;
		            			}
		            			else
		            			{
		                		
		                			return 0;
		            			}
		        		}
		        		else
		        		{
		            			return 0; // it indicates the camera has  stoped to push 
		        		}
		    		}
		    
		    		leftBytes -= static_cast<size_t>(receivedBytes);
		    		if (leftBytes == 0)
		        		break;
		    		bufferForward   += receivedBytes;
			}
			return (totalBytes-leftBytes);
		}


		ssize_t   writeSpecifySize(int fd, const void *buffer, size_t totalBytes)
		{
    			size_t      leftBytes;
    			ssize_t    sentBytes;
    			const uint8_t *bufferForward= (const uint8_t *)buffer;
    			leftBytes = totalBytes;

			while ( true )
    			{
        			if ( (sentBytes = write( fd, bufferForward, leftBytes)) <= 0)
        			{
            				if ( sentBytes < 0 )
            				{
                				if( errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK )
                				{
                    				return totalBytes-leftBytes;
                				}
                				else
                				{
                    				return -1;
                				}
            				}
            				else
            				{
                				return totalBytes-leftBytes;
            				}

        			}
        
        			leftBytes -= sentBytes;
        			if( leftBytes == 0 )
            			break;
        			bufferForward  += sentBytes;
    			}
    			return(totalBytes);
		}


		
		ssize_t  writeSpecifySize2(int fd, const void *buffer, size_t totalBytes)
		{
			size_t      leftBytes;
			ssize_t     sentBytes;
			const uint8_t *bufferForward= (const uint8_t *)buffer;
			leftBytes = totalBytes;
			while ( true )
			{
	    			if ( (sentBytes = write(fd, bufferForward, leftBytes)) <= 0)
	    			{
	        			if ( sentBytes < 0 )
	        			{
	            				if ( errno == EINTR  || errno == EAGAIN || errno == EWOULDBLOCK )
	            				{
	                				sentBytes = 0;
	            				}
	            				else
	            				{
	                				return -1;
	            				}
	        			}
	        			else
	            			return -1;
	    			}
				
	    			leftBytes -= sentBytes;
	    			if ( leftBytes == 0 )
	        		break;
	    			bufferForward   += sentBytes;
			}
			return(totalBytes);
		}
	};
}
