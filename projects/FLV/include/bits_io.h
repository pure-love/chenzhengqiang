/*
@author:chenzhengqiang
@file name:bits_operate.h
@version:1.0
@start date:
@modified date:
@desc:providing the apis of bits operate
*/

#ifndef _CZQ_BITS_IO_H_
#define _CZQ_BITS_IO_H_
#include<cstdio>
#include<stdint.h>

#define hton8(x)    ((x>>8)&0xff)
#define hton16(x)  (((x>>8)&0xff)|((x<<8)&0xff00))
#define hton24(x)  (((x>>16)&0xff)|((x<<16)&0xff0000)|(x&0xff00))
#define hton32(x)  (((x>>24)&0xff)|((x>>8) &0xff00)|\
((x<<8)&0xff0000)|((x<<24)&0xff000000))


#define STR(x) (x.c_str())
#define FCUR(x) (ftell(x))
#define FSEEK(x,f) (fseek(f,x,SEEK_CUR))
#define FSET(x,f) (fseek(f,x,SEEK_SET))


class bits_io
{
    public:
        static bool read_8_bit( int & bit8, FILE * fs );
        static bool read_8_bit(int & bit8, uint8_t *stream);
        static bool read_16_bit( int & bit16, FILE * fs );
        static bool read_16_bit( int & bit16, uint8_t * stream );
        static bool read_24_bit( int & bit24, FILE * fs );
        static bool read_24_bit( int & bit24, uint8_t * stream );
        static bool read_32_bit( int & bit32, FILE * fs );
        static bool read_32_bit( int & bit32, uint8_t * stream);
};
#endif
