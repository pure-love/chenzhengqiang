/*
@file name:flv.h
@author:chenzhengqiang
@version:1.0
@start date:2015/8/27
@modified date:
@desc:providing the apis of handleing flv's mux and demux
*/

#ifndef _CZQ_FLV_DEMUX_H_
#define _CZQ_FLV_DEMUX_H_
#include "flv.h"
#include<cstdio>
#include<stdint.h>


//the flv's aac sequence header
struct FLV_AAC_SEQUENCE_HEADER
{
        uint8_t sample_index1:3;
        uint8_t object_type:5;
        uint8_t reserved:3;//000
        uint8_t channel:4;
        uint8_t sample_index2:1;	
};



struct FLV_AVC_SEQUENCE_HEADER
{
        uint8_t configure_version;
        uint8_t avc_profile_indication;
        uint8_t profile_compatibility;
        uint8_t avc_level_indication;
        uint8_t reserved1:6;
        uint8_t nal_unit_length:2;
        uint8_t reserved:3;
        uint8_t sps_size:5;
        uint8_t pps_size;
};


//the adts
struct AAC_ADTS_HEADER
{
	unsigned	char check1;
	
	unsigned    char protection:1;//误码校验1
	unsigned    char layer:2;//哪个播放器被使用0x00
	unsigned    char ver:1;//版本 0 for MPEG-4, 1 for MPEG-2
	unsigned    char check2:4;
	
	unsigned    char channel1:1;
	unsigned    char privatestream:1;//0
	unsigned    char SamplingIndex:4;//采样率
	unsigned    char ObjectType:2;
	
	unsigned    char length1:2;
	unsigned	   char copyrightstart:1;//0
	unsigned    char copyrightstream:1;//0
	unsigned    char home:1;//0
	unsigned    char originality:1;//0
	unsigned    char channel2:2;
	unsigned    char length2;
	unsigned    char check3:5;
	unsigned    char length3:3;
	
	unsigned    char frames:2;//超过一块写
	unsigned    char check4:6;
};


//the h264 space
static const int H264_SPACE=0x010000;


//the common used data structure for storing the flv tag header info
struct FLV_TAG_HEADER_INFO
{
           int tag_type;
           int tag_timestamp;
           int tag_data_size;
};


//class flv_demux inherited from base class flv
//for the common interface's sake
//varify the flv format and so on
class flv_demux:public flv
{
    public:
        flv_demux( const char * flv_file );
        ~flv_demux();
        void demux_for_aac( const char * aac_file );
        void demux_for_h264( const char * h264_file );
        void demux_meanwhile_mux_2_ts();
        void get_aac_adts_frame( uint8_t *flv_aac_packet, uint8_t *aac_adts_frame, const int & data_size);
    public:
        enum{DEMUX_H264,DEMUX_AAC};
    private:
        void get_flv_aac_sequence_header();
        bool do_flv_demux( int action );
        bool get_flv_tag_header_info( FLV_TAG_HEADER_INFO & tag_header_info );
    private:
        const char *_flv_file;
        FILE *_fflv_handler;
        bool _find_h264_sequence_header;
        bool _find_aac_sequence_header;
        FLV_AAC_SEQUENCE_HEADER _flv_aac_sequence_header;
        AAC_ADTS_HEADER _aac_adts_header;
};

#endif
