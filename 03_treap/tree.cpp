#include "tree.h"

#include "../asserts.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

struct Node {
    int key;
    uint64_t priority;
    Node* left;
    Node* right;
};

struct Tree {
    Node* root;
    int size;
    uint64_t seed;
};

static uint64_t next_random(uint64_t* state) {
    uint64_t z = 0;

    ASSERT(state != nullptr);

    *state += 0x9e3779b97f4a7c15ULL;
    z       = *state;
    z       = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z       = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static Node* node_create(int key, uint64_t priority) {
    Node* node = (Node*) calloc(1, sizeof(Node));

    RETURN_IF(node == nullptr, nullptr);

    node->key      = key;
    node->priority = priority;
    node->left     = nullptr;
    node->right    = nullptr;
    return node;
}

Tree* tree_create(void) {
    Tree* tree = (Tree*) calloc(1, sizeof(Tree));

    RETURN_IF(tree == nullptr, nullptr);

    tree->seed = 0x123456789abcdefULL;
    return tree;
}

void tree_destroy(Tree* tree) {
    Node** stack = nullptr;
    int top = 0;

    if (tree == nullptr) {
        return;
    }

    if (tree->root != nullptr && tree->size > 0) {
        stack = (Node**) calloc((size_t) tree->size, sizeof(Node*));
        if (stack != nullptr) {
            stack[top++] = tree->root;
            while (top > 0) {
                Node* node = stack[--top];
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
    }

    free(tree);
}

void tree_set_seed(Tree* tree, uint64_t seed) {
    if (tree != nullptr) {
        tree->seed = seed == 0 ? 0x123456789abcdefULL : seed;
    }
}

int tree_insert(Tree* tree, int key) {
    Node*** path = nullptr;
    Node** link = nullptr;
    Node* node = nullptr;
    int depth = 0;

    RETURN_IF(tree == nullptr, 0);

    path = (Node***) calloc((size_t) (tree->size + 2), sizeof(Node**));
    RETURN_IF(path == nullptr, 0);

    link = &tree->root;
    while (*link != nullptr) {
        Node* current = *link;

        if (key == current->key) {
            CLEANUP_AND_RETURN_IF(1, free(path);, 0);
        }

        path[depth++] = link;
        if (key < current->key) {
            link = &current->left;
        } else {
            link = &current->right;
        }
    }

    node = node_create(key, next_random(&tree->seed));
    CLEANUP_AND_RETURN_IF(node == nullptr, free(path);, 0);

    *link = node;
    while (depth > 0 && (*path[depth - 1])->priority < node->priority) {
        Node** parent_link = path[depth - 1];
        Node* parent = *parent_link;

        ASSERT(parent != nullptr);

        if (parent->left == node) {
            parent->left = node->right;
            node->right = parent;
        } else {
            parent->right = node->left;
            node->left = parent;
        }

        *parent_link = node;
        depth        -= 1;
    }

    tree->size += 1;
    free(path);
    return 1;
}

int tree_erase(Tree* tree, int key) {
    Node** link = nullptr;
    Node* node = nullptr;

    RETURN_IF(tree == nullptr, 0);

    link = &tree->root;
    node = tree->root;
    while (node != nullptr && node->key != key) {
        if (key < node->key) {
            link = &node->left;
        } else {
            link = &node->right;
        }
        node = *link;
    }

    RETURN_IF(node == nullptr, 0);

    while (node->left != nullptr || node->right != nullptr) {
        if (node->left == nullptr ||
            (node->right != nullptr && node->right->priority > node->left->priority)) {
            Node* right = node->right;

            ASSERT(right != nullptr);

            node->right = right->left;
            right->left = node;
            *link       = right;
            link        = &right->left;
        } else {
            Node* left = node->left;

            ASSERT(left != nullptr);

            node->left = left->right;
            left->right = node;
            *link      = left;
            link       = &left->right;
        }
    }

    *link = nullptr;
    free(node);
    tree->size -= 1;
    return 1;
}

int tree_contains(Tree* tree, int key) {
    Node* current = tree == nullptr ? nullptr : tree->root;

    while (current != nullptr) {
        if (key == current->key) {
            return 1;
        }
        current = key < current->key ? current->left : current->right;
    }

    return 0;
}

int tree_size(Tree* tree) {
    return tree == nullptr ? 0 : tree->size;
}

int tree_validate(Tree* tree) {
    struct Frame {
        Node* node;
        long long min_key;
        long long max_key;
        uint64_t parent_priority;
        int has_parent;
    };

    Frame* stack = nullptr;
    int top = 0;
    int count = 0;

    RETURN_IF(tree == nullptr, 0);
    if (tree->root == nullptr) {
        return tree->size == 0;
    }

    stack = (Frame*) calloc((size_t) tree->size, sizeof(Frame));
    if (stack == nullptr) {
        return 0;
    }

    stack[top++] = {tree->root, (long long) INT_MIN - 1LL, (long long) INT_MAX + 1LL, 0, 0};
    while (top > 0) {
        Frame frame = stack[--top];
        Node* node = frame.node;

        ASSERT(node != nullptr);

        if (node->key <= frame.min_key || node->key >= frame.max_key) {
            free(stack);
            return 0;
        }
        if (frame.has_parent && node->priority > frame.parent_priority) {
            free(stack);
            return 0;
        }

        count += 1;

        if (node->right != nullptr) {
            stack[top++] = {node->right, node->key, frame.max_key, node->priority, 1};
        }
        if (node->left != nullptr) {
            stack[top++] = {node->left, frame.min_key, node->key, node->priority, 1};
        }
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

    stack = (Node**) calloc((size_t) tree->size, sizeof(Node*));
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
    return "Treap";
}
