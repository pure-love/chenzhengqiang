/*
*@filename:rosehttp.cpp
*@author:chenzhengqiang
*@start date:2016/2/1 12:51
*@modified date:
*@desc: 
*/

#include "errors.h"
#include "common.h"
#include "netutil.h"
#include "rosehttp.h"
#include "fileliveserver.h"


namespace czq
{
	
	namespace RoseHttp
	{
		static const char * SimpleRoseHttpReply[]=
		{   
		    "HTTP/1.1 200 OK\r\n",
		    "HTTP/1.1 301 Moved Permanently\r\n",		
		    "HTTP/1.1 400 Bad Request\r\n",
		    "HTTP/1.1 401 Unauthorized\r\n",
		    "HTTP/1.1 404 Not Found\r\n",
		    "HTTP/1.1 500 Internal Server Error\r\n",
		    "HTTP/1.1 501 Not Implemented\r\n",
		    "HTTP/1.1 503 Service Unavailable\r\n"
		};


		/*
		#@returns:0 ok,-1 error or it indicates that clients has close the socket
		          others indicates try again
		#@desc:
		*/
		ssize_t readRoseHttpHeader( int sockFd, void *buffer, size_t bufferSize )
		{
		    #define JUDGE_RETURN(X) \
		    if( (X) < 0 )\
		    {\
		           if( errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK )\
		           {\
		               (X) = 0;\
		           }\
		           else\
		           {\
		               return 0;\
		           }\
		    }\
		    else\
		    {\
			   return 0;\
		    }	
		              
		    ssize_t receivedBytes;
		    size_t leftBytes = 0;
		    size_t readBytes = 0;
		    char *bufferForward = (char *)buffer;

		    while(  true )
		    {
		         if( ( receivedBytes = read( sockFd,bufferForward,sizeof(char))) <= 0 )
		         {
		              JUDGE_RETURN( receivedBytes );
		         }
		         
		         readBytes += receivedBytes;
		         if( readBytes == bufferSize )
		         {
		              return LENGTH_OVERFLOW;
		         }

		         if( *bufferForward == '\r' )
		         {
		              bufferForward += receivedBytes; 
		              leftBytes = 3*sizeof( char );
			      if( ( readBytes + leftBytes ) > bufferSize )
			      {
				    return LENGTH_OVERFLOW;		 
			      }

		              while( true )
		              {
		                   if( (receivedBytes = read( sockFd,bufferForward,leftBytes )) <= 0 )
		                   {
		                        JUDGE_RETURN( receivedBytes );
		                   }
		                   
		                   readBytes+=receivedBytes;
		                   bufferForward += receivedBytes;
		                   leftBytes -= receivedBytes;
		                   if( leftBytes == 0 )
		                   break; 
		              }
		              if( *--bufferForward == '\n' )
		              {
		                    return readBytes;
		              }
		              ++bufferForward;
		         }
		         else
		         {
		              bufferForward+=receivedBytes; 
		         }
		    }
		    
		    return readBytes;
		}



