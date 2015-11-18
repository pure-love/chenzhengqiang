/*
 * author:chenzhengqiang
 * start date:2015/11/17
 * desc:unit test for testing rosehttp
 * */


#include"rosehttp.h"
#include<cstring>
#include<iostream>
using namespace std;


int main( int argc, char ** argv )
{
    (void)argc;
    (void)argv;
    //static const char *request1="get MLGB/channel=123 http/1.1\r\n\r\n";
    static const char *request2="post MLGB/channel=123&src=192.168.1.11:54321&type=flv http/1.1\r\n\r\n";
    SIMPLE_ROSEHTTP_HEADER srt_header;
    parse_simple_rosehttp_header( request2, strlen(request2), srt_header );
    cout<<"method:"<<srt_header.method<<endl;
    cout<<"server_path:"<<srt_header.server_path<<endl;
    cout<<"channel:"<<srt_header.url_args["channel"]<<endl;
    cout<<"src:"<<srt_header.url_args["src"]<<endl;
    cout<<"type:"<<srt_header.url_args["type"]<<endl;
    cout<<"version:"<<srt_header.http_version<<endl;
    return 0;
}
