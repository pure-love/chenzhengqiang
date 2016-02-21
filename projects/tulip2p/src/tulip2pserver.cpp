/*
*@filename:tulip2pserver.cpp
*@author:chenzhengqiang
*@start date:2016/02/21 11:56:04
*@modified date:
*@desc: 
*/

#include "errors.h"
#include "common.h"
#include "nana.h"
#include "netutil.h"
#include "tulip2p.h"
#include "tulip2pserver.h"
#include <sys/epoll.h>

namespace czq
{
	namespace service
	{
			//our angel nana
			Nana* nana = 0;
			
			static const int DEFAULT =0;
			static const int SELECT = 1;
			static const int EPOLL = 2;
			
			Tulip2pServer::Tulip2pServer( const ServerUtil::ServerConfig & serverConfig )
			:serverConfig_(serverConfig), serverFd_(-1)
			{
				;
			}
			
			
			Tulip2pServer::~Tulip2pServer()
			{
				;
			}
			
					
			void Tulip2pServer::printHelp()
			{
				std::cout<<serverConfig_.usage<<std::endl;
				exit(EXIT_SUCCESS);
			}
			
			
			void Tulip2pServer::printVersion()
			{
				std::cout<<serverConfig_.meta["version"]<<std::endl<<std::endl;
				exit(EXIT_SUCCESS);
			}
			
			
			void Tulip2pServer::registerServer( int listenFd )
			{
				serverFd_ = listenFd;
			}


			void Tulip2pServer::serveForever()
			{
				 #define ToServeForever __func__
	    			//if run_as_daemon is true,then make this speech transfer server run as daemon
	    			if ( serverConfig_.server["daemon"] == "yes" )
    				{
        				daemon(0,0);
    				}

				nana = Nana::born(serverConfig_.server["log-file"], atoi(serverConfig_.server["log-level"].c_str()),
	    					    		atoi(serverConfig_.server["flush-time"].c_str()));
	    					   
	    			nana->say(Nana::HAPPY, ToServeForever,  "SERVER SOCK FD:%d", serverFd_);
	    
	    			//you have to ignore the PIPE's signal when client close the socket
	    			struct sigaction sa;
	    			sa.sa_handler = SIG_IGN;//just ignore the sigpipe
	    			sa.sa_flags = 0;
	    			if ( sigemptyset( &sa.sa_mask ) == -1 ||sigaction( SIGPIPE, &sa, 0 ) == -1 )
	    			{ 
	        			nana->say(Nana::COMPLAIN, ToServeForever, "FAILED TO IGNORE SIGPIPE SIGNAL");
	    			}
					
	 			
	    			NetUtil::setReuseAddr( serverFd_ );

				int eventLoop = atoi(serverConfig_.server["event-loop"].c_str());
	   			if( eventLoop == DEFAULT || eventLoop == EPOLL )
	    			{
	        			nana->say(Nana::HAPPY, ToServeForever, "+++++EPOLL CONFIGURED,TRANSFER SERVER RUN EPOLL EVENT LOOP+++++");
	         			int epollFd, events;
	         			struct epoll_event epollEvent;
	         			struct epoll_event epollEvents[1024];
	         			epollFd = epoll_create(1024);
	         			epollEvent.events = EPOLLIN | EPOLLET;
	         			epollEvent.data.fd = serverFd_;
	         
	         			if( epoll_ctl( epollFd, EPOLL_CTL_ADD, serverFd_, &epollEvent ) < 0 ) 
	        			{
	             				nana->say(Nana::COMPLAIN, ToServeForever, "EPOLL_CTL_ADD ERROR FOR:FD=%d", serverFd_);
	             				goto BYEBYE;
	        			}

	        			while ( true ) 
	        			{
	             				events = epoll_wait( epollFd, epollEvents, sizeof( epollEvents ), -1 );
	             				if ( events == -1 )
	             				{
	                 				nana->say(Nana::COMPLAIN, ToServeForever, "EPOLL_WAIT FAILED:%s", strerror(errno) );
	                 				break;
	             				}
	             
	             				for ( int event = 0; event < events; ++event )
	             				{
	                 				if ( epollEvents[event].data.fd == serverFd_ && ( epollEvents[event].events & EPOLLIN ) ) 
	                 				{
	                      				nana->say(Nana::HAPPY, ToServeForever, "UDP SOCK FD:%d CAN READ !NOW GO TO HANDLE_UDP_MSG ROUTINE", serverFd_ );
	                      				;//do the callback here
	                      				nana->say(Nana::HAPPY, ToServeForever, "HANDLE_UDP_MSG ROUTINE DONE,RETURN BACK TO MAIN EVENT LOOP");
	                 				} 
	             				}
	        			}
	    			}
	    			else if( eventLoop == SELECT )
	    			{
	          			nana->say( Nana::HAPPY, ToServeForever,"SELECT CONFIGURED,SERVER STARTUP SELECT EVENT LOOP");
	          			fd_set readFds;
	          			FD_ZERO( &readFds );
	          			FD_SET( serverFd_, &readFds );
	          			while( true )
	          			{
	                			int ret = select( serverFd_+1, &readFds, NULL, NULL, NULL );
	                			if(  ret ==-1 )
	                			{
	                    			nana->say( Nana::COMPLAIN, ToServeForever , "SELECT FAILED:%s", strerror( errno ) );
	                    			break;
	                			}
	                
	                			if( FD_ISSET( serverFd_, &readFds ) )
	                			{
	                      			handleClientRequest();
	                			}
	          			}
	    			}
					
	    			BYEBYE:
	    			close(serverFd_);
	    			nana->die();
			}


