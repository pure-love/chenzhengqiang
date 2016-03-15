/*
*@filename:base64.h
*@author:chenzhengqiang
*@start date:2016/03/15 13:51:09
*@modified date:
*@desc: 
*/



#ifndef _CZQ_BASE64_H_
#define _CZQ_BASE64_H_
//write the function prototypes or the declaration of variables here
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

char* base64_encode(const char* data, int data_len); 
char *base64_decode(const char* data, int data_len); 
static char find_pos(char ch); 
#endif
