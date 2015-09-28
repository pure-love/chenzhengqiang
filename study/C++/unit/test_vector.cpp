/*
 * file name:test_vector.cpp
 * author:chenzhengqiang
 * start date:2015/9/28
 * modified date:
 * desc:
 */

#include "../include/czq_vector.h"
#include<iostream>
using namespace std;

int main( int argc, char ** argv )
{
	czq_vector<int> v_container;
	v_container.push_back(1);
	v_container.push_back(2);
	v_container.push_back(3);
	v_container.push_back(4);
	cout<<"v_container[0] is "<<v_container[0]<<endl;
	cout<<"v_container[1] is "<<v_container[1]<<endl;
	return 0;
}
