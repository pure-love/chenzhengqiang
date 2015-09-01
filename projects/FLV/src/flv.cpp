/*
@file name:flv.h
@author:chenzhengqiang
@version:1.0
@start date:2015/8/27
@modified date:
@desc:providing the basic api related to flv
*/


#include "bits_io.h"
#include "flv.h"
#include<cstdio>
#include<cstdlib>
#include<iostream>
#include<arpa/inet.h>
using std::cout;
using std::endl;

flv::flv():_flv_signature(0),_version(0),_type_flags(0),_flv_header_length(0),
                                     _audio_are_present(false),_video_are_present(false),
                                     _format_invalid(true)
{
    ;
}

flv::~flv()
{
    ;
}

bool flv::verify_format_ok( const char *flv_file )
{
    FILE * fflv_handler = fopen( flv_file, "r");
    return do_flv_verify( fflv_handler );
}


bool flv::do_flv_verify( FILE * fflv_handler )
{
    if( fflv_handler == NULL )
    return false;
    
    //read the flv's signature
    if(!bits_io::read_24_bit( _flv_signature,fflv_handler) )
    {
       return false;
    }

    _flv_signature = hton24(_flv_signature);
    int FIXED_FLV_SIGNATURE='FLV';
    if ( _flv_signature != FIXED_FLV_SIGNATURE  )
    {
       return false;
    }

    if( ! bits_io::read_8_bit( _version, fflv_handler ))
    {
       return false;
    }
    
    if( !bits_io::read_8_bit( _type_flags, fflv_handler ))
    {
       return false;
    }

    if( ( _type_flags & 0x04 ) == 4 )
    _audio_are_present = true;
    
    if( ( _type_flags & 0x01 ) == 1 )
    _video_are_present = true;
    
    if (!bits_io::read_32_bit(_flv_header_length,fflv_handler ))
    {
       return false;
    }
    
    _flv_header_length = hton32( _flv_header_length );
    _format_invalid = false;
    fclose( fflv_handler );
    return true;
}

void flv::print_flv_header() const
{
    if( _format_invalid )
    return;
    cout<<"signature:flv"<<endl;
    cout<<"version:"<<_version<<endl;
    cout<<"audio are present:"<<(_audio_are_present?"yes":"no")<<endl;
    cout<<"video are present:"<<(_video_are_present?"yes":"no")<<endl;
    cout<<"header length:"<<_flv_header_length<<endl;
}
