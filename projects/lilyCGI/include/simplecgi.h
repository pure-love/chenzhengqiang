/*
*@filename:simplecgi.h
*@author:chenzhengqiang
*@start date:2016/01/15 09:10:33
*@modified date:
*@desc: 
*/



#ifndef _CZQ_SIMPLECGI_H_
#define _CZQ_SIMPLECGI_H_
//write the function prototypes or the declaration of variables here
#include<stdint.h>
#include<netinet/in.h>
namespace czq
{
	class SimpleCGI
	{
		public:
			void init( int epollFd, int sockFd, const struct sockaddr_in & connAddr);
			void onRead();
			void onWrite();
		private:
			enum{MAX_BUFFER_SIZE=65535};
			static int epollFd_;
			int sockFd_;
			struct sockaddr_in connAddr_;
			uint8_t data_[MAX_BUFFER_SIZE];
			int size_;//indicates the data buffer's size
	};
};
#endif
