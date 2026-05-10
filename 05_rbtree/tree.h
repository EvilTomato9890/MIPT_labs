#ifndef RBTREE_TREE_H_INCLUDED
#define RBTREE_TREE_H_INCLUDED

#include <stdint.h>

struct Tree;

Tree* tree_create(void);
void tree_destroy(Tree* tree);
void tree_set_seed(Tree* tree, uint64_t seed);
int tree_insert(Tree* tree, int key);
int tree_erase(Tree* tree, int key);
int tree_contains(Tree* tree, int key);
int tree_size(Tree* tree);
int tree_validate(Tree* tree);
int tree_export_keys(Tree* tree, int* out, int capacity);
const char* tree_name(void);

#endif /* RBTREE_TREE_H_INCLUDED */
