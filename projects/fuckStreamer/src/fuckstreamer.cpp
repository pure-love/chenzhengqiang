/*
*@filename:fuckstreamer.cpp
*@author:chenzhengqiang
*@start date:2016/02/24 09:32:58
*@modified date:
*@desc: 
*/


#include "netutil.h"
#include "serverutil.h"
#include "fuckstreamer.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <sys/time.h>
#include <cstring>
#include <iostream>
#include <errno.h>
#include <stdint.h>
extern int errno;
using std::cout;
using std::endl;

namespace czq
{
	namespace service
	{
		FuckStreamer::FuckStreamer( const ServerUtil::ServerConfig & serverConfig )
		:serverConfig_(serverConfig),performanceLogHandler_(0),first_(0),element_(0),end_(0)
		{
			bzero(buffer_, MAX_BUFFER_SIZE);
			element_ = first_ = buffer_;
			end_ = first_+MAX_BUFFER_SIZE;
			
			streamerIP_ =  (char *)serverConfig_.server["streamer-address"].c_str();
			streamerPort_ = atoi( serverConfig_.server["streamer-port"].c_str());
			concurrency_ = atoi(serverConfig_.server["concurrency"].c_str());
			totalConcurrency_ = concurrency_;
			int ret = sprintf(first_, "Server IP:%s\nServer Port:%d\nConfigured Concurrency:%d\n", 
							      streamerIP_, streamerPort_, concurrency_);
			if ( ret <= 0 )
			{
				std::cerr<<"sprintf error:"<<strerror(errno)<<endl;
				exit(1);
			}

			cout<<first_;	
			element_+=ret;
			performanceLogHandler_ = fopen(serverConfig_.server["performance-output-file"].c_str(), "w");
			if ( performanceLogHandler_ != 0 )
			{
				epollFd_ = epoll_create(concurrency_);
				initConn();
			}
			else
			{
				std::cerr<<"open the performance log file failed!"<<std::endl;
				exit(1);
			}
		}

	
		void FuckStreamer::printHelp()
		{
			std::cout<<serverConfig_.usage<<std::endl;
			exit(EXIT_SUCCESS);
		}


		void FuckStreamer::printVersion()
		{
			std::cout<<serverConfig_.meta["version"]<<std::endl<<std::endl;
			exit(EXIT_SUCCESS);
		}


		void FuckStreamer::serveForever()
		{
			epoll_event events[10000];

			int elapsed = 0;
			int duration = atoi(serverConfig_.server["duration"].c_str()) * 60;
			time_t prev = time(NULL);
			cout<<"Duration:"<<duration<<" Seconds"<<endl;
			uint64_t totalSentBytes = 0;
			while ( elapsed <= duration )
			{
				int completeEvents = epoll_wait(epollFd_, events, sizeof(events), -1);
				for ( int index = 0; index < completeEvents; ++index)
				{
					int sockFd = events[index].data.fd;
					std::map<int, ConnInfo>::iterator cciter = ConnInfoPool_.find(sockFd);
					if ( events[index].events & EPOLLOUT )
					{
						if ( cciter != ConnInfoPool_.end() )
						{
							if ( cciter->second.first )
							{
								char post[50]={0};
								sprintf(post,"POST MLGB/live?channel=%d HTTP/1.1\r\n\r\n", sockFd);
								cciter->second.first = false;
								NetUtil::writeSpecifySize2(sockFd, post, strlen(post));
								totalSentBytes += strlen(post);
							}
							else
							{
								if ( cciter->second.sentBytes == sizeof(cciter->second.stream) )
								{
									cciter->second.sentBytes = 0;
									ssize_t ret = fread(cciter->second.stream, 
													sizeof(cciter->second.stream), 
													sizeof(char), cciter->second.handler);
									if ( ret <=0 )
									{
										totalConcurrency_ -=1;
										fclose(cciter->second.handler);
										Epoll::IODel(epollFd_, sockFd);
										ConnInfoPool_.erase(cciter);
										
									}
								}
								
								ssize_t ret = NetUtil::writeSpecifySize(sockFd,  cciter->second.stream+
																		cciter->second.sentBytes, 
																		sizeof(cciter->second.stream)-cciter->second.sentBytes);
								if ( ret != -1 )
								{
									totalSentBytes += ret;
									cciter->second.sentBytes += (int)ret;
								}
								else
								{
									totalConcurrency_ -=1;
									fclose(cciter->second.handler);
									Epoll::IODel(epollFd_, sockFd);
									ConnInfoPool_.erase(cciter);
								}
							}
						}
					}
					else if ( events[index].events & EPOLLERR )
					{
						totalConcurrency_ -=1;
						fclose(cciter->second.handler);
						Epoll::IODel(epollFd_, sockFd);
						ConnInfoPool_.erase(cciter);
					}
				}

				time_t now = time(NULL);
				elapsed = now-prev;
			}

			sprintf(element_, "Duration:%d Seconds\nTotal Concurrency:%d\n"\
						     "Total Sent:%lu (bytes)\nAverage Bitrate:%d Kbit/s", 
							duration, totalConcurrency_, totalSentBytes, totalSentBytes*8 /1024 /duration/totalConcurrency_);
			fwrite(first_, strlen(first_), sizeof(char), performanceLogHandler_);
			fclose(performanceLogHandler_);
			performanceLogHandler_ = 0;
		}


		void FuckStreamer::initConn()
		{
			if ( concurrency_ > 0 )
			{
				struct sockaddr_in serverAddr;
				bzero(&serverAddr, sizeof(serverAddr));
				serverAddr.sin_family = AF_INET;
				inet_pton(AF_INET,streamerIP_, &serverAddr.sin_addr);
				serverAddr.sin_port = htons(static_cast<short>(streamerPort_));
				char *mediaFile = (char *)serverConfig_.server["media-source"].c_str();

				
				struct timeval prev, now;
				gettimeofday(&prev, NULL);
				for ( int index = 0; index < concurrency_; ++index )
				{
					int connFd = socket(AF_INET, SOCK_STREAM, 0 );
					if ( connFd == -1 )
					{
						totalConcurrency_ -=1;
						continue;
					}

					if ( connect(connFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr) ) == 0 )
					{
						Epoll::IOSet(epollFd_, connFd, Epoll::WRITE);
					}
					else
					{
						totalConcurrency_ -=1;
						continue;
					}
					
					ConnInfo connInfo;
					connInfo.first = true;
					connInfo.sentBytes = 0;
					connInfo.handler = fopen(mediaFile, "r");
					ssize_t ret = fread(connInfo.stream, sizeof(connInfo.stream), sizeof(char), connInfo.handler);
					if ( ret <=0 )
					{
						totalConcurrency_ -=1;
						continue;
					}
					ConnInfoPool_.insert(std::make_pair(connFd, connInfo));
				}

				
				gettimeofday(&now, NULL);

				time_t totalMS = now.tv_sec *1000+now.tv_usec/1000 - prev.tv_sec *1000+prev.tv_usec/1000;
				int ret = sprintf(element_, "Total Connect Requests:%d\n"\
									 "Complete Connect Requests:%d\n"\
									 "Total Connect Time:%u MS\n"\
									 "Per Connect Time:%.2f MS\n",
							      		 concurrency_, totalConcurrency_,  (unsigned)totalMS, (double)totalMS / totalConcurrency_);
				cout<<element_<<endl;
				if ( ret <= 0 )
				{
					std::cerr<<"sprintf error:"<<strerror(errno)<<endl;
					exit(1);
				}

				element_ += ret;
			}
			
		}
	};
};
