/*
*@author:chenzhengqiang
*@start date:2015/7/9
*@modified date:
*/
#ifndef _BACKER_SIGNALLING_H_
#define _BACKER_SIGNALLING_H_
#include<sys/types.h>

static const int SIGNALLING_LENGTH = 64; 
static const int CHANNEL_LENGTH = 24;
static const int CAMERA = 0;
static const int PC = 1;
static const int CREATE=0;
static const int JOIN=0;
static const int CANCEL = 1;
static const int LEAVE = 0;

bool generate_signalling(char *signalling_buf,size_t buf_size,
	                                       const char *channel,int flags, int action);
#endif
