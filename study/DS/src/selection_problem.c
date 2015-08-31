/*
 *@author:chenzhengqiang
 *@file name:with_all_sort_select_k_max.c
 *@version:1.0
 *@start date:2015/8/29
 *@modified date:
 *@desc:scores of function of selection problem
 */


#include "selection_problem.h"
#include <stdlib.h>
#include <string.h>



/*
 * returns:the k-max value,-1 indicates error
 * desc:sort the array with bubble sort order by desc,
   then just return the data[k-1] as the k-max value
 */

int with_all_sort_select_k_max( int *data,int size, int k )
{
    int i,j,tmp;
    int is_sorted = 1;    
    if( data == NULL || size < k )
	    return -1;
    //firstly,sort the data buffer with bubble sort
    //order by desc
    for( i=0; i < size-1; ++i )
    {
    	for( j=0; j < size-i-1; ++j )
    	{	
         	if( data[j] < data[j+1] )
	 	{
	     		tmp = data[j+1];
	     		data[j+1]=data[j];
	     		data[j] = tmp;
	     		is_sorted = 0;
	 	}
	}
	if( is_sorted )
	break;
    }
    //then just return the data[k-1]
    return data[k-1];
}


/*
 *returns:-1 indicates error,otherwise return the k largest value
 *desc:allocate k size's buffer to save values of k in source buffer sorted by desc,
   then take the next value int source buffer out one by one to campare with the k largets value
   in the sorted buffer,if larger then take replace of the smallest one,else ignore it
 */
int with_k_sort_select_k_max( int * data_buffer, int size, int k )
{
	int * k_buffer = NULL;
	int first = 1;
	int index,i,j;
	int tmp,k_max;
	int is_sorted = 1;
	if( data_buffer == NULL || size < k )
		return -1;
	if( k == size )
		return with_all_sort_select_k_max( data_buffer, size, k);
	k_buffer = ( int * ) malloc( k* sizeof(int));
	if( k_buffer == NULL )
		return -1;
	
	for( index = 0; index < size; ++index )
	{
		if( index < k )
		{	
			k_buffer[index] = data_buffer[index];
			continue;
		}
		if( first )
		{
			first = 0;
			//sort the k_buffer's value with bubble sort order by asc
			for( i=0; i < k-1; ++i )	
			{
				for( j=0; j< k-1-i; ++j )
				{
					if( k_buffer[j] > k_buffer[j+1])
					{
						tmp = k_buffer[j];
						k_buffer[j] = k_buffer[j+1];
						k_buffer[j+1] = tmp;
						is_sorted = 0;
					}
					if( is_sorted )
					break;
				}
			}
		}
		if( data_buffer[index] <= k_buffer[0] )
		{
			;//just ignore it
		}
		else
		{
			;//replace the smallest one in k_buffer
			for( i=1; i < k; ++i )
			{
				if( data_buffer[index] < k_buffer[i])
					break;
			}
			k_buffer[0]=k_buffer[i-1];
			k_buffer[i-1] = data_buffer[index];	
		}
	}

	k_max = k_buffer[0];
	free( k_buffer );
	k_buffer = NULL;
	return k_max;
}
