/*
*@filename:czq_vector.h
*@author:chenzhengqiang
*@start date:2015/09/27 18:28:22
*@modified date:
*@desc: 
*/



#ifndef _CZQ_CZQ_VECTOR_H_
#define _CZQ_CZQ_VECTOR_H_
//write the function prototypes or the declaration of variables here
#include<memory>
template<typename T>
class czq_vector
{
    public:
        czq_vector():elements(0),first_free(0),end(0){}
        ~czq_vector(){}
        void push_back( const T &value );
        void reallocate();
        void reserve( size_t new_capacity );
        void resize( size_t new_size );
        void resize( size_t new_size, const T & value);
        const T & operator[]( const size_t index) const;
        T & operator[]( const size_t index); 
    private:
        T *elements;
        T *first_free;
        T *end;
        std::allocator<T> v_allocator;
};

#include"czq_vector.cpp"
#endif
