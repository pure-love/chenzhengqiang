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

//template class sorts for kinds of sort algorithm
template<typename T>
class sorts
{
	public:
		static void bubble( T *buffer, int buffer_size, bool asc = true );
		static int obtain_middle_key_pos( T * buffer, int begin , int end, bool asc );
		static void quick( T *buffer, int begin, int end, bool asc = true );
		static void insert( T *buffer, int buffer_size, bool asc = true );
		static void shell( T *buffer, int buffer_size, bool asc = true );
		static void selection( T *buffer, int buffer_size, bool asc = true );
		static void heap( T *buffer, int buffer_size, bool asc = true );
};

//particular type for const char *
template<>
class sorts<const char *>
{
	public:
		static void bubble( const char * *buffer, int buffer_size, bool asc = true );
};
#include"sorts.cpp"
#endif
