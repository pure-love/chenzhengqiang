/*
*@filename:processpool.h
*@author:chenzhengqiang
*@start date:2016/01/11 09:58:26
*@modified date:
*@desc: 
*/



#ifndef _CZQ_PROCESSPOOL_H_
#define _CZQ_PROCESSPOOL_H_
namespace czq
{
	class process;
	template<typename T>
	class ppool
	{
		private:
			ppool(int listenFd, int processes= MAX_PROCESSES);
			ppool<T> & operator=(const ppool<T>&);
			ppool(const ppool<T>&);
		public:
			
			//singleton pattern as we allknow
			static ppool<T> *getInstance(int listenFd, int processes = MAX_PROCESSES);
			~ppool();

			//run the pool
			void go();
		private:
			int signal2Pipe();
			void letParentGo();
			void letChildrenGo();
		private:
			//the max processes that ppoll supports
			enum{
					//the max epoll events
					MAX_EPOLL_EVENTS = 5,
					//the max processes that pool supports
					MAX_PROCESSES=8,
					//the max users that each sub-process can handle
					USERS_PER_PROCESS = 10000,
					//the max events that the epoll can handle
					MAX_EVENT_NUMBER = 10000
			};

			int listenFd_;
			int processes_;
			int indexOfPool_;
			int epollFd_;
			bool stop_;
			process *subProcesses_;
			static ppool<T>* instance_;
			
	};
};
#include "ppool.hpp"
//write the function prototypes or the declaration of variables here
#endif
