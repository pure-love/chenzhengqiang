/*
 * author:chenzhengqiang
 * file name:test_selection_problem.c
 * version:1.0
 * start date:2015/8/29
 * modified date:
 * desc:unit test for testing selection problem
  ignore the error checka
 */

#include "selection_problem.h"
#include <stdio.h>
#include <stdlib.h>

static const char *usage="usage:%s <1 2...numbers>\n";
int main( int argc, char ** argv )
{
	int index, k_max_index,k_max;
	int *data_buffer;
	if( argc < 3 )
	{
		printf( usage,argv[0]);
		exit( EXIT_FAILURE );
	}
	printf("enter the index of k max(start at 1):");
	scanf("%d",&k_max_index);
	
	data_buffer = ( int *)malloc((argc-1) * sizeof(int));
	printf("input:");
	for( index=1; index < argc-1; ++index )
	{
		printf("%s ",argv[index]);
		data_buffer[index-1]=atoi(argv[index]);
	}
	printf("\n");
	k_max = with_all_sort_select_k( data_buffer,argc-1,k_max_index );
	printf("the %d max value is %d",k_max_index, k_max);
	free( data_buffer);
	return 0;
}
