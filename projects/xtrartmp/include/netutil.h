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
	class NetUtil
	{
		public:
			static int registerTcpServer(const char *IP, int PORT );
			static std::string getPeerInfo(int sockFd, int flag=2);
			static void setReuseAddr(int listenFd ); 
			static void setNonBlocking(int sockFd);
			static size_t readSpecifySize2( int fd, void *buffer, size_t totalBytes);
			static void setSndBufferSize(int sockFd, unsigned int sndBufferSize);
			static ssize_t writeSpecifySize2(int fd, const void *buffer, size_t total_bytes);
	};
}
#endif
