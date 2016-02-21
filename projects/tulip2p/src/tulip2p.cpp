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
#include "tulip2p.h"


namespace czq
{
	
	namespace Tulip2p
	{
		static const char * SimpleTulip2pReply[]=
		{   
		    "TULIP2P/2016.2.21 200 OK\r\n\r\n",
		    "TULIP2P/2016.2.21 206 Partial Content\r\n\r\n",		
		    "TULIP2P/2016.2.21 301 Moved Permanently\r\n\r\n",		
		    "TULIP2P/2016.2.21 400 Bad Request\r\n\r\n",
		    "TULIP2P/2016.2.21 401 Unauthorized\r\n\r\n",
		    "TULIP2P/2016.2.21 404 Not Found\r\n\r\n",
		    "TULIP2P/2016.2.21 500 Internal Server Error\r\n\r\n",
		    "TULIP2P/2016.2.21 501 Not Implemented\r\n\r\n",
		    "TULIP2P/2016.2.21 503 Service Unavailable\r\n\r\n"
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



		ssize_t parseTulip2pHeader( char *cstrHttpHeader, size_t length, Tulip2pHeader  & tulip2pHeader )
		{
		 	if( cstrHttpHeader == NULL || length == 0 )
		    	{
		       	return ARGUMENT_ERROR;
		    	}

			char requestLine[1024]={0};
			char * pos = strstr(cstrHttpHeader, "\r\n");
			if ( pos == 0 )
			{
				return STREAM_FORMAT_ERROR;
			}

			if ( (size_t) (pos - cstrHttpHeader+2) > sizeof(requestLine) )
			{
				return STREAM_LENGTH_ERROR;
			}

			strncpy(requestLine, cstrHttpHeader, pos - cstrHttpHeader+2);
			
			char *curPos = strchr(requestLine, ' ');
			char *prevPos = curPos;
			if ( curPos == 0 )
			{
				return STREAM_FORMAT_ERROR;
			}

			//GET /swwy.com http/2016.2.21\r\n
			tulip2pHeader.method.assign(requestLine, curPos-requestLine);
			bool hasArg = true;
			curPos = strchr(prevPos+1, '?');
			if ( curPos == 0 )
			{
				hasArg = false;
				curPos = strchr(prevPos+1, '=');
				if ( curPos != 0 )
				{
					return STREAM_FORMAT_ERROR;
				}
				
				else
				{
					curPos = strchr(prevPos+1, ' ');
					if ( curPos == 0 )
					{
						return STREAM_FORMAT_ERROR;
					}
				}
			}
			
			tulip2pHeader.serverPath.assign(prevPos, curPos-prevPos);
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

					tulip2pHeader.urlArgs.insert(std::make_pair(key,value));
        				key="";
        				value="";
					if ( *prevPos == ' ')
					break;
					else if ( ! *prevPos )
					return STREAM_FORMAT_ERROR;	
        				++prevPos;
   				 }
			}
			
			
			curPos = strstr(prevPos, "TULIP2P/");
			if ( curPos != 0 )
			{
				prevPos = curPos+5;
				curPos = strstr(prevPos,"\r\n");
				if ( curPos != 0 )
				{
					tulip2pHeader.version.assign(prevPos, curPos-prevPos);
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


		ssize_t replyWithTulip2pStatus( int status, int sockFd )
		{
		    Tulip2pStatus tulip2pStatus;
		    switch( status )
		    {
		        case 200:
		             tulip2pStatus = TULIP2P_STATUS_200;
			      break;
			 case 206:
			 	tulip2pStatus = TULIP2P_STATUS_206;
			 	break;
			 case 301:
			 	tulip2pStatus = TULIP2P_STATUS_301;
			      break;
		        case 400:
		            tulip2pStatus = TULIP2P_STATUS_400;
		            break;
		        case 401:
		            tulip2pStatus = TULIP2P_STATUS_401;
		            break;
		        case 404:
		            tulip2pStatus = TULIP2P_STATUS_404;
			    break;
		        case 500:
		            tulip2pStatus = TULIP2P_STATUS_500;
		            break;
		        case 501:
		            tulip2pStatus = TULIP2P_STATUS_501;
			    break;
		        case 503:
		            tulip2pStatus = TULIP2P_STATUS_503;
		            break;
		    }
		    return NetUtil::writeSpecifySize2( sockFd, SimpleTulip2pReply[tulip2pStatus], strlen(SimpleTulip2pReply[tulip2pStatus]));
		}
	}
};
