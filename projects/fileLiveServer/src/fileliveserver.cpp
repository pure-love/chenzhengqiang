/*
*@filename:fileliveserver.cpp
*@author:chenzhengqiang
*@start date:2016/01/30 18:37:10
*@modified date:
*@desc: 
*/



#include "fileliveserver.h"
#include "common.h"
#include "errors.h"
#include "netutil.h"
#include "serverutil.h"
#include "nana.h"
#include "rosehttp.h"
#include<string>
#include<iostream>
#include<stdint.h>
#include<dirent.h>
#include<list>
#include<pthread.h>
#include<sys/time.h>

namespace czq
{
	
	namespace Service
	{	
		//define the global macro for libev's event clean
		#define DO_EVENT_CB_CLEAN(M,W) \
    		ev_io_stop(M, W);\
    		delete W;\
    		W= NULL;\
    		return;

		M3u8Pool gM3u8Pool;
		//the global config
		ServerUtil::ServerConfig SERVER_CONFIG;
		//the global logging tool
		Nana *nana=0;
		static std::string m3u8Header="#EXTM3U\r\n#EXT-X-VERSION:3\r\n";
		FileLiveServer::FileLiveServer( const ServerUtil::ServerConfig & serverConfig )
		:serverConfig_(serverConfig),listenFd_(-1),mainEventLoop_(0),listenWatcher_(0),
		acceptWatcher_(0),writeWatcher_(0),stop_(false)
		{
			SERVER_CONFIG = serverConfig;
			sleepTime_ = atoi(SERVER_CONFIG.server["sleep"].c_str());
		}


		FileLiveServer::~FileLiveServer()
		{
			;
		}

		
		void FileLiveServer::printHelp()
		{
			std::cout<<serverConfig_.usage<<std::endl;
			exit(EXIT_SUCCESS);
		}


		void FileLiveServer::printVersion()
		{
			std::cout<<serverConfig_.meta["version"]<<std::endl<<std::endl;
			exit(EXIT_SUCCESS);
		}


		void FileLiveServer::registerServer( int listenFd )
		{
			listenFd_ = listenFd;
		}


