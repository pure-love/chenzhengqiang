/*
@file name:test_flv.cpp
@author:chenzhengqiang
@start date:2015/8/27
@modified date:
@desc:unit test for testing flv
*/

#include "flv_demux.h"
#include<cstdio>
#include<cstdlib>

using namespace std;

static const char *usage="usage:%s <flv file> <aacfile>\n";

int main( int argc, char ** argv )
{
    if( argc != 3 )
    {
        printf(usage,argv[0]);
        exit(EXIT_FAILURE);
    }

    flv_demux test_flv_demux(argv[1],argv[2]);
    test_flv_demux.demux_for_aac();
    return 0;
}
