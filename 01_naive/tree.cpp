#include "tree.h"

#include "../asserts.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
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

static Node* find_node(Tree* tree, int key) {
    ASSERT(tree != nullptr);

    Node* current = tree->root;

    while (current != nullptr) {
        if (key == current->key) {
            return current;
        }
        current = key < current->key ? current->left : current->right;
    }

    return nullptr;
}

static Node* minimum_node(Node* node) {
    ASSERT(node != nullptr);

    while (node != nullptr && node->left != nullptr) {
        node = node->left;
    }

    return node;
}

static void transplant(Tree* tree, Node* old_node, Node* new_node) {
    ASSERT(tree != nullptr);
    ASSERT(old_node != nullptr);

    if (old_node->parent == nullptr) {
        tree->root = new_node;
    } else if (old_node == old_node->parent->left) {
        old_node->parent->left = new_node;
    } else {
        old_node->parent->right = new_node;
    }

    if (new_node != nullptr) {
        new_node->parent = old_node->parent;
    }
}

Tree* tree_create(void) {
    Tree* tree = (Tree*) calloc(1, sizeof(Tree));

    RETURN_IF(tree == nullptr, nullptr);

    tree->seed = 1;
    return tree;
}

void tree_destroy(Tree* tree) {
    Node* current = nullptr;

    if (tree == nullptr) {
        return;
    }

    current = tree->root;
    while (current != nullptr) {
        if (current->left != nullptr) {
            current = current->left;
        } else if (current->right != nullptr) {
            current = current->right;
        } else {
            Node* parent = current->parent;

            if (parent != nullptr) {
                if (parent->left == current) {
                    parent->left = nullptr;
                } else {
                    parent->right = nullptr;
                }
            }

            free(current);
            current = parent;
        }
    }

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

    tree->size += 1;
    return 1;
}

int tree_erase(Tree* tree, int key) {
    Node* node = nullptr;
    Node* successor = nullptr;

    RETURN_IF(tree == nullptr, 0);

    node = find_node(tree, key);
    RETURN_IF(node == nullptr, 0);

    if (node->left == nullptr) {
        transplant(tree, node, node->right);
    } else if (node->right == nullptr) {
        transplant(tree, node, node->left);
    } else {
        successor = minimum_node(node->right);
        if (successor->parent != node) {
            transplant(tree, successor, successor->right);
            successor->right = node->right;
            successor->right->parent = successor;
        }
        transplant(tree, node, successor);
        successor->left = node->left;
        successor->left->parent = successor;
    }

    free(node);
    tree->size -= 1;
    return 1;
}

int tree_contains(Tree* tree, int key) {
    RETURN_IF(tree == nullptr, 0);

    return find_node(tree, key) != nullptr;
}

int tree_size(Tree* tree) {
    return tree == nullptr ? 0 : tree->size;
}

int tree_validate(Tree* tree) {
    Node** stack = nullptr;
    Node* current = nullptr;
    int count = 0;
    int top = 0;
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
        previous     = current->key;
        has_previous = 1;
        count        += 1;
        current      = current->right;
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
    return "Naive BST";
}