			void Tulip2pServer::handleClientRequest()
			{
				#define ToHandleClientRequest __func__
				size_t totalBytes = 0;
				ssize_t receivedBytes = 0;
	     			ClientInfo clientInfo;
	     			socklen_t addrLen = sizeof( clientInfo.address );
				char request[1024]={0};
				
	     			if ( ( receivedBytes = recvfrom(serverFd_, request+totalBytes, sizeof(request)-totalBytes, MSG_DONTWAIT, 
	                			(struct sockaddr *)&(clientInfo.address), &addrLen )  ) <=0 )
	     			{
	         			nana->say(Nana::COMPLAIN, ToHandleClientRequest, " RECVFROM FAILED:%s",strerror(errno) );
	     			}
			
				else	
				{
					request[receivedBytes]='\0';
					nana->say(Nana::HAPPY, ToHandleClientRequest, "REQUEST:%s", request);

					char IP[INET_ADDRSTRLEN];
         				inet_ntop( AF_INET, &( clientInfo.address.sin_addr), IP, INET_ADDRSTRLEN );
         				std::ostringstream OSS_ID;
         				OSS_ID<<IP<<":"<<clientInfo.address.sin_port;
         				std::string CID=OSS_ID.str();
						
					//PING TULIP2P/2016.2.21
					//QUERY /a.pdf TULIP2P/2016.2.21

					Tulip2p::Tulip2pHeader tulip2pHeader;
					ssize_t ret = Tulip2p::parseTulip2pHeader( request, strlen( request ), tulip2pHeader);
					if ( ret == OK )
					{
						std::string::size_type pos = tulip2pHeader.serverPath.find_last_of('/');
						std::string resource =  tulip2pHeader.serverPath.substr(pos+1);
						
						if ( tulip2pHeader.method == "PING" )
						{
							onPingRequest(CID, clientInfo);
						}
						else if ( tulip2pHeader.method == "QUERY" )	
						{
							
							onQueryRequest(CID, resource);
						}
						else if ( tulip2pHeader.method == "COMMIT" )
						{
							onCommitRequest(CID, resource);
						}
						else if ( tulip2pHeader.method == "TRANSFER" )
						{
							onTransferRequest(CID, resource);
						}
						else if ( tulip2pHeader.method == "QUIT" )
						{
							onQuitRequest(CID);
						}
					}
					else
					{
						nana->say(Nana::COMPLAIN, ToHandleClientRequest, "REQUEST FORMAT ERROR");
					}
				}
			}


			void Tulip2pServer::onPingRequest( const std::string & id , ClientInfo & clientInfo )
			{
				#define ToOnPingRequest __func__
				
				if ( ! id.empty() )
				{
					 std::map<std::string, ClientInfo>::const_iterator iter = ClientsPool_.find( id );
					 std::string reply;
					 if ( iter == ClientsPool_.end() )
					 {
					 	//calling md5 algorithm md5(id, clientInfo.token);
					 	ClientsPool_.insert(std::make_pair(id, clientInfo));

						reply="TULIP2P/2016.2.21 200 OK\r\nNat-Address:"+id+"\r\n\r\n";
						sendto(serverFd_, reply.c_str(), reply.length(), 0,
                           		(struct sockaddr *)&(clientInfo.address), sizeof(clientInfo.address));
						nana->say(Nana::HAPPY, ToOnPingRequest, "REPLY:%s", reply.c_str());
					 }
					 else
					 {
					 	reply="TULIP2P/2016.2.21 200 OK\r\n\r\n";
						sendto(serverFd_, reply.c_str(), reply.length(), 0,
                           		(struct sockaddr *)&(clientInfo.address), sizeof(clientInfo.address));
					 	nana->say(Nana::HAPPY, ToOnPingRequest, "REPEATED PING REQUEST FROM %s, JUST IGNORE IT", id.c_str());
					 }
				}
			}


