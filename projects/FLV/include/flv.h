/*
@file name:flv.h
@author:chenzhengqiang
@version:1.0
@start date:2015/8/27
@modified date:
@desc:providing the basic api related to flv
*/

#ifndef _CZQ_FLV_H_
#define _CZQ_FLV_H_

#include<cstdio>
#include<stdint.h>


struct AAC_SEQUENCE_HEADER
{
     unsigned char sample_index1:3;
     unsigned char object_type:5;
     unsigned char other:3;//reserved
     unsigned char channel:4;
     unsigned char sample_index2:1;
};


//the basic class flv,providing the basic operatios of flv
class flv
{
    public:
        flv();
        ~flv();
        bool verify_format_ok( const char *flv_file );
        bool verify_format_ok( uint8_t * stream );
        void print_flv_header()const;
    public:        
        enum{TAG_AAC = 8,TAG_H264 = 9};
    protected:
        int _flv_signature;
        int _version;
        int _type_flags;
        int _flv_header_length;
        bool _audio_are_present;
        bool _video_are_present;
    private:
        bool _format_invalid;    
    private:
        bool do_flv_verify( FILE * fflv_handler );
};
#endif
