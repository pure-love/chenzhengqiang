/*
*@filename:tulipserver.h
*@author:chenzhengqiang
*@start date:2016/02/21 11:16:19
*@modified date:
*@desc: 
*/



#ifndef _CZQ_TULIPSERVER_H_
#define _CZQ_TULIPSERVER_H_

//write the function prototypes or the declaration of variables here
#include "serverutil.h"
#include <netinet/in.h>
#include <stdint.h>
#include <map>
#include <string>
using std::string;

namespace czq
{
    namespace service
    {
        
            class Tulip2pServer
            {
                public:
    			Tulip2pServer(const ServerUtil::ServerConfig & serverConfig);
    			~Tulip2pServer();
    			void printHelp();
    			void printVersion();
    			void registerServer( int serverFd );
    			void serveForever();
    		private:
    		        ServerUtil::ServerConfig serverConfig_;
    		        int serverFd_;
    		        struct ClientInfo
    		        {
    		            struct sockaddr_in address;
    		            uint8_t token[32];
    		        };

                     struct ResourceInfo
                     {
                        struct sockaddr_in address;
                        std::string ID;
                        std::string desc;
                     };
                     
                     typedef std::string Key;
                     typedef std::string Id;
                     
    		        std::map<Key, ResourceInfo> ResourcesList_;
    		        std::map<Id, ClientInfo> ClientsPool_;
    		        enum{PING=1,QUERY=2, COMMIT=3, QUIT=4};
    		        
    		 private:
    		       void handleClientRequest();
    		       void onPingRequest(const std::string & id, ClientInfo & clientInfo); 
    		       void onCommitRequest( const std::string & id , const std::string & resource, const std::string &desc="no description");
    		       void onQueryRequest( const std::string & id , const std::string & resource);
    		       void onTransferRequest( const std::string & id , const std::string & resource);
    		       void onQuitRequest( const std::string & id );
            };
    };
};
#endif