			void Tulip2pServer::onCommitRequest( const std::string & id , const std::string & resource, const std::string &desc)
			{
				#define ToOnCommit __func__
				
				if ( ! id.empty() )
				{
					 std::map<Id, ClientInfo>::iterator iter = ClientsPool_.find( id );
					 if ( iter != ClientsPool_.end() )
					 {
					 	std::string key = resource;
						std::string reply="TULIP2P/2016.2.21 200 OK\r\n\r\n";
					 	if ( ResourcesList_.find( key ) == ResourcesList_.end() )
					 	{
							ResourceInfo resourceInfo;
							resourceInfo.ID = iter->first;
							resourceInfo.desc = desc;
							memcpy(&resourceInfo.address, &iter->second.address, sizeof(resourceInfo.address));
							ResourcesList_.insert(std::make_pair(key, resourceInfo));
							nana->say(Nana::HAPPY, ToOnCommit, "%s COMMITED  FROM %s", 
									  						   resource.c_str(), id.c_str());
							
					 	}
						else
						{
							nana->say(Nana::HAPPY, ToOnCommit, "REPEATED COMMITED REQUEST ABOUT RESOURCE %s  FROM %s", 
									  						   resource.c_str(), id.c_str());
						}

						sendto(serverFd_, reply.c_str(), reply.length(), 0,
                           		(struct sockaddr *)&(iter->second.address), sizeof(iter->second.address));
					 }
					 else
					 {
					 	nana->say(Nana::HAPPY, ToOnCommit, "%s MUST PING FIRST", id.c_str());
					 }
				}
			}


			void Tulip2pServer::onQueryRequest( const std::string & id , const std::string & resource)
			{
				#define ToOnQueryRequest __func__
				
				if ( ! id.empty() )
				{
					 std::map<std::string, ClientInfo>::iterator citer = ClientsPool_.find( id );
					 if ( citer != ClientsPool_.end() )
					 {
					 	std::map<Key, ResourceInfo>::iterator riter = ResourcesList_.find( resource );

						std::string reply;
					 	if ( riter != ResourcesList_.end() )
					 	{
							reply="TULIP2P/2016.2.21 200 Ok\r\nNat-Address:"+riter->second.ID+"\r\n"
								+"Resource-Desc:"+riter->second.desc+"\r\n\r\n";
					 	}
						else
						{
							reply="TULIP2P/2016.2.21 404 Not Found\r\n\r\n";
							nana->say(Nana::HAPPY, ToOnQueryRequest, "REQUESTED RESOURCE %s NOT EXISTS", resource.c_str());
						}

						
						sendto(serverFd_, reply.c_str(), reply.length(), 0,
                           				(struct sockaddr *)&(citer->second.address), sizeof(citer->second.address));
						nana->say(Nana::HAPPY, ToOnQueryRequest, "REPLY:%s", reply.c_str());
					 }
					 else
					 {
					 	nana->say(Nana::HAPPY, ToOnQueryRequest, "%s MUST PING FIRST", id.c_str());
					 }
				}
			}



			void Tulip2pServer::onTransferRequest( const std::string & id , const std::string & resource)
			{
				#define ToOnTransferRequest __func__
				if ( ! id.empty() )
				{
					 std::map<std::string, ClientInfo>::iterator citer = ClientsPool_.find( id );
					 if ( citer != ClientsPool_.end() )
					 {
					 	std::map<Key, ResourceInfo>::iterator riter = ResourcesList_.find( resource );

						std::string reply;
					 	if ( riter != ResourcesList_.end() )
					 	{
							reply="TRANSFER /"+resource+" TULIP2P/2016.2.21\r\nNat-Address:"+id+"\r\n\r\n";
							sendto(serverFd_, reply.c_str(), reply.length(), 0,
                           				(struct sockaddr *)&(riter->second.address), sizeof(riter->second.address));
							nana->say(Nana::HAPPY, ToOnTransferRequest, "REPLY:%s", reply.c_str());
					 	}
						else
						{
							reply="TULIP2P/2016.2.21 404 Not Found\r\n\r\n";
						}
						nana->say(Nana::HAPPY, ToOnTransferRequest, "REPLY:%s", reply.c_str());
					 }
					 else
					 {
					 	nana->say(Nana::HAPPY, ToOnTransferRequest, "%s MUST PING FIRST", id.c_str());
					 }
				}
			}


			void Tulip2pServer::onQuitRequest( const std::string & id )
			{
				#define ToOnQuitRequest __func__
				if ( ! id.empty() )
				{
					 std::map<std::string, ClientInfo>::iterator citer = ClientsPool_.find( id );
					 if ( citer != ClientsPool_.end() )
					 {
					 	ClientsPool_.erase(citer);
					 }
					 else
					 {
					 	nana->say(Nana::HAPPY, ToOnQuitRequest, "%s MUST PING FIRST", id.c_str());
					 }
				}
			}
	};
};