		void FileLiveServer::serveForever()
		{
			if ( listenFd_ >= 0 )
			{
				if ( serverConfig_.server["daemon"] == "yes" )
    				{
        				daemon(0,0);
    				}

    				nana = Nana::born(serverConfig_.server["log-file"], atoi(serverConfig_.server["log-level"].c_str()),
    					    atoi(serverConfig_.server["flush-time"].c_str()));
    					   
    				nana->say(Nana::HAPPY, __func__,  "LISTEN SOCKET FD:%d", listenFd_);
    
    				//you have to ignore the PIPE's signal when client close the socket
    				struct sigaction sa;
    				sa.sa_handler = SIG_IGN;//just ignore the sigpipe
    				sa.sa_flags = 0;
    				if ( sigemptyset( &sa.sa_mask ) == -1 ||sigaction( SIGPIPE, &sa, 0 ) == -1 )
    				{ 
        				nana->say(Nana::COMPLAIN, __func__, "FAILED TO IGNORE SIGPIPE SIGNAL");
    				}
				else
				{
					mainEventLoop_ = EV_DEFAULT;
					if ( serverConfig_.server["lazy"] == "yes" )
					{
						//just update the m3u8 file directly from specified directory
						while ( !stop_ )
						{
							directlyUpdateM3u8();
							nana->say(Nana::HAPPY, __func__, "ALL MEDIA FILE UPDATED, JUST WAIT %d SECONDS", sleepTime_);
							//the best way
							//you can startup a daemon thread or process to watch the change of media file directory
							//and when the manager add some media files manualy,just send a signal to wake up
							//the file live server
							//sleep several seconds is one simple way
							//everything is upond to you!
							sleep(sleepTime_);
						}	
					}
					else
					{
    						//initialization on main event-loop
    						pthread_t tid;
						pthread_attr_t threadAttr;
						pthread_attr_init(&threadAttr);
       					pthread_attr_setdetachstate( &threadAttr, PTHREAD_CREATE_DETACHED );
						ssize_t ret = pthread_create(&tid, &threadAttr, upgradeThreadEntry, (void *)this);
						if ( ret != 0 )
						{
							nana->say(Nana::COMPLAIN, __func__, "PTHREAD CREATE ERROR:%s", strerror(errno));
							abort();
						}
						
    						listenWatcher_ = new ev_io;
    						ev_io_init( listenWatcher_, acceptCallback, listenFd_, EV_READ );
             					ev_io_start( mainEventLoop_, listenWatcher_ );
						ev_run( mainEventLoop_, 0 );
					}
					
    				}
			}
			else
			{
				;
			}
		}

		
		void *upgradeThreadEntry( void * arg )
		{
			#define ToUpgradeThreadEntry __func__
			
			 std::string dir = SERVER_CONFIG.server["media-file-dir"];
			 if ( ! dir.empty() )
			 {
			 	int avgDuration;
			 	while ( true )
			 	{
					avgDuration = obtainM3u8Source(dir.c_str());
					/*
					if ( nana->is(Nana::HAPPY) )
					{
						M3u8Pool::const_iterator mp_citer = gM3u8Pool.begin();
						while ( mp_citer != gM3u8Pool.end() )
						{
							nana->say(Nana::HAPPY, ToUpgradeThreadEntry, "M3U8 SOURCE:%s", mp_citer->first.c_str());
							nana->say(Nana::HAPPY, ToUpgradeThreadEntry, "M3U8 HEADER:%s", mp_citer->second.header.c_str());
							nana->say(Nana::HAPPY, ToUpgradeThreadEntry, "M3U8 POS:%u", mp_citer->second.pos);
							std::vector<std::string>::const_iterator tulCiter = mp_citer->second.tsUrlPool.begin();
							while ( tulCiter  != mp_citer->second.tsUrlPool.end() )
							{
								nana->say(Nana::HAPPY, ToUpgradeThreadEntry, "M3U8 TS URL:%s", tulCiter->c_str());
								++tulCiter ;
							}
							++mp_citer;
						}
					}*/
					sleep(avgDuration);
			 	}	
			 }
			 return arg;
		}

		int obtainM3u8Source( const char * dir )
		{
			int avgDuration = 0;
			if ( dir != 0 )
			{
				DIR * dirp = opendir(dir);
				struct dirent * dirEntry;
				if ( dirp != 0 )
				{
					FILE *fp;
					std::string mediaType = SERVER_CONFIG.server["media-type"];
					if ( mediaType.empty() )
					mediaType = "m3u8";
					int count = 0;
					char line[1024];
					std::string absUrl;
					std::string tsItem;
					
					while ( (dirEntry = readdir(dirp)) != 0 )
					{
						if (strstr(dirEntry->d_name, mediaType.c_str() ) != 0 )
						{
							M3u8Pool::iterator iter = gM3u8Pool.find(std::string(dirEntry->d_name));
							if ( iter == gM3u8Pool.end() )
							{
								std::string fullPath = std::string(dir)+"/"+dirEntry->d_name;
								M3u8 m3u8;
								m3u8.pos = 0;
								m3u8.header = m3u8Header;
								fp = fopen(fullPath.c_str(), "r");
								while ( fgets(line, sizeof(line), fp) != 0 )
								{
									absUrl="http://";
									if ( strstr(line, "#EXT-X-TARGETDURATION:") != 0 )
									{
										m3u8.header+=line;
									}

									if (strncmp(line, "#EXTINF:", 8) == 0)
									{
										tsItem = line;
										if ( fgets(line, sizeof(line), fp) != 0 )
										{
											if (  strncmp(line, "#EXT", 4) != 0 )
											{
												absUrl+=SERVER_CONFIG.server["media-source-domain"]+"/"+line;
												tsItem += absUrl;
												m3u8.tsUrlPool.push_back(tsItem);
											}
										}
										else
										break;	
									}
								}
								m3u8.tsUrlPool.push_back("#EXT-X-ENDLIST");
								fclose(fp);
								gM3u8Pool.insert(std::make_pair(std::string(dirEntry->d_name), m3u8));
								++count;
								if ( count == 5 )
								{
									break;
								}
							}
							else
							{
								iter->second.pos = (iter->second.pos+1) % iter->second.tsUrlPool.size();
							}
							
						}
					}
					closedir(dirp);
				}
				
			}
			
			return (avgDuration==0) ? 10:avgDuration;
		}
		
