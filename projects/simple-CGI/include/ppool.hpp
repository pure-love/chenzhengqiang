/*
*@filename:processpool.cpp
*@author:chenzhengqiang
*@start date:2016/01/11 09:58:26
*@modified date:
*@desc: 
*/


#include "netutil.h"
#include "process.h"
#include "serverutil.h"
#include<stdexcept>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<errno.h>
extern int errno;


namespace czq
{
	//the global pipe fd for converting signal event to epoll event
	static int signalPipeFd[2];
	//for converting signal event to epoll event
	 static void sigHandler(int signo)
	{
		//just send the signal to pipe
		send(signalPipeFd[1], (char *)&signo, 1, 0);
	}

	
	template<typename T>
	ppool<T>* ppool<T>::instance_ = 0;	
	
	template<typename T>
	ppool<T> * ppool<T>::getInstance(int listenFd, int processes)
	{
		if ( instance_ == 0 )
		{
			instance_ = new ppool<T>(listenFd, processes);
		}
		return instance_;
	}


	template<typename T>
	ppool<T>::ppool(int listenFd, int processes)
	:listenFd_(listenFd), processes_(processes),
	indexOfPool_(-1),stop_(false)
	{
		if ( listenFd <0 || processes < 0 || processes > MAX_PROCESSES )
		{
			;
		}
		subProcesses_ = new process[processes];
		if (subProcesses_ == 0 )
		{
			;
		}

		for (int index = 0; index < processes; ++index)
		{
			int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, subProcesses_[index].pipeFd_);
			if ( ret != 0 )
			{
				;//do something here
			}
			
			subProcesses_[index].pid_ = fork();
			if ( subProcesses_[index].pid_ > 0 )
			{
				//come in the parent's envirenment
				close(subProcesses_[index].pipeFd_[1]);
			}
			else if ( subProcesses_[index].pid_ == 0 )
			{
				//the child's envirenment
				close(subProcesses_[index].pipeFd_[0]);
				indexOfPool_ = index;
				break;
			}
		}
	}


	
	template<typename T>
	ppool<T>::~ppool()
	{
		if ( subProcesses_ != 0 )
		delete [] subProcesses_;	
	}


	template<typename T>
	int ppool<T>::signal2Pipe()
	{
		epollFd_ = epoll_create(MAX_EPOLL_EVENTS);
		if ( epollFd_ == -1 )
		{
			;//do something here
		}

		int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, signalPipeFd);
		if ( ret != 0 )
		{
			;//do something here
		}

		ServerUtil::setNonBlocking(signalPipeFd[1]);
		Epoll::IOSet(epollFd_, signalPipeFd[0], Epoll::READ);

		Signal::registerHandler(SIGCHLD, sigHandler);
		Signal::registerHandler(SIGTERM, sigHandler);
		Signal::registerHandler(SIGINT, sigHandler);
		Signal::registerHandler(SIGPIPE, SIG_IGN);
		
		return 0;
	}


	template<typename T>
	void ppool<T>::go()
	{
		if ( indexOfPool_ != -1 )
		{
			letChildrenGo();
			return;
		}
		letParentGo();
	}



	template<typename T>
	void ppool<T>::letChildrenGo()
	{
		signal2Pipe();
		int pipeFd = subProcesses_[indexOfPool_].pipeFd_[1];

		//listen to the pipe fd related to parent
		Epoll::IOSet(epollFd_, pipeFd, Epoll::READ);

		epoll_event epollEvents[MAX_EVENT_NUMBER];
		T *users = new T[USERS_PER_PROCESS];
		if ( users == 0 )
		{
			;//do something here
		}

		ssize_t events = 0;
		ssize_t ret = -1;

		while ( ! stop_ )
		{
			events = epoll_wait(epollFd_, epollEvents, MAX_EVENT_NUMBER, -1);
			if ( (events < 0) && (errno != EINTR))
			{
				break;
			}

			for (int event = 0; event < events; ++event)
			{
				int sockFd = epollEvents[event].data.fd;
				if ( (sockFd == pipeFd) && (epollEvents[event].events & EPOLLIN) )
				{
					int client = 0;
					ret = recv(sockFd, (char *)&client, sizeof(client), 0);

					//just accept the connection of client
					struct sockaddr_in connAddr;
					socklen_t addrLen = sizeof(connAddr);
					int connFd = accept(listenFd_, (struct sockaddr *)&connAddr, &addrLen);
					if ( connFd < 0 )
					{
						continue;
					}

					Epoll::IOSet(epollFd_, signalPipeFd[0], Epoll::READ);

					//note that the TYPE T must support the init interface
					//do not forgfet that "T *users = new T[USERS_PER_PROCESS];"
					users[connFd].init(epollFd_, connFd, connAddr);
				}
				else if ( (sockFd == signalPipeFd[0]) && (epollEvents[event].events & EPOLLIN) )
				{
					char signals[1024];
					ret = recv(signalPipeFd[0], signals, sizeof(signals), 0);
					if ( ret <= 0 )
					continue;

					for (int i=0; i < ret; ++i)
					{
						switch (signals[i])
						{
							case SIGCHLD:
								pid_t pid;
								int stat;
								while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
								{
									;
								}
								break;
							case SIGTERM:
							case SIGINT:
								stop_ = true;
								break;
							default:
								break;
						}
					}
				}
				else if ( epollEvents[event].events & EPOLLIN )
				{
					users[sockFd].onRead();
				}
				else if (epollEvents[event].events & EPOLLOUT )
				{
					users[sockFd].onWrite();
				}
				
			}
		}


		delete [] users;
		users = NULL;
		close(pipeFd);
		//close(listenFd_); it's the responsibility of creater to close the listen fd
		close(epollFd_);
	}


	template<typename T>
	void ppool<T>::letParentGo()	
	{
		signal2Pipe();
		Epoll::IOSet(epollFd_, listenFd_, Epoll::READ);
		epoll_event epollEvents[MAX_EVENT_NUMBER];
		int subProcessCounter = 0;
		int newConn = 1;
		int events = 0;

		while ( ! stop_ )
		{
			events = epoll_wait(epollFd_, epollEvents, MAX_EVENT_NUMBER, -1);
			if ( (events < 0) && (errno != EINTR))
			break;

			for (int event = 0; event < events; ++event)
			{
				int sockfd = epollEvents[event].data.fd;
				if ( sockfd == listenFd_ )
				{
					;//select one child using round robin algorithm
					;//when new client connection coming, send a message through pipe created by socketpair
					int subProcessIndex = subProcessCounter;

					//simple round robin algorithm
					do
					{
						if (subProcesses_[subProcessIndex].pid_ != -1)
						{
							break;
						}
						subProcessIndex = (subProcessIndex+1) % processes_;
					}while(subProcessIndex != subProcessCounter);

					if ( subProcesses_[subProcessIndex].pid_ == -1)
					{
						stop_ = true;
						break;
					}

					//just send a message to child thorugh pipe
					//when child receive the message,just accept the connection of client
					subProcessCounter = (subProcessIndex+1) % processes_;
					send(subProcesses_[subProcessIndex].pipeFd_[0],
						(char *)&newConn, sizeof(newConn), 0);
					
				}
				else if ( (sockfd == signalPipeFd[0]) && epollEvents[event].events & EPOLLIN )
				{
					;//handle the signals especially the SIGCHLD signal
					;//when receive the SIGTERM OR SIGINT,just kill all children
				}
			}
		}

		//close(listenFd_); it's the responsibility of creater to close the listen fd
		close( epollFd_ );
	}
};
