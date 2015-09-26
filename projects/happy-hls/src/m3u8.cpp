/*
@file name:chenzhengqiang
@author:chenzhengqiang
@start date:2015/9/13
@modified date:
*/

#include "m3u8.h"
#include<cstring>
#include<sstream>
FILE * create_ts_file( std::string &ts_url, const M3U8_CONFIG & m3u8_config )
{
    if( m3u8_config.path.empty() || m3u8_config.channel.empty())
        return NULL;
    std::ostringstream OSS_ts_url;
    OSS_ts_url<<m3u8_config.channel<<"_"<<m3u8_config.media_cur_sequence<<".ts";
    ts_url=OSS_ts_url.str();
    std::string ts_full_path=m3u8_config.path+ts_url;
    FILE *fts_handler = fopen( ts_full_path.c_str(), "wb" );
    return fts_handler;
}

void create_m3u8_file( const M3U8_CONFIG & m3u8_config, std::string & new_m3u8_file, bool is_tmp )
{
    if( m3u8_config.path.empty() || m3u8_config.channel.empty() || m3u8_config.timestamp <=0 )
    return;
    new_m3u8_file=m3u8_config.path+m3u8_config.channel+".m3u8";
    if( is_tmp )
    {
        new_m3u8_file=m3u8_config.path+m3u8_config.channel+"_new.m3u8";
    }
    
    FILE *fm3u8_handler = fopen( new_m3u8_file.c_str() , "w+b" );
    if( fm3u8_handler == NULL )
    return;
    char m3u8_head[199]="#EXTM3U\r\n";
    snprintf( m3u8_head,sizeof(m3u8_head),"#EXTM3U\n"\
                                                            "#EXT-X-TARGETDURATION:%d\n"\
                                                            "#EXT-X-VERSION:%d\n"\
                                                            "#EXT-X-MEDIA-SEQUENCE:%ld\n",
                                                            m3u8_config.target_duration,
                                                            m3u8_config.version,
                                                            m3u8_config.media_cur_sequence > 3 ?
                                                            m3u8_config.media_cur_sequence-m3u8_config.total_ts_urls+2:1
                                                            );
    fputs( m3u8_head,fm3u8_handler );
    fclose( fm3u8_handler );
}



void add_ts_url_2_m3u8_file( std::list<std::string> & ts_urls_pool, M3U8_CONFIG & m3u8_config )
{
    if( ts_urls_pool.empty() )
        return;
    char EXTINF[99];

    std::string m3u8_old = m3u8_config.path+m3u8_config.channel+".m3u8";
    FILE *fm3u8_old_hanlder = fopen( m3u8_old.c_str(), "rw+" );
    if( ts_urls_pool.size() < (size_t) m3u8_config.total_ts_urls )
    {
        std::string ts_url = ts_urls_pool.back();
        fseek( fm3u8_old_hanlder,0, SEEK_END );
        snprintf( EXTINF, sizeof(EXTINF),"#EXTINF:%d,\n%s\n",m3u8_config.target_duration,ts_url.c_str());
        fputs( EXTINF, fm3u8_old_hanlder );
        fclose( fm3u8_old_hanlder);
    }
    else
    {
        std::string new_m3u8_file;
        create_m3u8_file( m3u8_config, new_m3u8_file , true );
        ts_urls_pool.pop_front();
        std::list<std::string>::iterator ts_urls_iter = ts_urls_pool.begin();
        FILE *fm3u8_handler_new = fopen( new_m3u8_file.c_str(), "rw+" );
        fseek( fm3u8_handler_new, 0, SEEK_END );
        while( ts_urls_iter != ts_urls_pool.end() )
        {
            snprintf(EXTINF,sizeof(EXTINF),"#EXTINF:%d,\n%s\n",
                                m3u8_config.target_duration, ts_urls_iter->c_str());
            fputs( EXTINF, fm3u8_handler_new );
            ++ts_urls_iter;
            
        }
        fclose( fm3u8_handler_new );
        rename( new_m3u8_file.c_str(), m3u8_old.c_str() );
    }
}

