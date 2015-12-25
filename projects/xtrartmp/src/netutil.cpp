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
    		int listen_fd=-1;
    		int REUSEADDR_ON=1;
    		struct sockaddr_in server_addr;
    		if (( listen_fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0)
    		{
        		return SYSTEM_ERROR;
    		}

    		int ret=setsockopt( listen_fd, SOL_SOCKET, SO_REUSEADDR, &REUSEADDR_ON, sizeof(REUSEADDR_ON) );
    		if ( ret != 0 )
    		{
        		return SYSTEM_ERROR;
    		}

    		bzero(server_addr.sin_zero, sizeof(server_addr.sin_zero));
    		server_addr.sin_family = AF_INET;
    		server_addr.sin_port = htons(PORT);
    		server_addr.sin_addr.s_addr = inet_addr(IP);
    		if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    		{
        		return SYSTEM_ERROR;
    		}

    		if (listen(listen_fd, LISTENQ) < 0)
    		{
        		return SYSTEM_ERROR;
    		}

    		return listen_fd;
	}
}
