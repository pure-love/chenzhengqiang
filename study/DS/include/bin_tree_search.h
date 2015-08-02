/*
@author:chenzhengqiang
@start date:2015/8/2
@desc:implementaion of the binary search tree
*/
#ifndef _CZQ_BST_H_
#define _CZQ_BST_H_

typedef int element_t;
struct  TREE_NODE;
typedef TREE_NODE * POSITION;
typedef TREE_NODE * SEARCH_TREE;

void     bst_delete_tree(SEARCH_TREE search_tree);
POSITION bst_find(element_t element, SEARCH_TREE search_tree);
POSITION bst_find_min(SEARCH_TREE search_tree);
POSITION bst_find_max(SEARCH_TREE search_tree);
POSITION bst_insert(element_t element, SEARCH_TREE search_tree);
POSITION bst_delete_node(element_t element, SEARCH_TREE search_tree);

#endif
