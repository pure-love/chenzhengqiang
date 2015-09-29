/*
*@filename:sorts.cpp
*@author:chenzhengqiang
*@start date:2015/09/26 16:36:13
*@modified date:
*@desc: 
*/

#include "sorts.h"
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
            //compare the value of end in this data buffer to the middle key
            //till it's less than the middle key
            while( buffer[end] > the_middle_key && ( begin < end ))
            --end;
	      //now place the value that less than the middle key to the left side(sort by asc)		
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


//the quick sort,optimized base on the bubble sort
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


//the insert sort
template<typename T>
void	sorts<T>::insert( T * buffer, int buffer_size, bool asc )
{
	for( int index=1; index < buffer_size; ++index )
	{
		int prev_pos = index-1;
		T cur_val = buffer[index];
		if( asc )
		{
			while(  prev_pos >= 0 && buffer[prev_pos] > cur_val )
			{
				buffer[prev_pos+1]=buffer[prev_pos];
				--prev_pos;
			}
		}
		else
		{
			while( prev_pos>=0 && buffer[prev_pos] < cur_val )
			{
				buffer[prev_pos+1]=buffer[prev_pos];
				--prev_pos;
			}
			
		}
		buffer[prev_pos+1]=cur_val;
	}
}


//the shell sort,optimized base on insert sort
template<typename T>
void	sorts<T>::shell(T * buffer, int buffer_size, bool asc )
{
	int delta = buffer_size /2;
	while( delta > 0 )
	{
		for( int unsorted_index=delta; unsorted_index < buffer_size; ++unsorted_index )
		{
			int sorted_last_index = unsorted_index-delta;
			T cur_unsorted_value = buffer[unsorted_index];
			if( asc )
			{
				while(  sorted_last_index >= 0 && buffer[sorted_last_index] > cur_unsorted_value )
				{
					buffer[sorted_last_index+delta]=buffer[sorted_last_index];
					sorted_last_index-=delta;
				}
			}
			else
			{
				while(  sorted_last_index >= 0 && buffer[sorted_last_index] < cur_unsorted_value )
				{
					buffer[sorted_last_index+delta]=buffer[sorted_last_index];
					sorted_last_index-=delta;
				}
			
			}
			buffer[sorted_last_index+delta]= cur_unsorted_value;
		}
		delta/=2;
	}
}



//the selection sort
template<typename T>
void	sorts<T>::select(T * buffer, int buffer_size, bool asc )
{
	T tmp;
	for( int o_index=0; o_index <buffer_size-1; ++o_index )
	{
		int min_max_index = o_index;
		for( int i_index = o_index+1; i_index < buffer_size; ++i_index )
		{
			if( asc )
			{
				if( buffer[i_index] < buffer[min_max_index] )
				min_max_index = i_index;
			}
			else
			{
				if( buffer[i_index] > buffer[min_max_index] )
				min_max_index = i_index;
			}
		}
		if( min_max_index == o_index )
		continue;
		tmp = buffer[o_index];
		buffer[o_index] = buffer[min_max_index];
		buffer[min_max_index]=tmp;
	}
}



//the heap sort
template<typename T>
void	sorts<T>::create_heap(T * buffer, int root_index, int end, bool asc )
{
    T root = buffer[root_index];
    int min_max_index = -1;
    for( int left_child_index = 2*root_index+ 1; left_child_index <=end; )
    {
        min_max_index = left_child_index;
        if( asc )
        {
            
            if( (left_child_index+1 <= end ) && ( buffer[left_child_index] < buffer[left_child_index+1] ) )
            {
                min_max_index = left_child_index+1;
            }

            if( root >= buffer[min_max_index])
            {
              break;
            }
            
        }
        else
        {
            if( (left_child_index+1 <= end ) && ( buffer[left_child_index] > buffer[left_child_index+1] ) )
            {
                min_max_index = left_child_index+1;
            }

            if( root <= buffer[min_max_index])
            {
                break;
            }
        }
            
        buffer[root_index] = buffer[min_max_index];
        root_index = min_max_index;
        left_child_index =2*root_index+1;
   }
   buffer[root_index]= root;
}



template<typename T>
void	sorts<T>::heap( T * buffer, int buffer_size, bool asc )
{
    for( int end=buffer_size/2; end>=0; --end )
    {
        create_heap( buffer, end, buffer_size-1, asc );
    }
   
    for(int end=buffer_size-1; end>0; --end )
    {
        
        T temp=buffer[end];
        buffer[end] = buffer[0];
        buffer[0] = temp;
        create_heap( buffer,0,end-1, asc );
    }

}


template<typename T>
void sorts<T>::merge_array( T * buffer, int begin, int mid, int end, T *tmp,bool asc )
{
    int left = begin;
    int right = mid+1;
    int tmp_index = 0;
    while( left <= mid && right <=end )
    {
        if( asc )
        {
            if( buffer[left] < buffer[right])
            {
                tmp[tmp_index++] = buffer[left];
                ++left;
            }
            else
            {
                tmp[tmp_index++] = buffer[right];
                ++right;
            }
        }
        else
        {
            
            if( buffer[left] > buffer[right])
            {
                tmp[tmp_index++] = buffer[left];
                ++left;
            }
            else
            {
                tmp[tmp_index++] = buffer[right];
                ++right;
            }
        }
    }

    while( left <= mid )
    {
        tmp[tmp_index++]=buffer[left++];
    }
    while( right <= end )
    {
        tmp[tmp_index++]=buffer[right++];
    }

    tmp_index = 0;
    while( begin <= end )
    {
        buffer[begin++]=tmp[tmp_index++];
    }
}


template<typename T>
void sorts<T>::merge_sort( T *buffer, int begin, int end, T *tmp,bool asc  )
{
    if( begin < end )
    {
        int mid = (begin+end) /2;
        merge_sort( buffer,begin,mid,tmp,asc );
        merge_sort( buffer,mid+1,end,tmp,asc );
        merge_array( buffer,begin,mid,end,tmp,asc );
    }
}

template<typename T>
void sorts<T>::merge( T *buffer, int buffer_size, bool asc  )
{
    if( buffer == NULL || buffer_size <=0 )
    return;    
    T * tmp = new T[buffer_size];
    merge_sort( buffer,0,buffer_size-1,tmp,asc );
    delete [] tmp;
}
