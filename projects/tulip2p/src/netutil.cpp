/*
 *@Author:chenzhengqiang
 *@company:swwy
 *@date:2016/2/1
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
		int registerTcpServer(const char *IP, int PORT)
		{
				int listenFd=-1;
				struct sockaddr_in server_addr;
				if (( listenFd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0)
				{
		    			return SYSTEM_ERROR;
				}

				setReuseAddr(listenFd);
				setNonBlocking(listenFd);
				bzero(server_addr.sin_zero, sizeof(server_addr.sin_zero));
				server_addr.sin_family = AF_INET;
				server_addr.sin_port = htons(static_cast<uint16_t>(PORT));
				server_addr.sin_addr.s_addr = inet_addr(IP);
				if (bind(listenFd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
				{
		    		return SYSTEM_ERROR;
				}

				if (listen(listenFd, 10) < 0)
				{
		    		return SYSTEM_ERROR;
				}

				return listenFd;
		}


		int registerUdpServer(const char *IP, int PORT)
		{
			struct sockaddr_in localAddr;
      			int serverFd;
      			bzero(&localAddr, sizeof(localAddr));

      			localAddr.sin_family = AF_INET;
      			localAddr.sin_addr.s_addr = inet_addr(IP);
      			localAddr.sin_port = htons(static_cast<uint16_t>(PORT));

      			serverFd = socket(AF_INET, SOCK_DGRAM, 0);
      			if ( -1 == serverFd )
      			{
          			return SYSTEM_ERROR;
      			}
      
      			if( -1 == bind(serverFd, (struct sockaddr *)&localAddr, sizeof(
			localAddr)))
      			{
          			return SYSTEM_ERROR;
      			}
      
      			return serverFd;
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

				if (flag == 0)
				{
				OSSPeerInfo<<ip;
				}
				else if (flag == 1)
				{
				OSSPeerInfo<<port;
				}
				else if (flag == 2)
				{
				OSSPeerInfo<<ip<<":"<<port;
				}

				return OSSPeerInfo.str();
		}


		void setReuseAddr(int listenFd )
		{
			int REUSEADDR_ON=1;
			int ret=setsockopt( listenFd, SOL_SOCKET, SO_REUSEADDR, &REUSEADDR_ON, sizeof(REUSEADDR_ON) );
			if ( ret != 0 )
			{
		    		;//do the error log here	
			}
				
		}


		void setNonBlocking(int sockFd)
		{
			int flags = fcntl(sockFd, F_GETFL, 0);
			if (flags < 0)
			{
		    		return;
			}
			fcntl(sockFd, F_SETFL, flags | O_NONBLOCK);
		}

		void setSndBufferSize(int sockFd, unsigned int sndBufferSize)
		{
			setsockopt(sockFd, SOL_SOCKET, SO_SNDBUF, &sndBufferSize, sizeof(sndBufferSize));
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

		ssize_t writeSpecifySize(int fd, const void *buffer, size_t totalBytes)
		{
			size_t      leftBytes;
    			ssize_t     sentBytes;
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
    			return totalBytes;
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
