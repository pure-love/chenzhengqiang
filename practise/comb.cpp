/*
*@filename:comb.cpp
*@author:chenzhengqiang
*@start date:2015/10/31 10:49:02
*@modified date:
*@desc: 
*/



#include<iostream>
using namespace std;

void print_chars( char *chars, int *flags int size )
{
	cout<<"{";
	for( int index = 0; index <= size; ++index )
	{
		if( flags[index]  )
		{
			cout<<chars[index]<<" ";
		}
	}
	cout<<"}"<<endl;
}

void comb( char *chars, int *flags, int start, int end )
{
	if( start == end+1 )
	{
		print_chars( chars, flags, end );
	}
	else
	{
		flags[start] = 0;
		comb( chars, flags, start+1, end );
		flags[start] = 1;
		comb( chars, flags, start+1, end );
	}
}


int main( int argc, char ** argv )
{
	int flags[]={0,0,0};
	char chars[]={'a','b','c'};
	comb( chars, flags, 0, 2 );
	return 0;
}
