/*
*@filename:czq_vector.cpp
*@author:chenzhengqiang
*@start date:2015/09/27 18:28:22
*@modified date:
*@desc: 
*/



#include "czq_vector.h"
#include<cstddef>
#include<algorithm>
template<typename T>
void czq_vector<T>::push_back( const T & value )
{
    if( first_free == end )
    {
        reallocate();
    }
    v_allocator.construct( first_free, value );
    ++first_free;
}


template<typename T>
void czq_vector<T>::reallocate()
{
    std::ptrdiff_t old_size = end-elements;
    size_t new_capacity = old_size > 1 ? 2*old_size:2;
    T * new_memory = v_allocator.allocate(new_capacity);
    uninitialized_copy( elements, first_free, new_memory);
    T *p = first_free;
    while( p != elements )
    {
        v_allocator.destroy(--p);
    }

    if( elements )
    {
        v_allocator.deallocate( elements, end - elements );
    }
    elements = new_memory;
    first_free = elements+old_size;
    end = elements+new_capacity;
}


template<typename T>
void czq_vector<T>::reserve( size_t new_capacity )
{
    if( new_capacity < ( end - elements) )
    return;

    size_t old_size = first_free-elements;
    T *new_elements = v_allocator.allocate( new_capacity );
    
    uninitialized_copy( elements,first_free,new_elements );
    T *p = first_free;
    while( p != elements )
    {
        v_allocator.destroy(--p);    
    }

    if( elements )
    v_allocator.deallocate( elements, end-elements );

    elements = new_elements;
    first_free = elements + old_size;
    end = elements+new_capacity;
}



template<typename T>
void czq_vector<T>::resize( size_t new_size )
{
    T tmp=T();
    
}


template<typename T>
void czq_vector<T>::resize( size_t new_size, const T & value )
{
   T tmp = T();
}


template<typename T>
const T & czq_vector<T>::operator[]( const size_t index ) const
{
    //don't do the error check
    return elements[index];
}


template<typename T>
T & czq_vector<T>::operator[]( const size_t index )
{
    //don't do the error check
    return elements[index];
}

