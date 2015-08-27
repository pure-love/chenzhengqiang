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


//the adts
struct AAC_ADTS_HEADER
{
	unsigned	char check1;
	
	unsigned  char protection:1;//误码校验1
	unsigned  char layer:2;//哪个播放器被使用0x00
	unsigned	char ver:1;//版本 0 for MPEG-4, 1 for MPEG-2
	unsigned	char check2:4;
	
	unsigned	char channel1:1;
	unsigned    char privatestream:1;//0
	unsigned	char SamplingIndex:4;//采样率
	unsigned	char ObjectType:2;
	
	unsigned	char length1:2;
	unsigned	char copyrightstart:1;//0
	unsigned    char copyrightstream:1;//0
	unsigned    char home:1;//0
	unsigned    char originality:1;//0
	unsigned	char channel2:2;
	unsigned	char length2;
	unsigned	char check3:5;
	unsigned	char length3:3;
	
	unsigned	char frames:2;//超过一块写
	unsigned	char check4:6;
};


//class flv_demux inherited from base class flv
//for the common interface's sake
//varify the flv format and so on
class flv_demux:public flv
{
    public:
        flv_demux( const char * flv_file, const char * aac_file );
        ~flv_demux();
        void demux_for_aac();
        void demux_for_h264();
        void demux_for_h264_aac();
        void get_aac_adts_frame( uint8_t *flv_aac_packet,uint8_t *aac_adts_frame, const int & data_size);
    private:
        void get_flv_aac_sequence_header( const int & data_size );
        void goto_h264_demux( const int & data_size );
    private:
        const char *_flv_file;
        FILE *_fflv_handler;
        FILE *_faac_handler;
        bool _find_aac_sequence_header;
        FLV_AAC_SEQUENCE_HEADER _flv_aac_sequence_header;
        AAC_ADTS_HEADER _aac_adts_header;
};

#endif
