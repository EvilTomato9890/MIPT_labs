#include "tree.h"

#include "../asserts.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

enum Color {
    RED = 0,
    BLACK = 1
};

struct Node {
    int key;
    int color;
    Node* left;
    Node* right;
    Node* parent;
};

struct Tree {
    Node nil_node;
    Node* nil;
    Node* root;
    int size;
    uint64_t seed;
};

static Node* node_create(Tree* tree, int key) {
    Node* node = (Node*) calloc(1, sizeof(Node));

    ASSERT(tree != nullptr);
    RETURN_IF(node == nullptr, nullptr);

    node->key    = key;
    node->color  = RED;
    node->left   = tree->nil;
    node->right  = tree->nil;
    node->parent = tree->nil;
    return node;
}

static void rotate_left(Tree* tree, Node* x) {
    ASSERT(tree != nullptr);
    ASSERT(x != nullptr);

    Node* y = x->right;

    ASSERT(y != tree->nil);

    x->right = y->left;
    if (y->left != tree->nil) {
        y->left->parent = x;
    }

    y->parent = x->parent;
    if (x->parent == tree->nil) {
        tree->root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }

    y->left   = x;
    x->parent = y;
}

static void rotate_right(Tree* tree, Node* x) {
    ASSERT(tree != nullptr);
    ASSERT(x != nullptr);

    Node* y = x->left;

    ASSERT(y != tree->nil);

    x->left = y->right;
    if (y->right != tree->nil) {
        y->right->parent = x;
    }

    y->parent = x->parent;
    if (x->parent == tree->nil) {
        tree->root = y;
    } else if (x == x->parent->right) {
        x->parent->right = y;
    } else {
        x->parent->left = y;
    }

    y->right  = x;
    x->parent = y;
}

static Node* find_node(Tree* tree, int key) {
    ASSERT(tree != nullptr);

    Node* current = tree->root;

    while (current != tree->nil) {
        if (key == current->key) {
            return current;
        }
        current = key < current->key ? current->left : current->right;
    }

    return tree->nil;
}

static Node* minimum_node(Tree* tree, Node* node) {
    ASSERT(tree != nullptr);
    ASSERT(node != tree->nil);

    while (node->left != tree->nil) {
        node = node->left;
    }

    return node;
}

static void transplant(Tree* tree, Node* old_node, Node* new_node) {
    ASSERT(tree != nullptr);
    ASSERT(old_node != tree->nil);
    ASSERT(new_node != nullptr);

    if (old_node->parent == tree->nil) {
        tree->root = new_node;
    } else if (old_node == old_node->parent->left) {
        old_node->parent->left = new_node;
    } else {
        old_node->parent->right = new_node;
    }

    new_node->parent = old_node->parent;
}

static void insert_fixup(Tree* tree, Node* z) {
    ASSERT(tree != nullptr);
    ASSERT(z != tree->nil);

    while (z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            Node* y = z->parent->parent->right;
            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rotate_left(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rotate_right(tree, z->parent->parent);
            }
        } else {
            Node* y = z->parent->parent->left;
            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rotate_right(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rotate_left(tree, z->parent->parent);
            }
        }
    }

    tree->root->color = BLACK;
}

static void delete_fixup(Tree* tree, Node* x) {
    ASSERT(tree != nullptr);
    ASSERT(x != nullptr);

    while (x != tree->root && x->color == BLACK) {
        if (x == x->parent->left) {
            Node* w = x->parent->right;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                rotate_left(tree, x->parent);
                w = x->parent->right;
            }
            if (w->left->color == BLACK && w->right->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->right->color == BLACK) {
                    w->left->color = BLACK;
                    w->color = RED;
                    rotate_right(tree, w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                rotate_left(tree, x->parent);
                x = tree->root;
            }
        } else {
            Node* w = x->parent->left;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                rotate_right(tree, x->parent);
                w = x->parent->left;
            }
            if (w->right->color == BLACK && w->left->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->left->color == BLACK) {
                    w->right->color = BLACK;
                    w->color = RED;
                    rotate_left(tree, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                rotate_right(tree, x->parent);
                x = tree->root;
            }
        }
    }

    x->color = BLACK;
}

static void destroy_nodes(Tree* tree) {
    Node** stack = nullptr;
    int top       = 0;

    ASSERT(tree != nullptr);

    if (tree->root == tree->nil || tree->size <= 0) {
        return;
    }

    stack = (Node**) calloc((size_t) tree->size, sizeof(Node*));
    if (stack == nullptr) {
        return;
    }

    stack[top++] = tree->root;
    while (top > 0) {
        Node* node = stack[--top];

        ASSERT(node != tree->nil);
        if (node->left != tree->nil) {
            stack[top++] = node->left;
        }
        if (node->right != tree->nil) {
            stack[top++] = node->right;
        }
        free(node);
    }

    free(stack);
}

