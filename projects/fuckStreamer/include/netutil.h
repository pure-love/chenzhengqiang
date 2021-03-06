/*
 * @Author:chenzhengqiang
 * @Date:2015/3/16
 * @Modified:
 * @desc:
 */
#ifndef _CZQ_NETUTIL_H_
#define _CZQ_NETUTIL_H_
#include<string>
using std::string;
namespace czq
{
	namespace NetUtil
	{
			int registerTcpServer(const char *IP, int PORT );
			std::string getPeerInfo(int sockFd, int flag=2);
			void setReuseAddr(int listenFd ); 
			void setNonBlocking(int sockFd);
			size_t readSpecifySize2( int fd, void *buffer, size_t totalBytes);
			void setSndBufferSize(int sockFd, unsigned int sndBufferSize);
			ssize_t writeSpecifySize(int fd, const void *buffer, size_t total_bytes);
			ssize_t writeSpecifySize2(int fd, const void *buffer, size_t total_bytes);
	};
}
#endif
