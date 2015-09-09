/*
@author:chenzhengqiang
@start date:2015/9/9
@modified date:
@desc unit test for testing flv demux:h264,aac included
*/

#include "flv_demux.h"
#include <cstdio>
#include <cstdlib>

static const char * usage="usage:%s <flv file> <h264 file> <aac file>\n";
static const int CMD_ARGS=4;

int main( int argc, char ** argv )
{
       if( argc != CMD_ARGS )
       {
           printf(usage,argv[0]);
           exit(EXIT_FAILURE);
       }
       flv_demux_2_h264_aac( argv[1], argv[2], argv[3] );
	return 0;
}