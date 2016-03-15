/*
*@filename:sha1.h
*@author:chenzhengqiang
*@start date:2016/03/15 13:51:03
*@modified date:
*@desc: 
*/



#ifndef _CZQ_SHA1_H_
#define _CZQ_SHA1_H_
//write the function prototypes or the declaration of variables here
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct SHA1Context{
	unsigned Message_Digest[5];      
	unsigned Length_Low;             
	unsigned Length_High;            
	unsigned char Message_Block[64]; 
	int Message_Block_Index;         
	int Computed;                    
	int Corrupted;                   
} SHA1Context;

void SHA1Reset(SHA1Context *);
int SHA1Result(SHA1Context *);
void SHA1Input( SHA1Context *,const char *,unsigned);
char * sha1_hash(const char *source);

#endif
