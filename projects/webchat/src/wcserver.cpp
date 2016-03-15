/*
*@filename:xtrartmp.cpp
*@author:chenzhengqiang
*@start date:2015/11/26 11:26:16
*@modified date:
*@desc: 
*/

#include "common.h"
#include "errors.h"
#include "netutil.h"
#include "serverutil.h"
#include "nana.h"
#include "sha1.h"
#include "base64.h"
#include "intlib.h"
#include "rosehttp.h"
#include "wcserver.h"
#include<iostream>
#include<stdint.h>



namespace czq
{
	namespace service
	{	

		//define the global macro for libev's event clean
		#define DO_LIBEV_CB_CLEAN(M,W) \
	    	ev_io_stop(M, W);\
	    	delete W;\
	    	W= NULL;\
	    	return;
		
		Nana *nana = 0;
		WcServer::WcServer( const ServerUtil::ServerConfig & serverConfig )
		 :listenFd_(-1),serverConfig_(serverConfig)
		{
			;
		}

		void WcServer::printHelp()
		{
			std::cout<<serverConfig_.usage<<std::endl;
			exit(EXIT_SUCCESS);
		}

		void WcServer::printVersion()
		{
			std::cout<<serverConfig_.meta["version"]<<std::endl<<std::endl;
			exit(EXIT_SUCCESS);
		}


		void WcServer::registerServer( int listenFd )
		{
			listenFd_ = listenFd;
			
		}


		void WcServer::serveForever()
		{
			#define ToServeForever __func__
			
			if ( listenFd_ >= 0 )
			{
				if ( serverConfig_.server["daemon"] == "yes" )
	    			{
	        			daemon(0,0);
	    			}

	    			nana = Nana::born(serverConfig_.server["log-file"], atoi(serverConfig_.server["log-level"].c_str()),
	    					   		atoi(serverConfig_.server["flush-time"].c_str()));
	    					   
	    			nana->say(Nana::HAPPY, ToServeForever,  "LISTEN SOCKET FD:%d", listenFd_);
	    
	    			//you have to ignore the PIPE's signal when client close the socket
	    			struct sigaction sa;
	    			sa.sa_handler = SIG_IGN;//just ignore the sigpipe
	    			sa.sa_flags = 0;
	    			if ( sigemptyset( &sa.sa_mask ) == -1 ||sigaction( SIGPIPE, &sa, 0 ) == -1 )
	    			{ 
	        			nana->say(Nana::COMPLAIN, ToServeForever, "FAILED TO IGNORE SIGPIPE SIGNAL");
	    			}
				else
				{
					struct ev_loop * mainEventLoopEntry = EV_DEFAULT;
	    				struct ev_io * listenWatcher = new ev_io;
					if ( mainEventLoopEntry != 0 && listenWatcher != 0 )
					{
						listenWatcher->data = static_cast<void *>(this);
						ev_io_init( listenWatcher, service::acceptCallback, listenFd_, EV_READ );
	             				ev_io_start( mainEventLoopEntry, listenWatcher);
	            				ev_run( mainEventLoopEntry, 0 );			
						free(mainEventLoopEntry);
						mainEventLoopEntry = 0;
						delete listenWatcher;
						listenWatcher = 0;	
					}
					else
					{
						nana->say(Nana::COMPLAIN, ToServeForever, "FAILED TO ALLOCATE MEMORY");	
					}
	    			}
	    			nana->die();
			}
			else
			{
				nana->say(Nana::COMPLAIN, ToServeForever, "INVALID LISTEN SOCKET FD");
			}
		}


		char * WcServer::fetchSecKey(const char * buf)
		{
		 	char *key;
		  	char *keyBegin;
		  	char *flag="Sec-WebSocket-Key: ";
		  	int i=0, bufLen=0;

		  	key=(char *)malloc(WEB_SOCKET_KEY_LEN_MAX);
		  	memset(key,0, WEB_SOCKET_KEY_LEN_MAX);
		  	if(!buf)
		    	{
		      		return NULL;
		    	}
		 
		  	keyBegin=(char *)strstr(buf,flag);
		  	if(!keyBegin)
		    	{
		      		return NULL;
		    	}

			keyBegin+=strlen(flag);
		  	bufLen=strlen(buf);
		  	for(i=0;i<bufLen;i++)
		    	{
		      		if(keyBegin[i]==0x0A||keyBegin[i]==0x0D)
				{
			  		break;
				}
		      		key[i]=keyBegin[i];
		    	}
		  	return key;
		}


