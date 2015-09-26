/*
*@filename:sorts.h
*@author:chenzhengqiang
*@start date:2015/09/26 16:36:13
*@modified date:
*@desc: 
*/



#ifndef _CZQ_SORTS_H_
#define _CZQ_SORTS_H_
//write the function prototypes or the declaration of variables here


template<typename T>
class sorts
{
	public:
		static void bubble( T *buffer, int buffer_size, bool asc = true );
};


template<>
class sorts<const char *>
{
	public:
		static void bubble( const char * *buffer, int buffer_size, bool asc = true );
};
#include"sorts.cpp"
#endif
