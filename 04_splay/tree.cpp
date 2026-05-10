#include "tree.h"

#include "../asserts.h"

#include <stdint.h>
#include <stdlib.h>

struct Node {
    int key;
    Node* left;
    Node* right;
    Node* parent;
};

struct Tree {
    Node* root;
    int size;
    uint64_t seed;
};

static Node* node_create(int key) {
    Node* node = (Node*) calloc(1, sizeof(Node));

    RETURN_IF(node == nullptr, nullptr);

    node->key    = key;
    node->left   = nullptr;
    node->right  = nullptr;
    node->parent = nullptr;
    return node;
}

static void rotate_left(Tree* tree, Node* x) {
    ASSERT(tree != nullptr);
    ASSERT(x != nullptr);

    Node* y = x->right;

    ASSERT(y != nullptr);

    x->right = y->left;
    if (y->left != nullptr) {
        y->left->parent = x;
    }

    y->parent = x->parent;
    if (x->parent == nullptr) {
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

    ASSERT(y != nullptr);

    x->left = y->right;
    if (y->right != nullptr) {
        y->right->parent = x;
    }

    y->parent = x->parent;
    if (x->parent == nullptr) {
        tree->root = y;
    } else if (x == x->parent->right) {
        x->parent->right = y;
    } else {
        x->parent->left = y;
    }

    y->right  = x;
    x->parent = y;
}

static void splay(Tree* tree, Node* node) {
    ASSERT(tree != nullptr);
    ASSERT(node != nullptr);

    while (node->parent != nullptr) {
        Node* parent = node->parent;
        Node* grand  = parent->parent;

        if (grand == nullptr) {
            if (node == parent->left) {
                rotate_right(tree, parent);
            } else {
                rotate_left(tree, parent);
            }
        } else if (node == parent->left && parent == grand->left) {
            rotate_right(tree, grand);
            rotate_right(tree, parent);
        } else if (node == parent->right && parent == grand->right) {
            rotate_left(tree, grand);
            rotate_left(tree, parent);
        } else if (node == parent->right && parent == grand->left) {
            rotate_left(tree, parent);
            rotate_right(tree, grand);
        } else {
            rotate_right(tree, parent);
            rotate_left(tree, grand);
        }
    }
}

static Node* find_node(Tree* tree, int key) {
    ASSERT(tree != nullptr);

    Node* current = tree->root;
    Node* last    = nullptr;

    while (current != nullptr) {
        last = current;
        if (key == current->key) {
            splay(tree, current);
            return current;
        }
        current = key < current->key ? current->left : current->right;
    }

    if (last != nullptr) {
        splay(tree, last);
    }

    return nullptr;
}

static Node* subtree_max(Node* node) {
    ASSERT(node != nullptr);

    while (node != nullptr && node->right != nullptr) {
        node = node->right;
    }

    return node;
}

static void destroy_nodes(Tree* tree) {
    Node** stack = nullptr;
    int top       = 0;

    ASSERT(tree != nullptr);

    if (tree->root == nullptr) {
        return;
    }

    stack = (Node**) malloc((size_t) tree->size * sizeof(Node*));
    if (stack == nullptr) {
        return;
    }

    stack[top++] = tree->root;
    while (top > 0) {
        Node* node = stack[--top];

        ASSERT(node != nullptr);
        if (node->left != nullptr) {
            stack[top++] = node->left;
        }
        if (node->right != nullptr) {
            stack[top++] = node->right;
        }
        free(node);
    }

    free(stack);
}

Tree* tree_create(void) {
    Tree* tree = (Tree*) calloc(1, sizeof(Tree));

    RETURN_IF(tree == nullptr, nullptr);

    tree->seed = 1;
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

    current = tree->root;
    while (current != nullptr) {
        parent = current;
        if (key == current->key) {
            splay(tree, current);
            return 0;
        }
        current = key < current->key ? current->left : current->right;
    }

    node = node_create(key);
    RETURN_IF(node == nullptr, 0);

    node->parent = parent;
    if (parent == nullptr) {
        tree->root = node;
    } else if (key < parent->key) {
        parent->left = node;
    } else {
        parent->right = node;
    }

    splay(tree, node);
    tree->size += 1;
    return 1;
}

int tree_erase(Tree* tree, int key) {
    Node* node = nullptr;
    Node* left = nullptr;
    Node* right = nullptr;
    Node* max_left = nullptr;

    RETURN_IF(tree == nullptr, 0);

    node = find_node(tree, key);
    RETURN_IF(node == nullptr || tree->root->key != key, 0);

    left  = node->left;
    right = node->right;
    if (left != nullptr) {
        left->parent = nullptr;
    }
    if (right != nullptr) {
        right->parent = nullptr;
    }

    free(node);
    tree->size -= 1;

    if (left == nullptr) {
        tree->root = right;
        return 1;
    }

    tree->root = left;
    max_left = subtree_max(left);
    splay(tree, max_left);
    tree->root->right = right;
    if (right != nullptr) {
        right->parent = tree->root;
    }

    return 1;
}

int tree_contains(Tree* tree, int key) {
    return tree != nullptr && find_node(tree, key) != nullptr;
}

int tree_size(Tree* tree) {
    return tree == nullptr ? 0 : tree->size;
}

int tree_validate(Tree* tree) {
    Node** stack = nullptr;
    Node* current = nullptr;
    int top = 0;
    int count = 0;
    int has_previous = 0;
    int previous = 0;

    RETURN_IF(tree == nullptr, 0);
    if (tree->root == nullptr) {
        return tree->size == 0;
    }
    if (tree->root->parent != nullptr) {
        return 0;
    }

    stack = (Node**) malloc((size_t) tree->size * sizeof(Node*));
    if (stack == nullptr) {
        return 0;
    }

    current = tree->root;
    while (current != nullptr || top > 0) {
        while (current != nullptr) {
            if (current->left != nullptr && current->left->parent != current) {
                free(stack);
                return 0;
            }
            if (current->right != nullptr && current->right->parent != current) {
                free(stack);
                return 0;
            }
            stack[top++] = current;
            current = current->left;
        }

        current = stack[--top];
        if (has_previous && previous >= current->key) {
            free(stack);
            return 0;
        }
        previous = current->key;
        has_previous = 1;
        count += 1;
        current = current->right;
    }

    free(stack);
    return count == tree->size;
}

int tree_export_keys(Tree* tree, int* out, int capacity) {
    Node** stack = nullptr;
    Node* current = nullptr;
    int count = 0;
    int top = 0;

    RETURN_IF(tree == nullptr || out == nullptr || capacity <= 0, 0);
    if (tree->root == nullptr) {
        return 0;
    }

    stack = (Node**) malloc((size_t) tree->size * sizeof(Node*));
    if (stack == nullptr) {
        return 0;
    }

    current = tree->root;
    while ((current != nullptr || top > 0) && count < capacity) {
        while (current != nullptr) {
            stack[top++] = current;
            current = current->left;
        }

        current = stack[--top];
        out[count] = current->key;
        count      += 1;
        current    = current->right;
    }

    free(stack);
    return count;
}

const char* tree_name(void) {
    return "Splay";
}
