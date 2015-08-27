/*
@file name:test_flv.cpp
@author:chenzhengqiang
@start date:2015/8/27
@modified date:
@desc:unit test for testing flv
*/

#include "flv.h"
#include<cstdio>
#include<cstdlib>

using namespace std;

static const char *usage="usage:%s <flv file>\n";

int main( int argc, char ** argv )
{
    if( argc != 2)
    {
        printf(usage,argv[0]);
        exit(EXIT_FAILURE);
    }

    flv flv_test;
    if( flv_test.verify_format_ok(argv[1]))
    {
        flv_test.print_flv_header();
    }
    return 0;
}
