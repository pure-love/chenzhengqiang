/*
*@filename:operator_new_delete.h
*@author:chenzhengqiang
*@start date:2015/09/28 16:38:27
*@modified date:
*@desc: 
*/



#ifndef _CZQ_OPERATOR_NEW_DELETE_H_
#define _CZQ_OPERATOR_NEW_DELETE_H_
//write the function prototypes or the declaration of variables here
#include<memory>
template<typename T>
class cached_obj
{
    public:
        void * operator new( size_t );
        void operator delete( void *p, size_t );
        ~ cached_obj(){}
    protected:
        T *next;
    private:
        static const size_t mem_chunk = 24;
        static T *free_store;
        static std::allocator<T> mem_allocator;
        void add_to_freelist( T *);
};
#include"operator_new_delete.cpp"
#endif
