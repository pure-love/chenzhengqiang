/*
*@filename:utils.cpp
*@author:chenzhengqiang
*@start date:2015/09/27 09:07:21
*@modified date:
*@desc: 
*/

#include "utils.h"
#include<iostream>
using std::cout;
using std::endl;
template<typename T,int N>
void print_array( T(&BUFFER)[N] )
{
    for( size_t index=0; index<N; ++index )
    {
        cout<<BUFFER[index]<<" ";
    }
    cout<<endl;
}
