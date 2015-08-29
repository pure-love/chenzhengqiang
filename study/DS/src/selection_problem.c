/*
 *@author:chenzhengqiang
 *@file name:with_all_sort_select_k_max.c
 *@version:1.0
 *@start date:2015/8/29
 *@modified date:
 *@desc:scores of function of selection problem
 */


/*
 * returns:the k-max value,-1 indicates error
 * desc:sort the array with bubble sort order by desc,
   then just return the data[k-1] as the k-max value
 */

#include "selection_problem.h"
#include <stdlib.h>

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
