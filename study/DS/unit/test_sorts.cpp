/*
 * @filename:test_sorts.cpp
 * @start date:2015/9/26
 * @modified date:
 * @desc:unit test for testing kinds of sort algorithm
 * */
#include "/share/chenzhengqiang/utils/utils.h"
#include "../include/sorts.h"
#include <iostream>
using namespace std;

int main( int argc, char ** argv )
{
	(void)argc;
	(void)argv;
	int data[6]={2,1,3,4,5,6};
	const char *strs[3]={"hello","what","hehe"};
	cout<<endl<<"unsorted numbers:";
	print_array( data );
	cout<<endl<<"sorted numbers using bubble sort by desc:";
	sorts<int>::bubble( data,6,false );
	print_array( data );
	cout<<endl<<"sorted numbers using quick sort by asc:";
	sorts<int>::quick( data,0,5 );
	print_array( data );
	cout<<endl<<"sorted numbers using insert sort by desc:";
	sorts<int>::insert( data,6,false );
	print_array( data );
	cout<<endl<<"sorted numbers using shell sort by asc:";
	sorts<int>::shell( data,6 );
	print_array( data );
	cout<<endl<<"sorted numbers using select  sort by desc:";
	sorts<int>::select( data,6,false);
	print_array( data );

	cout<<endl<<endl<<endl<<"unsorted strings:";
	print_array( strs );
	cout<<endl<<"sorted strings sort by asc:";
	sorts<const char *>::bubble(strs,3);
	print_array( strs );
	cout<<endl<<endl;
	return 0;
}

