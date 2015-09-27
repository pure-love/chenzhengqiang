#include<stdio.h>

int array_sum( int *array, int array_size )
{
	return (array_size<=1) ? array[0]:array[array_size-1]+array_sum(array,array_size-1);
}
int main( int argc, char ** argv )
{
	int array[5]={1,2,3,4,5};
	int sum=0;
	sum=array_sum( array,5);
	printf("sum is %d\n",sum);
	return 0;
}
