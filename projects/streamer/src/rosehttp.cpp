/*
*@filename:rosehttp.cpp
*@author:chenzhengqiang
*@start date:2015/11/13 08:43:26
*@modified date:
*@desc: 
*/

#include "errors.h"
#include "netutility.h"
#include "rosehttp.h"
#include "common.h"
#include "logging.h"


static const char * SIMPLE_ROSEHTTP_REPLY[]=
{   
    "HTTP/1.1 200 OK\r\n\r\n",
    "HTTP/1.1 400 Bad Request\r\n\r\n",
    "HTTP/1.1 401 Unauthorized\r\n\r\n",
    "HTTP/1.1 404 Not Found\r\n\r\n",
    "HTTP/1.1 500 Internal Server Error\r\n\r\n",
    "HTTP/1.1 501 Not Implemented\r\n\r\n",
    "HTTP/1.1 503 Service Unavailable\r\n\r\n"
};


/*
#@returns:0 ok,-1 error or it indicates that clients has close the socket
          others indicates try again
#@desc:
*/
int read_rosehttp_header( int sock_fd, void *buffer, size_t buffer_size )
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
               log_module( LOG_ERROR,"READ_HTTP_HEADER","SYSTEM ERROR:%s",strerror(errno));\
               return 0;\
           }\
     }\
     else\
     {\
	    log_module( LOG_ERROR, "READ_HTTP_HEADER", "READ 0 BYTE CLIENT DISCONNECTED RELATED TO SOCK FD:%d", sock_fd );\
	    return 0;\
     }	
              
    int received_bytes;
    size_t left_bytes = 0;
    size_t read_bytes = 0;
    char *buffer_forward = (char *)buffer;

    while(  true )
    {
         if( ( received_bytes = read( sock_fd,buffer_forward,sizeof(char))) <= 0 )
         {
              JUDGE_RETURN( received_bytes );
         }
         
         read_bytes += received_bytes;
         if( read_bytes == buffer_size )
         {
              return LENGTH_OVERFLOW;
         }

         if( *buffer_forward == '\r' )
         {
              buffer_forward += received_bytes; 
              left_bytes = 3*sizeof( char );
	      if( ( read_bytes + left_bytes ) > buffer_size )
	      {
		    return LENGTH_OVERFLOW;		 
	      }

              while( true )
              {
                   if( (received_bytes = read( sock_fd,buffer_forward,left_bytes )) <= 0 )
                   {
                        JUDGE_RETURN( received_bytes );
                   }
                   
                   read_bytes+=received_bytes;
                   buffer_forward += received_bytes;
                   left_bytes -= received_bytes;
                   if( left_bytes == 0 )
                   break; 
              }
              if( *--buffer_forward == '\n' )
              {
                    return read_bytes;
              }
              ++buffer_forward;
         }
         else
         {
              buffer_forward+=received_bytes; 
         }
    }
    
    log_module( LOG_DEBUG, "READ_HTTP_HEADER", "READ THE HTTP HEADER BYTE BY BYTE SOCK FD:%d DONE", sock_fd );
    return read_bytes;
}



int parse_simple_rosehttp_header( const char *cstr_http_header, size_t http_header_length, SIMPLE_ROSEHTTP_HEADER  & simple_rosehttp_header )
{
 	if( cstr_http_header == NULL || http_header_length == 0 )
	{
		 return ARGUMENT_ERROR;
	}

	char requestLine[1024]={0};
	char * pos = (char *)strstr(cstr_http_header, "\r\n");
	if ( pos == 0 )
	{
		return STREAM_FORMAT_ERROR;
	}

	if ( (size_t) (pos - cstr_http_header+2) > sizeof(requestLine) )
	{
		return STREAM_LENGTH_ERROR;
	}

	strncpy(requestLine, cstr_http_header, pos - cstr_http_header+2);
			
	char *curPos = strchr(requestLine, ' ');
	char *prevPos = curPos;
	if ( curPos == 0 )
	{
		return STREAM_FORMAT_ERROR;
	}

	//GET /swwy.com http/1.1\r\n
	simple_rosehttp_header.method.assign(requestLine, curPos-requestLine);
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
			
	simple_rosehttp_header.server_path.assign(prevPos, curPos-prevPos);
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

			simple_rosehttp_header.url_args.insert(std::make_pair(key,value));
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
			simple_rosehttp_header.http_version.assign(prevPos, curPos-prevPos);
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


int rosehttp_reply_with_status( const int status, int sock_fd )
{
    ROSEHTTP_STATUS http_status;
    switch( status )
    {
        case 200:
            http_status = HTTP_STATUS_200;
	    break;
        case 400:
            http_status = HTTP_STATUS_400;
            break;
        case 401:
            http_status = HTTP_STATUS_401;
            break;
        case 404:
            http_status = HTTP_STATUS_404;
	    break;
        case 500:
            http_status = HTTP_STATUS_500;
            break;
        case 501:
            http_status = HTTP_STATUS_501;
	    break;
        case 503:
            http_status = HTTP_STATUS_503;
            break;
    }
    return write_specify_size2( sock_fd, SIMPLE_ROSEHTTP_REPLY[http_status], strlen( SIMPLE_ROSEHTTP_REPLY[http_status] ) );
}