		struct TData
		{
			FILE * handler;
			char fileName[50];
		};
		void FileLiveServer::directlyUpdateM3u8()
		{
			std::vector<std::string> mediaFilePool;
			getMediaFiles(serverConfig_.server["media-file-dir"].c_str(), mediaFilePool);
			if ( ! mediaFilePool.empty() )
			{
				nana->say(Nana::HAPPY, __func__, "MEDIA FILES TOTAL:%u", mediaFilePool.size());
				std::vector<std::string>::const_iterator citer = mediaFilePool.begin();
				int mediaFileFd;
				bool haveFile = false;
				while ( citer != mediaFilePool.end() )
				{
					std::string newMediaFile = serverConfig_.server["ngx-root"]+"/"+*citer;
					nana->say(Nana::HAPPY, __func__, "NEW MEDIA FILE:%s", newMediaFile.c_str());
					remove(newMediaFile.c_str());
					if ( ! ServerUtil::fileExists( newMediaFile.c_str() ) )
					{
						mediaFileFd = open(newMediaFile.c_str(), O_CREAT|O_WRONLY);
						if ( mediaFileFd >= 0 )
						{
							ServerUtil::setNonBlocking(mediaFileFd);
							nana->say(Nana::HAPPY, __func__, "OPEN %s SUCCESSED", citer->c_str());
							std::string oldMediaFile = serverConfig_.server["media-file-dir"]+"/"+*citer;
							nana->say(Nana::HAPPY, __func__, "OLD MEDIA FILE:%s", oldMediaFile.c_str());
							TData *tdata = new TData;
							FILE *fp = fopen(oldMediaFile.c_str(), "r");
							if ( fp != 0 )
							{
								tdata->handler = fp;
								strcpy(tdata->fileName,newMediaFile.c_str());
								writeWatcher_= new ev_io;
								writeWatcher_->data = (void *)tdata;
								ev_io_init(writeWatcher_, writeCallback, mediaFileFd, EV_WRITE);
         							ev_io_start( mainEventLoop_, writeWatcher_);
								haveFile = true;
							}
								
						}
						else
						{
							nana->say(Nana::HAPPY, __func__, "OPEN %s FAILED:%s", newMediaFile.c_str(), strerror(errno));
						}
					}
					++citer;
				}

				if ( haveFile )
				ev_run( mainEventLoop_, 0 );
				//deleteMediaFiles(mediaFilePool);
				mediaFilePool.clear();
			}
		}

		void FileLiveServer::getMediaFiles( const char * dir, std::vector<std::string> & mediaFilePool )
		{
			if ( dir != 0 )
			{
				DIR * dirp = opendir(dir);
				struct dirent * dirEntry;
				if ( dirp != 0 )
				{
					std::string mediaType = serverConfig_.server["media-type"];
					if ( mediaType.empty() )
					mediaType = "m3u8";
					while ( (dirEntry = readdir(dirp)) != 0 )
					{
						if (strstr(dirEntry->d_name, mediaType.c_str() ) != 0 )
						{
							mediaFilePool.push_back(std::string(dirEntry->d_name));
						}
					}
					closedir(dirp);
				}
				
			}	
		}


		
		void FileLiveServer::deleteMediaFiles( std::vector<std::string> & mediaFilePool )
		{
			std::vector<std::string>::const_iterator citer = mediaFilePool.begin();
			while ( citer != mediaFilePool.end() )
			{
				std::string oldMediaFile = serverConfig_.server["media-file-dir"]+"/"+*citer;
				remove(oldMediaFile.c_str());
				++citer;
			}
		}
		
