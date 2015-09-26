/*
*@filename:sorts.cpp
*@author:chenzhengqiang
*@start date:2015/09/26 16:36:13
*@modified date:
*@desc: 
*/

#include <cstring>

template<typename T>
void	sorts<T>::bubble( T * buffer, int buffer_size , bool asc )
{
	bool is_sorted = true;
	T tmp;
	for( int o_index=0; o_index < buffer_size-1; ++o_index )
	{
		for( int i_index=0; i_index < buffer_size-o_index-1; ++i_index )	
		{
			if( asc )
			{
				if( buffer[i_index] > buffer[i_index+1] )
				{
					tmp = buffer[i_index+1];
					buffer[i_index+1]=buffer[i_index];
					buffer[i_index]=tmp;
					is_sorted = false;
				}
			}
			else
			{
			       
				if( buffer[i_index] < buffer[i_index+1] )
				{
				       
					tmp = buffer[i_index+1];
					buffer[i_index+1]=buffer[i_index];
					buffer[i_index]=tmp;
					is_sorted = false;
				}
			}
		}
		if( is_sorted )
		break;
	}
}


void	sorts<const char *>::bubble( const char ** buffer, int buffer_size , bool asc )
{
	bool is_sorted = true;
	const char * tmp;
	for( int o_index=0; o_index < buffer_size-1; ++o_index )
	{
		for( int i_index=0; i_index < buffer_size-o_index-1; ++i_index )	
		{
			if( asc )
			{
				if( strcmp(buffer[i_index],buffer[i_index+1] ) > 0 )
				{
					tmp = buffer[i_index+1];
					buffer[i_index+1]=buffer[i_index];
					buffer[i_index]=tmp;
					is_sorted = false;
				}
			}
			else
			{
				if( strcmp(buffer[i_index],buffer[i_index+1] ) < 0 )
				{
					tmp = buffer[i_index+1];
					buffer[i_index+1]=buffer[i_index];
					buffer[i_index]=tmp;
					is_sorted = false;
				}
			}
		}
		if( is_sorted )
		break;
	}
}

