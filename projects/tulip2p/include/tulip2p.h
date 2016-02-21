/*
*@filename:rosehttp.h
*@author:chenzhengqiang
*@start date:2015/11/13 08:43:26
*@modified date:
*@desc: 
*/



#ifndef _CZQ_ROSETULIP2P_H_
#define _CZQ_ROSETULIP2P_H_
//write the function prototypes or the declaration of variables here
#include<string>
#include<map>
using std::string;
namespace czq
{
	namespace Tulip2p
	{
		struct Tulip2pHeader
		{
		    std::string method;
		    std::string serverPath;
		    std::map<std::string,std::string>urlArgs;
		    std::string version;
		};


		enum Tulip2pStatus
		{
		    TULIP2P_STATUS_200,
		    TULIP2P_STATUS_206,
		    TULIP2P_STATUS_301,		
		    TULIP2P_STATUS_400,
		    TULIP2P_STATUS_401,
		    TULIP2P_STATUS_404,
		    TULIP2P_STATUS_500,
		    TULIP2P_STATUS_501,
		    TULIP2P_STATUS_503
		};

		ssize_t parseTulip2pHeader( char *cstrHttpHeader, size_t length,   Tulip2pHeader &  tulip2pHeader );
		ssize_t replyWithTulip2pStatus( int status, int sockFd );
	};
};
#endif
