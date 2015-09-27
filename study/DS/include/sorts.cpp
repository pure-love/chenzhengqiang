/*
*@filename:sorts.cpp
*@author:chenzhengqiang
*@start date:2015/09/26 16:36:13
*@modified date:
*@desc: 
*/

#include <cstring>

//the bubble sort,there is nothing to tell,it's so easy
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


//the bubble sort particular for const char * type
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


/*
*@returns:-1 indicates error,otherwise ok
*@desc:obtain the middle pos after sort by serveral times,then the values of one side are 
    all less than the middle key,and greater the other side
*/
template<typename T>
int sorts<T>::obtain_middle_key_pos( T * buffer, int begin , int end, bool asc )
{
    if( (begin >= end) || begin < 0 || end < 0 )
    return -1;  
    
    T the_middle_key = buffer[begin];
    //sort by serveral times
    while( begin < end )
    {
        if( asc )
        {
            while( buffer[end] > the_middle_key && ( begin < end ))
            --end;
            buffer[begin] = buffer[end];
            while( buffer[begin] < the_middle_key && ( begin < end ))
            ++begin;
            buffer[end] = buffer[begin];
        }
        else
        {
            while( buffer[end] < the_middle_key && ( begin < end ))
            --end;
            buffer[begin] = buffer[end];
            while( buffer[begin] > the_middle_key && ( begin < end ))
            ++begin;
            buffer[end] = buffer[begin];
        }
    }
    //and now one side is less than the middle_key and greater the other side
    buffer[begin] = the_middle_key;
    //return the middle key's pos for next sort
    return begin;
}


//the quick sort
template<typename T>
void	sorts<T>::quick( T * buffer, int begin , int end, bool asc )
{
	if( begin < end )
       {
            int middle_pos =sorts<T>::obtain_middle_key_pos( buffer, begin, end, asc );
            if( middle_pos == -1 )
            return;    
            quick( buffer, begin,middle_pos,asc );
            quick( buffer, middle_pos+1, end, asc );
       }
}


//the quick sort
template<typename T>
void	sorts<T>::insert( T * buffer, int buffer_size, bool asc )
{
	for( int index=)
}

