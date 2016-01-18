/*
*@filename:simplecgi.cpp
*@author:chenzhengqiang
*@start date:2016/01/15 09:10:33
*@modified date:
*@desc: 
*/



#include "lilycgi.h"
#include <cstring>
namespace czq
{
	int SimpleCGI::epollFd_ = -1;
	void SimpleCGI::init(int epollFd, int sockFd, const struct sockaddr_in & connAddr)
	{
		epollFd_ = epollFd;
		sockFd_ = sockFd;
		connAddr_ = connAddr;
		memset(data_, 0, MAX_BUFFER_SIZE);
		size_ = 0;
	}



	void SimpleCGI::onRead()
	{
		#define ToOnRead __func__
		while ( true )
		{
			;//do read the data here
		}
	}


	void SimpleCGI::onWrite()
	{
		#define ToOnWrite __func__
		while ( true )
		{
			;//do write here
		}
	}
};