static int validate_node(Tree* tree,
                         Node* node,
                         long long min_key,
                         long long max_key,
                         Node* parent,
                         int* count,
                         int* black_height) {
    int left_black_height = 0;
    int right_black_height = 0;

    ASSERT(tree != nullptr);
    ASSERT(count != nullptr);
    ASSERT(black_height != nullptr);

    if (node == tree->nil) {
        *black_height = 1;
        return 1;
    }

    if (node->key <= min_key || node->key >= max_key || node->parent != parent) {
        return 0;
    }
    if (node->color != RED && node->color != BLACK) {
        return 0;
    }
    if (node->color == RED &&
        (node->left->color != BLACK || node->right->color != BLACK)) {
        return 0;
    }

    if (!validate_node(tree, node->left, min_key, node->key, node, count, &left_black_height)) {
        return 0;
    }
    if (!validate_node(tree, node->right, node->key, max_key, node, count, &right_black_height)) {
        return 0;
    }
    if (left_black_height != right_black_height) {
        return 0;
    }

    *black_height = left_black_height + (node->color == BLACK ? 1 : 0);
    *count += 1;
    return 1;
}

static void export_node(Tree* tree, Node* node, int* out, int capacity, int* count) {
    ASSERT(tree != nullptr);
    ASSERT(out != nullptr);
    ASSERT(count != nullptr);

    if (node == tree->nil || *count >= capacity) {
        return;
    }

    export_node(tree, node->left, out, capacity, count);
    if (*count < capacity) {
        out[*count] = node->key;
        *count += 1;
    }
    export_node(tree, node->right, out, capacity, count);
}

Tree* tree_create(void) {
    Tree* tree = (Tree*) calloc(1, sizeof(Tree));

    RETURN_IF(tree == nullptr, nullptr);

    tree->nil         = &tree->nil_node;
    tree->nil->color  = BLACK;
    tree->nil->left   = tree->nil;
    tree->nil->right  = tree->nil;
    tree->nil->parent = tree->nil;
    tree->root        = tree->nil;
    tree->seed        = 1;
    return tree;
}

void tree_destroy(Tree* tree) {
    if (tree == nullptr) {
        return;
    }

    destroy_nodes(tree);
    free(tree);
}

void tree_set_seed(Tree* tree, uint64_t seed) {
    if (tree != nullptr) {
        tree->seed = seed;
    }
}

int tree_insert(Tree* tree, int key) {
    Node* parent = nullptr;
    Node* current = nullptr;
    Node* node = nullptr;

    RETURN_IF(tree == nullptr, 0);

    parent  = tree->nil;
    current = tree->root;
    while (current != tree->nil) {
        parent = current;
        if (key == current->key) {
            return 0;
        }
        current = key < current->key ? current->left : current->right;
    }

    node = node_create(tree, key);
    RETURN_IF(node == nullptr, 0);

    node->parent = parent;
    if (parent == tree->nil) {
        tree->root = node;
    } else if (key < parent->key) {
        parent->left = node;
    } else {
        parent->right = node;
    }

    insert_fixup(tree, node);
    tree->size += 1;
    return 1;
}

int tree_erase(Tree* tree, int key) {
    Node* z = nullptr;
    Node* y = nullptr;
    Node* x = nullptr;
    int y_original_color = BLACK;

    RETURN_IF(tree == nullptr, 0);

    z = find_node(tree, key);
    RETURN_IF(z == tree->nil, 0);

    y                = z;
    y_original_color = y->color;

    if (z->left == tree->nil) {
        x = z->right;
        transplant(tree, z, z->right);
    } else if (z->right == tree->nil) {
        x = z->left;
        transplant(tree, z, z->left);
    } else {
        y                = minimum_node(tree, z->right);
        y_original_color = y->color;
        x                = y->right;
        if (y->parent == z) {
            x->parent = y;
        } else {
            transplant(tree, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        transplant(tree, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }

    free(z);
    tree->size -= 1;

    if (y_original_color == BLACK) {
        delete_fixup(tree, x);
    }

    return 1;
}

int tree_contains(Tree* tree, int key) {
    return tree != nullptr && find_node(tree, key) != tree->nil;
}

int tree_size(Tree* tree) {
    return tree == nullptr ? 0 : tree->size;
}

int tree_validate(Tree* tree) {
    int count = 0;
    int black_height = 0;

    RETURN_IF(tree == nullptr, 0);
    if (tree->nil->color != BLACK) {
        return 0;
    }
    if (tree->root == tree->nil) {
        return tree->size == 0;
    }
    if (tree->root->color != BLACK || tree->root->parent != tree->nil) {
        return 0;
    }

    if (!validate_node(tree,
                       tree->root,
                       (long long) INT_MIN - 1LL,
                       (long long) INT_MAX + 1LL,
                       tree->nil,
                       &count,
                       &black_height)) {
        return 0;
    }

    return count == tree->size;
}

int tree_export_keys(Tree* tree, int* out, int capacity) {
    int count = 0;

    RETURN_IF(tree == nullptr || out == nullptr || capacity <= 0, 0);

    export_node(tree, tree->root, out, capacity, &count);
    return count;
}

const char* tree_name(void) {
    return "Red-Black";
}
