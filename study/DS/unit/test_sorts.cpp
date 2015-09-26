/*
 * @filename:test_sorts.cpp
 * @start date:2015/9/26
 * @modified date:
 * @desc:unit test for testing kinds of sort algorithm
 * */
#include "../include/sorts.h"
#include <iostream>
using namespace std;

int main( int argc, char ** argv )
{
	(void)argc;
	(void)argv;
	int data[6]={2,1,3,4,5,6};
	const char *strs[3]={"hello","what","hehe"};
	cout<<"unsorted numbers:";
	for( int index = 0; index < 6; ++index )
	{
		cout<<data[index]<<" ";	
	}
	cout<<endl<<"sorted numbers sort by desc:";
	sorts<int>::bubble(data,6,false);
	for( int index = 0; index < 6; ++index )
	{
		cout<<data[index]<<" ";	
	}
	cout<<endl<<"unsorted strings:";
	
	for( int index = 0; index < 3; ++index )
	{
		cout<<strs[index]<<" ";
	}
	cout<<endl<<"sorted strings sort by asc:";
	sorts<const char *>::bubble(strs,3);
	for( int index = 0; index < 3; ++index )
	{
		cout<<strs[index]<<" ";
	}
	cout<<endl;
	return 0;
}

