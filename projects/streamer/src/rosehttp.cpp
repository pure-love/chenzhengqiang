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
#include <iostream>
using namespace std;
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
    if( cstr_http_header == NULL )
    {
        return ARGUMENT_ERROR;
    }

    const char *start = cstr_http_header;
    while( *cstr_http_header )
    {
        if( *cstr_http_header != ' ' )
            break;
        ++cstr_http_header;
        if( (size_t) ( cstr_http_header - start ) == http_header_length )
        break;    
    }
    
    if( (size_t) ( cstr_http_header - start ) == http_header_length )
    {
        return STREAM_FORMAT_ERROR;
    }
    std::string string_http_header( cstr_http_header );
    if(
     ( 
	( string_http_header.find("get")  == std::string::npos && string_http_header.find("GET") == std::string::npos  )
	&& 
	( string_http_header.find("post") == std::string::npos && string_http_header.find("POST") == std::string::npos )
     )	
     ||( string_http_header.find("http/") == std::string::npos && string_http_header.find("HTTP/") == std::string::npos )
     )
     
    {
        return STREAM_FORMAT_ERROR;
    }

    size_t prev_pos=0,cur_pos=0;
    cur_pos = string_http_header.find_first_of(' ',prev_pos);
    if( cur_pos == std::string::npos )
    {
        return STREAM_FORMAT_ERROR;
    }

    simple_rosehttp_header.method = string_http_header.substr( prev_pos,cur_pos );
    std::string::iterator iter = simple_rosehttp_header.method.begin();
    while( iter != simple_rosehttp_header.method.end() )
    {
        *iter = toupper( *iter );
        ++iter;
    }
    prev_pos = cur_pos;
    
    cur_pos = string_http_header.find_first_of( '/', prev_pos );
    if( cur_pos == std::string::npos )
    {
        return STREAM_FORMAT_ERROR;    
    }
    
    simple_rosehttp_header.server_path = string_http_header.substr( prev_pos+1, cur_pos-prev_pos-1 );
    prev_pos = cur_pos+1;

    cur_pos = string_http_header.find_first_of( '=',prev_pos);
    std::string key;
    std::string value;
    
    while( cur_pos != std::string::npos )
    {
        key="";
        value="";
        key = string_http_header.substr( prev_pos, cur_pos-prev_pos );
        prev_pos=cur_pos+1;
        cur_pos = string_http_header.find_first_of('&',prev_pos);
        if( cur_pos == std::string::npos )
        {
            cur_pos = string_http_header.find_first_of(' ', prev_pos );
            if( cur_pos == std::string::npos )
            break;
        }
        
        value = string_http_header.substr( prev_pos, cur_pos-prev_pos);
        simple_rosehttp_header.url_args.insert(std::make_pair( key, value ));
        prev_pos=cur_pos+1;
        cur_pos = string_http_header.find_first_of( '=',prev_pos );
    }
    
    cur_pos = string_http_header.find_first_of( '/',prev_pos );
    if( cur_pos == std::string::npos )
    return STREAM_FORMAT_ERROR;    
    prev_pos = cur_pos;
    cur_pos = string_http_header.find_first_of("\r\n",prev_pos);
    if( cur_pos == std::string::npos )
    {
        return STREAM_FORMAT_ERROR;
    }

    simple_rosehttp_header.http_version = string_http_header.substr( prev_pos+1, cur_pos-prev_pos );
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