		char * WcServer::computeAcceptKey(const char * buf)
		{
			 char * clientKey;
			 char * serverKey; 
			 char * sha1DataTemp;
			 char * sha1Data;
			 short temp;
			 int i,n;
			 const char * GUID="258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

			if ( !buf )
    			{
      				return NULL;
    			}

			clientKey=(char *)malloc(LINE_MAX);
  			memset(clientKey,0,LINE_MAX);
  			clientKey=fetchSecKey(buf);
 
  			if (!clientKey)
    			{
      				return NULL;
    			}

 			strcat(clientKey,GUID);
  			sha1DataTemp=sha1_hash(clientKey);
  			n=strlen(sha1DataTemp);
  			sha1Data=(char *)malloc(n/2+1);
  			memset(sha1Data,0,n/2+1);
  			for(i=0;i<n;i+=2)
    			{      
      				sha1Data[i/2]=htoi(sha1DataTemp,i,2);    
    			}	 
			serverKey = base64_encode(sha1Data, strlen(sha1Data)); 
  			return serverKey;
		}


		
		void WcServer::shakeHand(int connfd,const char *serverKey)
		{
		 	char responseHeader [RESPONSE_HEADER_LEN_MAX];
		  	if( !connfd )
		  	{
			 	return;
		  	}
			
		  	if(!serverKey)
			{
			  	return;
			}

			memset(responseHeader,'\0',RESPONSE_HEADER_LEN_MAX);
		  	sprintf(responseHeader, "HTTP/1.1 101 Switching Protocols\r\n");
		  	sprintf(responseHeader, "%sUpgrade: websocket\r\n", responseHeader);
		  	sprintf(responseHeader, "%sConnection: Upgrade\r\n", responseHeader);
		  	sprintf(responseHeader, "%sSec-WebSocket-Accept: %s\r\n\r\n", responseHeader, serverKey);
		  	printf("Response Header:%s\n",responseHeader);
		  	write(connfd,responseHeader,strlen(responseHeader));
		}
		
		
		
		char * WcServer:: analyData(const char * buf,const int bufLen)
		{
		  	char * data;
		  	char fin, maskFlag,masks[4];
		  	char * payloadData;
		  	char temp[8];
		  	unsigned long n, payloadLen=0;
		  	unsigned short usLen=0;
		  	int i=0; 
		
		 	if (bufLen < 2) 
		   	{
			 	return NULL;
		   	}
		
		  	fin = (buf[0] & 0x80) == 0x80; // 1bit，1表示最后一帧  
		  	if (!fin)
		   	{
			 	return NULL;// 超过一帧暂不处理 
		   	}
		
		   	maskFlag = (buf[1] & 0x80) == 0x80; // 是否包含掩码	
		   	if (!maskFlag)
		   	{
			 	return NULL;// 不包含掩码的暂不处理
		   	}
			
		   	payloadLen = buf[1] & 0x7F; // 数据长度 
		   	if (payloadLen == 126)
		   	{	  
			 	memcpy(masks,buf+4, 4);	  
			 	payloadLen =(buf[2]&0xFF) << 8 | (buf[3]&0xFF);  
			 	payloadData=(char *)malloc(payloadLen);
			 	memset(payloadData,0,payloadLen);
			 	memcpy(payloadData,buf+8,payloadLen);
			}
			else if (payloadLen == 127)
			{
			 	memcpy(masks,buf+10,4);  
			 	for ( i = 0; i < 8; i++)
			 	{
				 	temp[i] = buf[9 - i];
			 	} 

				memcpy(&n,temp,8);  
			 	payloadData=(char *)malloc(n); 
			 	memset(payloadData,0,n); 
			 	memcpy(payloadData,buf+14,n);//toggle error(core dumped) if data is too long.
			 	payloadLen=n;	  
			 }
			 else
			 {	 
			  	memcpy(masks,buf+2,4);	
			  	payloadData=(char *)malloc(payloadLen);
			  	memset(payloadData,0,payloadLen);
			  	memcpy(payloadData,buf+6,payloadLen); 
			 }
		
			 for (i = 0; i < payloadLen; i++)
			 {
			   	payloadData[i] = (char)(payloadData[i] ^ masks[i % 4]);
			 }
		 
			 printf("data(%d):%s\n",payloadLen, payloadData);
			 return payloadData;
		}
		
		char *WcServer::packData(const char * message,unsigned long * len)
		{
			char * data=NULL;
			unsigned long n;
			n=strlen(message);
			if (n < 126)
			{
				  data=(char *)malloc(n+2);
				  memset(data,0,n+2);	 
				  data[0] = 0x81;
				  data[1] = n;
				  memcpy(data+2,message,n);
				  *len=n+2;
			}
			else if (n < 0xFFFF)
			{
				  data=(char *)malloc(n+4);
				  memset(data,0,n+4);
				  data[0] = 0x81;
				  data[1] = 126;
				  data[2] = (n>>8 & 0xFF);
				  data[3] = (n & 0xFF);
				  memcpy(data+4,message,n);    
				  *len=n+4;
			}
			else
			{
			 	// 暂不处理超长内容  
				 *len=0;
			}
			return data;
		 }
		
