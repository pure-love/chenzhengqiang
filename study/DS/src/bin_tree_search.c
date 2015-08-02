#include "bin_tree_search.h"
#include <stdlib.h>
struct TREE_NODE
{
    element_t element;
    SEARCH_TREE left;
    SEARCH_TREE right;    
};

void bst_delete_tree( SEARCH_TREE search_tree )
{
    if( search_tree == NULL )
    return;
    bst_delete_tree(search_tree->left);
    bst_delete_tree(search_tree->right);
    free(search_tree);    
}