		void writeCallback( struct ev_loop * mainEventLoop, struct ev_io * writeWatcher, int revents )
		{
			#define ToWriteCallback __func__
			if ( EV_ERROR & revents )
	    		{
	        		nana->say(Nana::COMPLAIN, ToWriteCallback, "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
	        		return;
	    		}

			static int targetDuration = 0;
			static suseconds_t tsDuration = 0;
			static suseconds_t waitTime = 0;
			static time_t prevTime = 0;
			static int mediaSequence = 0;
			static std::string tsItem;
			tsItem.reserve(1024);
			struct  timeval now;
			gettimeofday(&now, NULL);
			suseconds_t elapsed = now.tv_sec * 1000000;
			elapsed += now.tv_usec;
			if ( ( elapsed - prevTime ) < waitTime )
			return;
			else
			{
				nana->say(Nana::HAPPY, ToWriteCallback, "%d MICRO SECONDS HAS PASSED AWAY,UPDATE ANOTHER M3U8", waitTime);
				waitTime = 0;
				prevTime = elapsed;
			}
			
			TData *tdata = static_cast<TData *>(writeWatcher->data);
			if ( tdata != 0 && tdata->handler != 0 )
			{
				char line[1024];
				char *pNum;
				static std::string tmpM3u8File = std::string(tdata->fileName)+".tmp";
				if ( fgets(line, sizeof(line), tdata->handler ) != 0 )
				{
					std::string absUrl="http://";
					if (strncmp(line, "#EXTINF:", 8) == 0)
					{
						tsItem = line;
						pNum = strstr(line, ":");
						tsDuration = ((suseconds_t ) (atof(pNum+1)*10))*100000;
						waitTime += tsDuration;
						nana->say(Nana::HAPPY, ToWriteCallback, "TS DURATION IS:%u MICROSECONDS", tsDuration);
						int count = 0;
						while ( fgets(line, sizeof(line), tdata->handler) != 0 )
						{
							if (strncmp(line, "#EXTINF:", 8) == 0)
							{
								pNum = strstr(line, ":");
								tsDuration = ((suseconds_t ) (atof(pNum+1)*10))*100000;
								nana->say(Nana::HAPPY, ToWriteCallback, "TS DURATION IS:%u MICROSECONDS", tsDuration);
								waitTime += tsDuration;
								tsItem += line;
							}
							else if (  strncmp(line, "#EXT-X-ENDLIST", 14) != 0 )
							{
								absUrl+=SERVER_CONFIG.server["media-source-domain"]+"/"+line;
								tsItem += absUrl;
								absUrl="http://";
							}
							++count;
							if ( count == 5 )
							break;	
						}

						waitTime -= 677 * 10000;
						nana->say(Nana::HAPPY, ToWriteCallback, "TOTAL WAIT MICROSENDS IS:%u", waitTime);
						nana->say(Nana::HAPPY, ToWriteCallback, "READ THE M3U8 FILE'S TS ITEM:%s", tsItem.c_str());
						std::ostringstream	OMediaSequence;
						OMediaSequence<<"#EXT-X-MEDIA-SEQUENCE:"<<mediaSequence<<"\r\n";
						mediaSequence += 3;
						FILE* fp = fopen(tmpM3u8File.c_str(), "w");
						fputs(m3u8Header.c_str(), fp);
						fputs(OMediaSequence.str().c_str(), fp);
						fputs(tsItem.c_str(), fp);
						tsItem = "";
						rename(tmpM3u8File.c_str(), tdata->fileName);
						fclose(fp);
						if ( count != 5 )
						{
							fclose(tdata->handler);
							close(writeWatcher->fd);
							delete static_cast<TData *>(writeWatcher->data);
							DO_EVENT_CB_CLEAN(mainEventLoop, writeWatcher);
						}
					}
					else if( strncmp(line, "#EXT-X-ENDLIST", 14) == 0 )
					{
						nana->say(Nana::HAPPY, ToWriteCallback, "ALREADY REACH THE M3U8 FILE'S END");
					}
					else
					{
						nana->say(Nana::HAPPY, ToWriteCallback, "READ THE M3U8 FILE'S HEAD:%s", line);
						if ( strstr(line, "#EXT-X-TARGETDURATION:") != 0 )
						{
							m3u8Header += line;
							pNum = strstr(line, ":");
							targetDuration = atoi(pNum+1);
							nana->say(Nana::HAPPY, ToWriteCallback, "THE M3U8'S TARGET DURATION IS:%d", targetDuration);
						}
					}
				}
				else
				{
					fclose(tdata->handler);
					close(writeWatcher->fd);
					delete static_cast<TData *>(writeWatcher->data);
					DO_EVENT_CB_CLEAN(mainEventLoop, writeWatcher);
				}
			}
			else
			{
				close(writeWatcher->fd);
				DO_EVENT_CB_CLEAN(mainEventLoop, writeWatcher);
			}
		}

		
		void acceptCallback( struct ev_loop * mainEventLoop, struct ev_io * listenWatcher, int revents )
		{
			#define ToAcceptCallback __func__

	    		if ( EV_ERROR & revents )
	    		{
	        		nana->say(Nana::COMPLAIN, ToAcceptCallback, "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
	        		return;
	    		}

	    		struct sockaddr_in clientAddr;
	    		socklen_t len = sizeof( struct sockaddr_in );
	    		int connFd = accept( listenWatcher->fd, (struct sockaddr *)&clientAddr, &len );
	    		if ( connFd < 0 )
	    		{
	        		nana->say(Nana::COMPLAIN, ToAcceptCallback, "ACCEPT ERROR:%s", strerror(errno));   
	        		return;
	    		}
	    		
	    		struct ev_io * requestWatcher = new struct ev_io;
	    		if ( requestWatcher == NULL )
	    		{
	        		nana->say(Nana::COMPLAIN, ToAcceptCallback, "ALLOCATE MEMORY FAILED:%s", strerror( errno ));
	        		close(connFd);
	        		return;
	    		}
	    
	    		requestWatcher->active = 0;
	    		requestWatcher->data = 0;   
	    		if ( nana->is(Nana::PEACE))
	    		{
	        		std::string peerInfo = NetUtil::getPeerInfo(connFd);
	        		nana->say(Nana::PEACE, ToAcceptCallback, "CLIENT %s CONNECTED SOCK FD IS:%d",
	                                                             peerInfo.c_str(), connFd);
	    		}

			//register the socket io callback for reading client's request    
    			ev_io_init(  requestWatcher, requestCallback, connFd, EV_READ );
    			ev_io_start( mainEventLoop, requestWatcher );	
		}


		void requestCallback( struct ev_loop * mainEventLoop, struct ev_io * requestWatcher, int revents )
		{
			#define ToRequestCallback __func__

	    		if ( EV_ERROR & revents )
	    		{
	        		nana->say(Nana::COMPLAIN, ToRequestCallback, "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
				RoseHttp::replyWithRoseHttpStatus( 500, requestWatcher->fd);
        			close(requestWatcher->fd );
				DO_EVENT_CB_CLEAN(mainEventLoop, requestWatcher);
	    		}

			ssize_t ret;
    			//used to store those key-values parsed from http request
    			char request[1024];
    			//read the http header and then parse it
    			ret = RoseHttp::readRoseHttpHeader( requestWatcher->fd, request, sizeof(request) );
    			if(  ret > 0  )
    			{     
	          		request[ret]='\0';
	    			nana->say( Nana::HAPPY, ToRequestCallback, "HTTP REQUEST HEADER:%s", request );
				RoseHttp::SimpleRoseHttpHeader simpleRoseHttpHeader;
				ret = RoseHttp::parseSimpleRoseHttpHeader( request, strlen( request ), simpleRoseHttpHeader);
	    			if( ret != STREAM_FORMAT_ERROR )
	    			{
					std::string::size_type pos = simpleRoseHttpHeader.serverPath.find_last_of('/');
					std::string requestedSource = simpleRoseHttpHeader.serverPath.substr(pos+1);
					nana->say(Nana::HAPPY, ToRequestCallback, "REQUESTED CHANNEL:%s", simpleRoseHttpHeader.urlArgs["channel"].c_str());
					nana->say(Nana::HAPPY, ToRequestCallback, "REQUESTED SOURCE:%s", requestedSource.c_str());
		
					M3u8Pool::iterator iter = gM3u8Pool.find(requestedSource);
					
					if ( iter != gM3u8Pool.end() )
					{
						size_t tsUrlPos = iter->second.pos;
						std::ostringstream Oreply;
						Oreply<<"HTTP/1.1 206 Partial Content\r\nContent-Type:application/vnd.apple.mpegurl\r\n";
						std::ostringstream Ocontent;
						
						Ocontent<<iter->second.header<<"#EXT-X-MEDIA-SEQUENCE:"<<tsUrlPos<<"\r\n";
		
						size_t tsUrlPoolSize = iter->second.tsUrlPool.size();
						while ( tsUrlPos != tsUrlPoolSize )
						{
							Ocontent<<iter->second.tsUrlPool[tsUrlPos];
							++tsUrlPos;
						}
						
						Oreply<<"Content-Length:"<<Ocontent.str().length()<<"\r\n\r\n"<<Ocontent.str();
						NetUtil::writeSpecifySize2( requestWatcher->fd, Oreply.str().c_str(), Oreply.str().length());
					}
					else
					{
						nana->say(Nana::HAPPY, ToRequestCallback, "REQUESTED SOURCE:%s NOT EXISTS", requestedSource.c_str());
						RoseHttp::replyWithRoseHttpStatus(404, requestWatcher->fd);
					}
	    			}
				else
				{
					nana->say( Nana::COMPLAIN, ToRequestCallback, "HTTP REQUEST LINE FORMAT ERROR");
	        			RoseHttp::replyWithRoseHttpStatus( 400, requestWatcher->fd );
				}
    			}
			else
			{
				if(  ret == 0  )
          			{
              			if( nana->is(Nana::PEACE) )
              			{
                    			std::string peerInfo = NetUtil::getPeerInfo( requestWatcher->fd );
                    			nana->say( Nana::PEACE, ToRequestCallback," READ 0 BYTE FROM %s CLIENT DISCONNECTED ALREADY",
                                                                                           peerInfo.c_str() );
              			}
						
          			}
					
          			else if(  ret == LENGTH_OVERFLOW  )
          			{
              			if( nana->is(Nana::PEACE) )
              			{
                    			std::string peerInfo = NetUtil::getPeerInfo( requestWatcher->fd );
                    			nana->say( Nana::PEACE, ToRequestCallback, "CLIENT %s'S HTTP REQUEST LINE IS TOO LONG",
                                                                                          peerInfo.c_str() );
              			}
          			}
					
          			RoseHttp::replyWithRoseHttpStatus(400, requestWatcher->fd);
			}
    		
			close(requestWatcher->fd);
			DO_EVENT_CB_CLEAN(mainEventLoop, requestWatcher);
		}
	};
};

