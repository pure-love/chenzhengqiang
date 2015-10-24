/*
*@filename:bstree.h
*@author:chenzhengqiang
*@start date:2015/10/01 09:05:34
*@modified date:
*@desc: 
*/



#ifndef _CZQ_BSTREE_H_
#define _CZQ_BSTREE_H_
//write the function prototypes or the declaration of variables here
#include<stdexcept>
#include<iostream>
namespace czq
{
	template<typename T>
	//suppose that the binary search tree's left tree node's value is larger than the right one
	//also the template's "T" type must provide the "<" operand
	class bstree
	{
		public:		
			typedef struct bt_node
			{
				T value;
				bt_node *left;
				bt_node *right;
			}*BSTREE;
			
		public:
			static BSTREE get_me() { return _bstree; }
			static void insert( const T & value ) throw( std::bad_alloc );
			static BSTREE insert( const T & value, BSTREE bstree ) throw( std::bad_alloc );
			static BSTREE min();
			static BSTREE min( BSTREE bstree );
			static BSTREE max();
			static BSTREE max( BSTREE bstree );
			static BSTREE find( const T & value );
			static BSTREE find( const T & value , BSTREE bstree );
			static BSTREE erase( const T & value, BSTREE bstree );
			static void erase( const T & value );
			static void clear();
			static void clear( BSTREE bstree );
			static void walk_bstree();
		private:
			static BSTREE _bstree;
			bstree(){}
			bstree( const bstree & ){}
			bstree & operator=( const bstree & ){}
			~bstree(){}
	};

	template<>
	void bstree<int>::walk_bstree()
	{
		BSTREE bstree = _bstree;
		BSTREE prev = _bstree;
		while( bstree != 0 )
		{
			std::cout<<bstree->value<<" ";
			bstree = bstree->left;
		}
	}


	template<typename T>
	//do not forget the "typename" for the template's "type"
	typename bstree<T>::BSTREE bstree<T>::_bstree = 0;

	template<typename T>
	void bstree<T>::insert( const T & value ) throw( std::bad_alloc )	
	{
		bt_node * node = new bt_node;
		if( node == 0 )	
		throw std::bad_alloc( "allocate memory failed when execute new bt_node" );
		node->left =0;
		node->right = 0;
		node->value = value;
		bt_node *next = _bstree;
		while( next != 0 )
		{
			if( !( value < next->value ) )
			{
				next = next->right;
			}
			else
			{
				next = next->left;
			}
		}
		next = node;
	}


	template<typename T>
	//do not forget the "typename" for c++ template's "type"
	typename bstree<T>::BSTREE bstree<T>::insert( const T & value, BSTREE bstree ) throw( std::bad_alloc )
	{
		if( bstree == 0 )
		{
			bt_node * node = new bt_node;
			if( node == 0 )	
			throw std::bad_alloc( "allocate memory failed when execute new bt_node" );
			bstree = node;
		}
		else
		{
			if( bstree->value < value )
			{
				bstree->right = insert( value,bstree->right );
			}
			else
			{
				bstree->left = insert( value,bstree->left );
			}
		}
		return bstree;		
	} 


	//repeat again,suppose that the binaray search tree's left child's value is smaller than the right one
	template<typename T>
	typename bstree<T>::BSTREE bstree<T>::min()
	{
		BSTREE bstree_walker = _bstree;
		BSTREE prev_walker=bstree_walker;
		while( bstree_walker != 0 )
		{
			prev_walker = bstree_walker;
			bstree_walker = bstree_walker->left;
		}
		return prev_walker;
	}


       
       template<typename T>
       typename bstree<T>::BSTREE bstree<T>::min( BSTREE bstree )
       {
            if( bstree == 0 || bstree->left == 0 )
            return bstree;
            return min( bstree->left );
       }
           
	template<typename T>
	typename bstree<T>::BSTREE bstree<T>::max()
	{
		BSTREE bstree_walker = _bstree;
		BSTREE prev_walker = bstree_walker;
		while( bstree_walker != 0 )
		{
			prev_walker = bstree_walker;
			bstree_walker = bstree_walker->right;
		}
		return prev_walker;
	}


       template<typename T>
	typename bstree<T>::BSTREE bstree<T>::max( BSTREE bstree )
	{
		if( bstree == 0 || bstree->right == 0 )
		return bstree;
		return max( bstree->right );
	}
	
