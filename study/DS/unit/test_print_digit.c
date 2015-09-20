/*
 * @file name:test_print_digit.c
 * @author:chenzhengqiang
 * @start date:2015/9/20
 * modified date:
 * desc:unit test for testing print digit by using the recursive method
 * */

#include <stdio.h>
void print_digit( int number );
int main( int argc, char ** argv )
{
	int number;
	printf( "Please Enter A Number:" );
	scanf( "%d",&number );
	return 0;
}

void print_digit( int number )
{
	if( number < 10 || number > -10 )
		printf( "%d",number );
	number %= 10;
	print_digit( number );
	number /=10;
}
