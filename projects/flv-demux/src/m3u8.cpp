/*
@file name:chenzhengqiang
@author:chenzhengqiang
@start date:2015/9/13
@modified date:
*/

#include "m3u8.h"
#include <cstring>

FILE * create_ts_file( char *url_buffer,size_t buffer_size, const M3U8_CONFIG & m3u8_config )
{
    if( url_buffer == NULL || m3u8_config.path.empty() || m3u8_config.channel.empty())
        return NULL;
    snprintf(url_buffer,buffer_size,"%s%s_%ld.ts",m3u8_config.path.c_str(),m3u8_config.channel.c_str(),m3u8_config.timestamp);
    FILE *fts_handler = fopen( url_buffer, "wb");
    return fts_handler;
}

FILE * create_m3u8_file( const M3U8_CONFIG & m3u8_config )
{
    if( m3u8_config.path.empty() || m3u8_config.channel.empty() || m3u8_config.timestamp <=0 )
    return NULL;
    char m3u8_file[99];
    char utf8_header[3] = {0xef, 0xbb, 0xbf};
    sprintf(m3u8_file,"%s%s.m3u8",m3u8_config.path.c_str(),m3u8_config.channel.c_str());
    remove(m3u8_file);
    FILE *fm3u8_handler = fopen(m3u8_file,"w+b");
    if( fm3u8_handler == NULL )
    return NULL;
    
    fwrite(utf8_header,sizeof(char),3,fm3u8_handler);
    return fm3u8_handler;
}

void delete_m3u8_file( FILE * fm3u8_handler )
{
    if( fm3u8_handler == NULL )
    return;   
    fclose( fm3u8_handler );
}

int write_m3u8_file_header(FILE *fm3u8_handler, M3U8_CONFIG & m3u8_config )
{
    if( fm3u8_handler == NULL )
        return -1;
    char m3ud_head[99]="#EXTM3U\r\n";
    fputs(m3ud_head,fm3u8_handler);
    sprintf(m3ud_head,"#EXT-X-MEDIA-SEQUENCE:%d\r\n",m3u8_config.media_cur_sequence);
    fputs(m3ud_head,fm3u8_handler);
    sprintf(m3ud_head,"#EXT-X-TARGETDURATION:%d\r\n",m3u8_config.target_duration);
    fputs(m3ud_head,fm3u8_handler);
}

void add_ts_url_2_m3u8_file( FILE * *fm3u8_handler, const char *ts_url, M3U8_CONFIG & m3u8_config )
{
    if( m3u8_config.media_cur_sequence > m3u8_config.media_prev_sequence )
    {
        m3u8_config.media_prev_sequence = m3u8_config.media_cur_sequence;
        delete_m3u8_file(*fm3u8_handler);
        *fm3u8_handler = NULL;
        *fm3u8_handler=create_m3u8_file(m3u8_config);
        write_m3u8_file_header(*fm3u8_handler,m3u8_config);
    }
    
    char EXTINF[99];
    sprintf(EXTINF,"#EXTINF:%d\r\n",m3u8_config.target_duration);
    fputs(EXTINF,*fm3u8_handler);
    fputs(ts_url,*fm3u8_handler);
    fputs("\r\n",*fm3u8_handler);
}
