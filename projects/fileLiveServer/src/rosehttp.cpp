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



		ssize_t parseSimpleRoseHttpHeader( char *cstrHttpHeader, size_t length, SimpleRoseHttpHeader  & simpleRoseHttpHeader )
		{
			#define FILTER_SPACE() \
			while ( *cstrHttpHeader )\
			{\
				if ( *(cstrHttpHeader) == ' ')\
				{\
					++(cstrHttpHeader);\
				}\
				else\
				{\
					break;\
				}\
			}\
			if ( ! *cstrHttpHeader )\
			{\
				return STREAM_LENGTH_ERROR;\
			}
			
		 	if( cstrHttpHeader == NULL || length == 0 )
		    	{
		       	return ARGUMENT_ERROR;
		    	}

			char *curPos = strstr(cstrHttpHeader, " ");
			char *prevPos = curPos;
			if ( curPos == 0 )
			{
				return STREAM_FORMAT_ERROR;
			}

			//GET /swwy.com http/1.1\r\n
			simpleRoseHttpHeader.method.assign((char *)cstrHttpHeader, curPos-cstrHttpHeader);
			FILTER_SPACE();
			bool hasArg = true;
			curPos = strstr(prevPos+1, "?");
			if ( curPos == 0 )
			{
				 hasArg = false;
				curPos = strstr(prevPos+1, "=");
				if ( curPos != 0 )
				{
					return STREAM_FORMAT_ERROR;
				}
				
				else
				{
					curPos = strstr(prevPos+1, " ");
					if ( curPos == 0 )
					{
						return STREAM_FORMAT_ERROR;
					}
				}
			}

			simpleRoseHttpHeader.serverPath.assign(prevPos, curPos-prevPos);
			prevPos = curPos+1;
			
			if ( hasArg )
			{
				std::string key,value;
				while( *prevPos )
    				{
        				while( * prevPos && *prevPos != ' ' && *prevPos != '=')
        				{
            					key.push_back(*prevPos);
            					++prevPos;
        				}
        
       				 if( *prevPos == ' ' )
        				{
            					while( * prevPos && *prevPos==' ')
            					{
                					++prevPos;
            					}
						if ( ! *prevPos )
						{
							return STREAM_FORMAT_ERROR;
						}
        				}
        				else if( *prevPos == '=' )
        				++prevPos;
					else
					{
						return STREAM_FORMAT_ERROR;
					}

       
        				while( *prevPos && *prevPos != ' ' && *prevPos != '&' )
        				{
            					value.push_back(*prevPos);
            					++prevPos;
        				}

					simpleRoseHttpHeader.urlArgs.insert(std::make_pair(key,value));
        				key="";
        				value="";
					if ( *prevPos == ' ')
					break;
					else if ( ! *prevPos )
					return STREAM_FORMAT_ERROR;	
        				++prevPos;
   				 }
			}

			curPos = strstr(prevPos, "http/");
			if ( curPos == 0 )
			{
				curPos = strstr(prevPos, "HTTP/");
			}
			
			if ( curPos != 0 )
			{
				prevPos = curPos+5;
				curPos = strstr(prevPos,"\r\n");
				if ( curPos != 0 )
				{
					simpleRoseHttpHeader.version.assign(prevPos, curPos-prevPos);
				}
				else
				{
					return STREAM_FORMAT_ERROR;
				}
			}
			else
			{
				return STREAM_FORMAT_ERROR;
			}
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