		ssize_t parseSimpleRoseHttpHeader( const char *cstrHttpHeader, size_t length, SimpleRoseHttpHeader  & simpleRoseHttpHeader )
		{
		    if( cstrHttpHeader == NULL )
		    {
		        return ARGUMENT_ERROR;
		    }

		    const char *start = cstrHttpHeader;
		    while( *cstrHttpHeader )
		    {
		        if( *cstrHttpHeader != ' ' )
		            break;
		        ++cstrHttpHeader;
		        if( (size_t) ( cstrHttpHeader - start ) == length )
		        break;    
		    }
		    
		    if( (size_t) ( cstrHttpHeader - start ) == length )
		    {
		        return STREAM_FORMAT_ERROR;
		    }
		    std::string strHttpHeader( cstrHttpHeader );
		    if(
		     ( 
			( strHttpHeader.find("get")  == std::string::npos && strHttpHeader.find("GET") == std::string::npos  )
			&& 
			( strHttpHeader.find("post") == std::string::npos && strHttpHeader.find("POST") == std::string::npos )
		     )	
		     ||( strHttpHeader.find("http/") == std::string::npos && strHttpHeader.find("HTTP/") == std::string::npos )
		     )
		     
		    {
		        return STREAM_FORMAT_ERROR;
		    }

		    size_t prev_pos=0,cur_pos=0;
		    cur_pos = strHttpHeader.find_first_of(' ', prev_pos);
		    if( cur_pos == std::string::npos )
		    {
		        return STREAM_FORMAT_ERROR;
		    }

		    simpleRoseHttpHeader.method = strHttpHeader.substr( prev_pos,cur_pos );
		    std::string::iterator iter = simpleRoseHttpHeader.method.begin();
		    while( iter != simpleRoseHttpHeader.method.end() )
		    {
		        *iter = static_cast<char>(toupper( *iter ));
		        ++iter;
		    }
			
		    prev_pos = cur_pos+1;
		    cur_pos = strHttpHeader.find_first_of('/', prev_pos );
		    if( cur_pos == std::string::npos )
		    {
		        return STREAM_FORMAT_ERROR;    
		    }

		    prev_pos = cur_pos+1;
		    cur_pos = strHttpHeader.find_first_of('?', prev_pos);
		    if( cur_pos == std::string::npos )
		    {
		         cur_pos = strHttpHeader.find_first_of('=', prev_pos);
			  if ( cur_pos != std::string::npos )
			  {
			  	return STREAM_FORMAT_ERROR;
			  }
			  else
			  {
			  	cur_pos = strHttpHeader.find_first_of(' ', prev_pos);
				if ( cur_pos == std::string::npos )
				{
					return STREAM_FORMAT_ERROR;
				}
			  }
		    }	
			
		    simpleRoseHttpHeader.serverPath = strHttpHeader.substr( prev_pos, cur_pos-prev_pos);
		    prev_pos = cur_pos+1;

		    cur_pos = strHttpHeader.find_first_of('=', prev_pos);
		    std::string key;
		    std::string value;
		    
		    while( cur_pos != std::string::npos )
		    {
		        key="";
		        value="";
		        key = strHttpHeader.substr( prev_pos, cur_pos-prev_pos );
		        prev_pos=cur_pos+1;
		        cur_pos = strHttpHeader.find_first_of('&',prev_pos);
		        if( cur_pos == std::string::npos )
		        {
		            cur_pos = strHttpHeader.find_first_of(' ', prev_pos );
		            if( cur_pos == std::string::npos )
		            break;
		        }
		        
		        value = strHttpHeader.substr( prev_pos, cur_pos-prev_pos);
		        simpleRoseHttpHeader.urlArgs.insert(std::make_pair( key, value ));
		        prev_pos=cur_pos+1;
		        cur_pos = strHttpHeader.find_first_of( '=',prev_pos );
		    }
		    
		    cur_pos = strHttpHeader.find("HTTP/");
		    if ( cur_pos ==  std::string::npos )
		    {
			  cur_pos = strHttpHeader.find("http/");
			  if ( cur_pos == std::string::npos )
			  return 	STREAM_FORMAT_ERROR;
		    }
		    prev_pos = cur_pos+5;
		    cur_pos = strHttpHeader.find("\r\n", prev_pos);
		    if ( cur_pos == std::string::npos )
		    return STREAM_FORMAT_ERROR;		
		    simpleRoseHttpHeader.version = strHttpHeader.substr( prev_pos, cur_pos-prev_pos );
		    return OK;
		}


		ssize_t replyWithRoseHttpStatus( const int status, int sockFd, 
											const std::string & responseHeader, Nana* nana)
		{
		    RoseHttpStatus roseHttpStatus;
		    switch( status )
		    {
		        case 200:
		             roseHttpStatus = HTTP_STATUS_200;
			      break;
			 case 301:
			 	roseHttpStatus = HTTP_STATUS_301;
			      break;
		        case 400:
		            roseHttpStatus = HTTP_STATUS_400;
		            break;
		        case 401:
		            roseHttpStatus = HTTP_STATUS_401;
		            break;
		        case 404:
		            roseHttpStatus = HTTP_STATUS_404;
			    break;
		        case 500:
		            roseHttpStatus = HTTP_STATUS_500;
		            break;
		        case 501:
		            roseHttpStatus = HTTP_STATUS_501;
			    break;
		        case 503:
		            roseHttpStatus = HTTP_STATUS_503;
		            break;
		    }

		    
		    if ( !responseHeader.empty() )
		    {
			  std::string response = "HTTP/1.1 301 Moved Permanently\r\n";
			  response+=responseHeader;
			  nana->say(Nana::HAPPY, __func__, "RESPONSE:%s", response.c_str());
			  return NetUtil::writeSpecifySize2(sockFd, response.c_str(), response.length());
		    }
		    else
		    return NetUtil::writeSpecifySize2( sockFd, SimpleRoseHttpReply[roseHttpStatus], strlen(SimpleRoseHttpReply[roseHttpStatus]));
		}
	}
};
