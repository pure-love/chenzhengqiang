/*
*@filename:great_algs.h
*@author:chenzhengqiang
*@start date:2015/10/06 08:52:52
*@modified date:
*@desc: 
*/



#ifndef _CZQ_GREAT_ALGS_H_
#define _CZQ_GREAT_ALGS_H_
//write the function prototypes or the declaration of variables here

//good examples of book
namespace czq
{
   namespace algorithm
   {
        unsigned long pow( int x, int y )
        {
            if( y == 0 )
            return 1;
            if( y == 1 )
            return x;
            if( y % 2 == 0 )
            return pow( x*x, y /2 );
            if( y % 2 != 0)
            return pow( x*x, y/2 ) *x;
        }
        
        int max_subsequence_sum( const int nums[], size_t n )
        {
            int max_sub_sum = 0;
            int sum = 0;
            for( size_t index = 0; index < n; ++index )
            {
                sum+=nums[index];
                if( sum > max_sub_sum )
                max_sub_sum = sum;
                else if( sum < 0 )
                sum = 0;
            }
        }

        int max_subsequence_sum( const int nums[], size_t left, size_t right )
        {
            if( left == right )
            {
                if( nums[left] < 0 )
                return 0;
                else
                return nums[left];
            }

            size_t mid = ( left+right ) /2;
            int max_left_sum = max_subsequence_sum( nums, left, mid );
            int max_right_sum = max_subsequence_sum( nums, mid+1, right );

            int sum=0;
            int max_left_border_sum = 0;
            int max_right_border_sum = 0;
            
            for( size_t index = mid; index >=left; --index )
            {
                sum+=nums[index];
                if( sum > max_left_border_sum )
                max_left_border_sum = sum;
            }

            sum = 0;
            for( size_t index = mid+1; index <=right; ++index )
            {
                sum+=nums[index];
                if( sum > max_right_border_sum )
                max_right_border_sum = sum;
            }

            sum = max_left_border_sum+max_right_border_sum;
            sum = ( sum > max_left_sum )?sum:max_left_sum;
            return ( sum > max_right_sum )?sum:max_right_sum;
        }


        int min_subsequence_sum( const int nums[], size_t n )
        {
            //2,2,3,4,5,-2,1
            //2,2,3,4,5,2,1
            int min_sub_sum = nums[0];
            int sum = 0;
            for( size_t index = 0; index < n; ++index )
            {
                sum+=nums[index];
                if( sum < min_sub_sum )
                min_sub_sum = sum;
                else if( sum > 0 )
                sum = 0;
            }
        }


        void rand_int( const int start, const int end )
        {
            int *int_buffer =new int[end-start+1];
            if( int_buffer == NULL )
            return;
            for( size_t index=0; index < ( end-start+1); ++index )
            {
                int_buffer[index] = index+1;
            }

            for( size_t index=1; index < (end-start+1); ++index )
            {
                
            }
        }
   }
};
#endif