		void WcServer::response(int connfd,const char * message)
		{
		  	char * data;
		  	unsigned long n=0;
		  	int i;
		  	if(!connfd)
			{
			  	return;
			}
		
		  	if(!data)
			{
			  	return;
			}

			data=packData(message,&n); 
		  	if(!data||n<=0)
			{
			  	printf("data is empty!\n");
			  	return;
			} 
			printf("response:%s\n", data);
		  	write(connfd,data,n);
		}


		void  acceptCallback( struct ev_loop * eventLoopEntry, struct ev_io * listenWatcher, int revents )
		{
			#define ToAcceptCallback __func__
		
			if ( EV_ERROR & revents )
			{
				nana->say(Nana::COMPLAIN, ToAcceptCallback, "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
				DO_LIBEV_CB_CLEAN(eventLoopEntry, listenWatcher);
			}
		
			struct sockaddr_in clientAddr;
			socklen_t len = sizeof( struct sockaddr_in );
			int connFd = accept( listenWatcher->fd, (struct sockaddr *)&clientAddr, &len );
			if ( connFd < 0 )
			{
				nana->say(Nana::COMPLAIN, ToAcceptCallback, "ACCEPT ERROR:%s", strerror(errno));   
				return;
			}
					
			struct ev_io * shakeHandWatcher = new struct ev_io;
			if ( shakeHandWatcher != 0 )
			{
				shakeHandWatcher->active = 0;
				shakeHandWatcher->data = listenWatcher->data;   
				if ( nana->is(Nana::HAPPY) )
				{
					std::string peerInfo = NetUtil::getPeerInfo(connFd);
					nana->say(Nana::HAPPY, ToAcceptCallback, "CLIENT %s CONNECTED SOCK FD IS:%d",
																	 peerInfo.c_str(), connFd);
				}
				
				//register the socket io callback for reading client's request	  
				ev_io_init(  shakeHandWatcher, shakeHandCallback, connFd, EV_READ );
				ev_io_start( eventLoopEntry, shakeHandWatcher );
			}
			else
			{
				nana->say(Nana::COMPLAIN, ToAcceptCallback, "ALLOCATE MEMORY FAILED:%s", strerror( errno ));
				close(connFd);
			}
		}


		void  shakeHandCallback( struct ev_loop * eventLoopEntry, struct ev_io * shakeHandWatcher, int revents )
		{
			#define ToRecvRequestCallback __func__
			if ( EV_ERROR & revents )
			{
				nana->say(Nana::COMPLAIN, ToRecvRequestCallback, "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
				DO_LIBEV_CB_CLEAN(eventLoopEntry, shakeHandWatcher);
			}
			
			WcServer *wcServer = static_cast<WcServer *>(shakeHandWatcher->data);
			char *data;
			char *secWebSocketKey;
			char request[2048]={0};
			ssize_t readBytes = read(shakeHandWatcher->fd, request, sizeof(request));	
			if ( readBytes > 0 )
			{
				request[readBytes]='\0';
				nana->say(Nana::HAPPY, ToRecvRequestCallback, "REQUEST IS:%s", request);
			    	secWebSocketKey=wcServer->computeAcceptKey(request);	
			    	wcServer->shakeHand(shakeHandWatcher->fd, secWebSocketKey);

				struct ev_io * recvMessageWatcher = new struct ev_io;
				if ( recvMessageWatcher != 0 )
				{
					recvMessageWatcher->data = shakeHandWatcher->data;
					ev_io_init(  shakeHandWatcher, recvMessageCallback, shakeHandWatcher->fd, EV_READ );
					ev_io_start( eventLoopEntry, recvMessageWatcher );
				}
				else
				{
					close(shakeHandWatcher->fd);
				}
				
			}
			else
			{
				close(shakeHandWatcher->fd);
			}
			
			DO_LIBEV_CB_CLEAN(eventLoopEntry, shakeHandWatcher);
		}


		void  recvMessageCallback( struct ev_loop * eventLoopEntry, struct ev_io * recvMessageWatcher, int revents )
		{
			#define ToRecvMessageCallback __func__
			if ( EV_ERROR & revents )
			{
				nana->say(Nana::COMPLAIN, ToRecvMessageCallback, "LIBEV ERROR FOR EV_ERROR:%d", EV_ERROR);
				DO_LIBEV_CB_CLEAN(eventLoopEntry, recvMessageWatcher);
			}

			WcServer *wcServer = static_cast<WcServer *>(recvMessageWatcher->data);
			char *data;
			char *secWebSocketKey;
			char request[2048]={0};
			ssize_t readBytes = read(recvMessageWatcher->fd, request, sizeof(request));	
			if ( readBytes > 0 )
			{
				data=wcServer->analyData(request, readBytes);
				wcServer->response(recvMessageWatcher->fd, data);
			}
			else
			{
				close(recvMessageWatcher->fd);
				DO_LIBEV_CB_CLEAN(eventLoopEntry, recvMessageWatcher);
			}
		}
	};
}		;
