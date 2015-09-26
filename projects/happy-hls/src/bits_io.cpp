/*
@author:chenzhengqiang
@file name:bits_operate.h
@version:1.0
@start date:
@modified date:
@desc:providing the apis of bits operate
*/

#include "bits_io.h"
#include <errno.h>
#include <cstring>
extern int errno;
bool bits_io::read_8_bit( int & bit8, FILE * fs )
{
    if( fread(&bit8,1,1,fs )!=1)
    {  
        return false;
    }
    return true;
}


bool bits_io::read_16_bit( int & bit16, FILE * fs )
{
    if(fread(&bit16,2,1,fs )!=1)
    return false;
    return true;
}


bool bits_io::read_24_bit( int & bit24, FILE * fs )
{
     if(fread(&bit24,3,1,fs )!=1)
     return false;
     return true;
}


bool bits_io::read_32_bit( int & bit32, FILE * fs )
{
    if(fread(&bit32,4,1,fs )!=1)
    return false;
    return true;
}


