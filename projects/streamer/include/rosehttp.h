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
#include<string>
#include<map>
using std::string;
struct SIMPLE_ROSEHTTP_HEADER
{
    std::string method;
    std::string server_path;
    std::map<std::string,std::string>url_args;
    std::string http_version;
};

enum ROSEHTTP_STATUS
{
    HTTP_STATUS_200,
    HTTP_STATUS_400,
    HTTP_STATUS_401,
    HTTP_STATUS_404,
    HTTP_STATUS_500,
    HTTP_STATUS_501,
    HTTP_STATUS_503
};


int   read_rosehttp_header( int sock_fd, void *buffer, size_t buffer_size );
int parse_simple_rosehttp_header( const char *cstr_http_header, size_t http_header_length, SIMPLE_ROSEHTTP_HEADER  & req_info );
int rosehttp_reply_with_status( const int status, int sock_fd );
#endif
