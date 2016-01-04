/*
*@filename:perm.cpp
*@author:chenzhengqiang
*@start date:2015/10/31 10:07:50
*@modified date:
*@desc: 
*/



#include<iostream>
using namespace std;

void swap( int &val1, int &val2 )
{
	if( val1 == val2 )
		return;
	val1=val1^val2;
	val2=val1^val2;
	val1=val1^val2;
}

void perm( int * data, int start, int end )
{
	if( start >= end )
	{
		for( int index=0; index <= end; ++index )
		{
			cout<<data[index]<<" ";
		}
		cout<<endl;
	}
	else
	{
		for( int index = start; index <= end; ++index )
		{
			swap( data[index], data[start] );
			perm( data, start+1, end );
			swap( data[index], data[start] );
		}
	}
}


int main( int argc, char ** argv )
{
	int data[]={ 1,2,3 };
	perm( data, 0, 2 );
	return 0;
}
