/*
*@filename:timerlist.h
*@author:chenzhengqiang
*@start date:2016/01/29 10:00:01
*@modified date:
*@desc: 
*/



#ifndef _CZQ_TIMERLIST_H_
#define _CZQ_TIMERLIST_H_
#include<time.h>
#include<arpa/inet.h>
//write the function prototypes or the declaration of variables here
namespace czq
{
	namespace ServerUtil
	{
		enum{DATA_SIZE=64};

		struct UtilTimer;
		struct ClientData
		{
			struct sockaddr_in sockAddress;
			int sockFd;
			char data[DATA_SIZE];
			UtilTimer * utilTimer;
		};
		
		struct UtilTimer
		{
			UtilTimer():prev(0), next(0){}
		};
	};
};
#endif
