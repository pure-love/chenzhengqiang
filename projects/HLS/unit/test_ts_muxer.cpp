/*
@author:chenzhengqiang
@start date:2015/8/26
@modified date:
@desc:unit test for testing ts mux
*/

#include "ts_muxer.h"
#include <cstdio>
#include <cstdlib>

static const char * Usage="%s <h264 file> <aac file> <ts file>\n";

int main( int argc, char ** argv )
{
	if( argc != 4 )
      {
          printf(Usage,argv[0]);
          exit(EXIT_FAILURE);
      }
    
      ts_mux_for_h264_aac(argv[1],argv[2],argv[3]);
      return 0;
}