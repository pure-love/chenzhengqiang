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
	int NetUtil::registerTcpServer(const char *IP, int PORT)
	{
    		int listenFd=-1;
    		struct sockaddr_in server_addr;
    		if (( listenFd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0)
    		{
        		return SYSTEM_ERROR;
    		}

    		
		setReuseAddr(listenFd);
		//setNonBlocking(listenFd);
    		bzero(server_addr.sin_zero, sizeof(server_addr.sin_zero));
    		server_addr.sin_family = AF_INET;
    		server_addr.sin_port = htons(PORT);
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
	
	std::string NetUtil::getPeerInfo(int sockFd, int flag)
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

    		if( flag == 0 )
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
	
	void NetUtil::setReuseAddr(int listenFd )
	{
		int REUSEADDR_ON=1;
    		int ret=setsockopt( listenFd, SOL_SOCKET, SO_REUSEADDR, &REUSEADDR_ON, sizeof(REUSEADDR_ON) );
    		if ( ret != 0 )
    		{
        		;//do the error log here	
    		}
    		
	}

	void NetUtil::setNonBlocking(int sockFd)
	{
		int flags = fcntl(sockFd, F_GETFL, 0);
    		if (flags < 0)
    		{
        		return;
    		}
    		fcntl(sockFd, F_SETFL, flags | O_NONBLOCK);
	}

	int NetUtil::readSpecifySize2( int fd, void *buffer, size_t totalBytes)
	{
    		size_t  leftBytes;
    		int receivedBytes;
    		uint8_t *bufferForward;
    		bufferForward = (uint8_t *)buffer;
    		leftBytes = totalBytes;
    		while ( true )
    		{
        		if ( (receivedBytes = read(fd, bufferForward, leftBytes)) <= 0)
        		{
            			if (  receivedBytes < 0 )
            			{
                			if( errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK )
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
        
        		leftBytes -= receivedBytes;
        		if( leftBytes == 0 )
            		break;
        		bufferForward   += receivedBytes;
    		}
    		return (totalBytes-leftBytes);
	}
}
