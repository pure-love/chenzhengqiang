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
#include<list>
using std::string;
using std::list;
struct M3U8_CONFIG
{
	std::string path;
	std::string channel;
	unsigned long timestamp;
	unsigned long media_prev_sequence;
	unsigned long media_cur_sequence;
	int target_duration;
       int total_ts_urls;
       int playlist_type;
       int version;
};

FILE * create_ts_file(  std::string & ts_url,const M3U8_CONFIG & m3u8_config );
void create_m3u8_file( const M3U8_CONFIG & m3u8_config, std::string & new_m3u8_file, bool is_tmp = false );
int write_m3u8_file_header(  M3U8_CONFIG & m3u8_config );
void add_ts_url_2_m3u8_file( std::list<std::string> & ts_urls_queue, M3U8_CONFIG & m3u8_config );
#endif
