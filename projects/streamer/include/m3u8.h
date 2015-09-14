/*
@file name:chenzhengqiang
@author:chenzhengqiang
@start date:2015/9/13
@modified date:
*/

#ifndef _CZQ_M3U8_H_
#define _CZQ_M3U8_H_
#include<cstdio>
#include<string>
using std::string;
struct M3U8_CONFIG
{
	std::string path;
	std::string channel;
	unsigned long timestamp;
	unsigned long media_prev_sequence;
	unsigned long media_cur_sequence;
	int target_duration;
};

FILE * create_ts_file( char *url_buffer,size_t buffer_size, const M3U8_CONFIG & m3u8_config );
FILE * create_m3u8_file( const M3U8_CONFIG & m3u8_config );
void delete_m3u8_file( FILE * fm3u8_handler );
int write_m3u8_file_header(FILE *fm3u8_handler, M3U8_CONFIG & m3u8_config );
void add_ts_url_2_m3u8_file( FILE * *fm3u8_handler, const char *ts_url, M3U8_CONFIG & m3u8_config );
#endif
