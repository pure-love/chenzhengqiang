/*
*@filename:rosehttp.h
*@author:chenzhengqiang
*@start date:2015/11/13 08:43:26
*@modified date:
*@desc: 
*/



#ifndef _CZQ_ROSEHTTP_H_
#define _CZQ_ROSEHTTP_H_
//write the function prototypes or the declaration of variables here
#include "nana.h"
#include<string>
#include<map>
using std::string;
namespace czq
{
	namespace RoseHttp
	{
		struct SimpleRoseHttpHeader
		{
		    std::string method;
		    std::string serverPath;
		    std::map<std::string,std::string>urlArgs;
		    std::string version;
		};


		enum RoseHttpStatus
		{
		    HTTP_STATUS_200,
		    HTTP_STATUS_301,		
		    HTTP_STATUS_400,
		    HTTP_STATUS_401,
		    HTTP_STATUS_404,
		    HTTP_STATUS_500,
		    HTTP_STATUS_501,
		    HTTP_STATUS_503
		};


		ssize_t   readRoseHttpHeader( int sockFd, void *buffer, size_t bufferSize );
		ssize_t parseSimpleRoseHttpHeader( const char *cstrHttpHeader, size_t length, SimpleRoseHttpHeader  & requestInfo );
		ssize_t replyWithRoseHttpStatus( const int status, int sockFd, const std::string & responseHeader ="", Nana *nana=0);
	};
};
#endif
