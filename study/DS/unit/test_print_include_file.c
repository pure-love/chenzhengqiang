/*
 * @file name:test_print_include_file.c
 * @author:chenzhengqiang
 * @start date:2015/9/20
 * @modified date:
 * @desc:unit test for print the included file using recursive method
 */


#include<stdio.h>

void print_included_file( const char *file_name );
int has_included_file( const char *file_name, char *new_file);

int main( int argc, char ** argv )
{
	print_included_file( "test.h" );
	return 0;
}


void print_included_file( const char * file_name )
{
	char new_file[1024];
	char line[1024];
	
	//include"123.h"
	//include"456.h"
	if( has_included_file( file_name , new_file ) )
	print_included_file( new_file );
	else
	{
		FILE *fp = fopen( file_name, "r");
		char ptr[1024];	
		while( fgets( ptr, sizeof(ptr),fp) != NULL )
		{
			printf("%s",ptr);
		}
	}
	
}



int has_included_file( const char *file_name, char *new_file )
{
	int index;
	//assume that it's "#include"XX.h""	
	index = 0;
	char *ptr;
	ptr = (char *)line;
	FILE *fp = fopen( file_name );
	char line[1024];
	fgets( line, sizeof(line), fp );

	while( *ptr && *ptr == ' ' )
	++ptr;

	if( strncmp( ptr, "#include" ,8 ) != 0 )
	return 0;

	ptr = ptr+8;
	while( *ptr && *ptr != '"' )
	++ptr;

	if( *ptr == '"' )
	{
		++ptr;
		while( *ptr && *ptr !='"' )
		{
			new_file[index++]=*ptr;
			++ptr;					
		}
		new_file[index]='\0';
		return 1;
	}
	
	return 0;
}