	template<typename T>
	typename bstree<T>::BSTREE bstree<T>::find( const T & value )
	{
		BSTREE bstree_walker = _bstree;
		while( bstree_walker )
		{
			if( !( bstree_walker->value < value ) && !( value < bstree_walker->value ) )
			break;
			
			if( bstree_walker->value < value )
			bstree_walker = bstree_walker->right;
			else
			bstree_walker = bstree_walker->left;
		}
		return bstree_walker;
	}


	template<typename T>
	typename bstree<T>::BSTREE bstree<T>::find( const T & value, BSTREE bstree )
	{
		if( ( bstree == 0 ) ||  !( bstree->value < value ) && !( value < bstree->value ) )
		return bstree;

		if( bstree->value < value )
		return find( value, bstree->right );
		else
		return find( value, bstree->left );	
	}


	//you can not avoid the "memory leak" once the exception happen
	//unless using the smart pointer
	//it's so simple to delete this binary search tree using the recursive way
	template<typename T>
	void bstree<T>::clear( BSTREE bstree )
	{
		if( bstree == 0 )
		return;
		clear( bstree->left );
		clear( bstree->right );
		delete bstree;
	}
	

	template<typename T>
	typename bstree<T>::BSTREE  bstree<T>::erase( const T & value, BSTREE bstree )
	{
	    if( bstree != 0 )
	    {
	        if( bstree->value < value )
	        {
	            bstree->right = erase( value, bstree->right );
	        }
	        else if( value < bstree->value )
	        {
	            bstree->left = erase( value, bstree->left );
	        }
	        else if( bstree->left != 0 && bstree->right != 0 )
	        {
	            BSTREE min_rchild = min( bstree->right );
	            bstree->value = min_rchild->value;
	            bstree->right = erase( bstree->value, bstree->right );
	        }
	        else
	        {
	            BSTREE bstree_to_delete = bstree;
	            if( bstree->left == 0 )
	            bstree = bstree->right;
	            else
	            bstree = bstree->left;
	            delete bstree_to_delete;
	        }
	    }
	    return bstree;


	template<typename T>
	void  bstree<T>::erase( const T & value )
	{  
            BSTREE bstree_walker= _bstree;  
            BSTREE parent = 0 ; 
            while( bstree_walker != 0 )  
            {  
                if( value < bstree_walker->value ) 
                {  
                    parent= bstree_walker; 
                    bstree_walker=bstree_walker->left; 
                }   
                else if( bstree_walker->value < value )  
                {  
                    parent= bstree_walker;  
                    bstree_walker= bstree_walker->right;
                }  
                else     
                {  
                    //if the bt_node found is a leaf node
                    if( bstree_walker->left== 0 && bstree_walker->right==0 ) 
                    {  
                        //this binaray search tree is merely have one node
                        //just delete it
                        if( parent==0 ) 
                        {  
                            delete bstree_walker;
                            return ;
                        }  
                        else 
                        {    
                            //assign the child of parent to delete to 0
                            (parent->left ==bstree_walker ) ? (parent->left=0):(parent->right=0) ; 
                            delete bstree_walker;  
                            return;
                        }  
      
                    }  
                    else if( bstree_walker->left==0 && bstree_walker->right !=0 )
                    {   
                        if( parent==NULL )
                        {  
                            _bstree = bstree_walker->right;
                            delete bstree_walker;    
                            return ;  
                        }  
                        else  
                        {    
                            ( parent->left == bstree_walker ) ? ( parent->left = bstree_walker->right ):(parent->right = bstree_walker->right );
                            delete bstree_walker;  
                            return ;  
                        }  
                    }  
                    else  if( bstree_walker->left !=0 && bstree_walker->right == 0 ) 
                    {  
           
                        if( parent==0 ) 
                        {  
                            _bstree = bstree_walker->right;
                            delete bstree_walker;    
                            return ;   
                        }  
                        else  
                        {  
                            ( parent->left== bstree_walker )? ( parent->left= bstree_walker->left):(parent->right=bstree_walker->left);
                            delete bstree_walker;  
                            return ;  
                        }  
      
                    }  
                    else
                    {  
                        //if both the left child and right child are exist
                        //then just find the minimum one of right child tree or the maximum one of left child tree
                        //to replace the node already found
                        BSTREE left_tree_walker = bstree_walker->left;
                        while( left_tree_walker->right != 0 )  
                        {  
                            parent = left_tree_walker;
                            left_tree_walker=left_tree_walker->right;
                        }
                        
                        bstree_walker->value = left_tree_walker->value;  
                        parent->right = left_tree_walker->left;  
                        delete left_tree_walker;  
                        return;   
                    }  
	    }
                
    }
}


#endif
