/*
 * @Author:chenzhengqiang
 * @Date:2015/3/16
 * @Modified:
 * @desc:
 */
#ifndef _CZQ_NETUTIL_H_
#define _CZQ_NETUTIL_H_
#include "serverutil.h"
#include <string>
using std::string;
namespace czq
{
	namespace NetUtil
	{
		int registerTcpServer(ServerUtil::ServerConfig & serverConfig);
		std::string getPeerInfo(int sockFd, int flag=2);
		int setReuseAddr(int listenFd ); 
		int setNonBlocking(int sockFd);
		int setTcpKeepAlive( int listenFd, int idle=30, int interval=5, int count=3);
		int setTcpNodelay( int listenFd );
		size_t readSpecifySize(int fd, void *buffer, size_t totalBytes);
		size_t readSpecifySize2( int fd, void *buffer, size_t totalBytes);
		int setSendBufferSize(int sockFd, unsigned int sendBufferSize);
		int setRecvBufferSize(int sockFd, unsigned int recvBufferSize);
		ssize_t writeSpecifySize(int fd, const void * buffer, size_t totalBytes);
		ssize_t writeSpecifySize2(int fd, const void *buffer, size_t totalBytes);
	};
}
#endif
