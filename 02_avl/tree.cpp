#include "tree.h"

#include "../asserts.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

struct Node {
    int key;
    int height;
    Node* left;
    Node* right;
};

struct Tree {
    Node* root;
    int size;
    uint64_t seed;
};

static int max_int(int a, int b) {
    return a > b ? a : b;
}

static int abs_int(int value) {
    return value < 0 ? -value : value;
}

static int height(Node* node) {
    return node == nullptr ? 0 : node->height;
}

static Node* node_create(int key) {
    Node* node = (Node*) calloc(1, sizeof(Node));

    RETURN_IF(node == nullptr, nullptr);

    node->key    = key;
    node->height = 1;
    node->left   = nullptr;
    node->right  = nullptr;
    return node;
}

static void update_height(Node* node) {
    ASSERT(node != nullptr);

    if (node != nullptr) {
        node->height = 1 + max_int(height(node->left), height(node->right));
    }
}

static int balance_factor(Node* node) {
    return node == nullptr ? 0 : height(node->left) - height(node->right);
}

static Node* rotate_right(Node* y) {
    ASSERT(y != nullptr);

    Node* x = y->left;

    ASSERT(x != nullptr);

    Node* t2 = x->right;

    x->right = y;
    y->left = t2;

    update_height(y);
    update_height(x);
    return x;
}

static Node* rotate_left(Node* x) {
    ASSERT(x != nullptr);

    Node* y = x->right;

    ASSERT(y != nullptr);

    Node* t2 = y->left;

    y->left = x;
    x->right = t2;

    update_height(x);
    update_height(y);
    return y;
}

static Node* rebalance(Node* node) {
    int balance = 0;

    ASSERT(node != nullptr);

    update_height(node);
    balance = balance_factor(node);

    if (balance > 1) {
        if (balance_factor(node->left) < 0) {
            node->left = rotate_left(node->left);
        }
        return rotate_right(node);
    }

    if (balance < -1) {
        if (balance_factor(node->right) > 0) {
            node->right = rotate_right(node->right);
        }
        return rotate_left(node);
    }

    return node;
}

static Node* insert_rec(Node* node, int key, int* inserted) {
    ASSERT(inserted != nullptr);

    if (node == nullptr) {
        *inserted = 1;
        return node_create(key);
    }

    if (key < node->key) {
        node->left = insert_rec(node->left, key, inserted);
    } else if (key > node->key) {
        node->right = insert_rec(node->right, key, inserted);
    } else {
        *inserted = 0;
        return node;
    }

    return rebalance(node);
}

static void destroy_node(Node* node) {
    if (node == nullptr) {
        return;
    }

    destroy_node(node->left);
    destroy_node(node->right);
    free(node);
}

static int validate_node(Node* node, long long min_key, long long max_key, int* count, int* out_height) {
    int left_height = 0;
    int right_height = 0;

    ASSERT(count != nullptr);
    ASSERT(out_height != nullptr);

    if (node == nullptr) {
        *out_height = 0;
        return 1;
    }

    if (node->key <= min_key || node->key >= max_key) {
        return 0;
    }

    if (!validate_node(node->left, min_key, node->key, count, &left_height)) {
        return 0;
    }
    if (!validate_node(node->right, node->key, max_key, count, &right_height)) {
        return 0;
    }

    if (abs_int(left_height - right_height) > 1) {
        return 0;
    }

    *out_height = 1 + max_int(left_height, right_height);
    if (node->height != *out_height) {
        return 0;
    }

    *count += 1;
    return 1;
}

static void export_node(Node* node, int* out, int capacity, int* count) {
    ASSERT(out != nullptr);
    ASSERT(count != nullptr);
    ASSERT(capacity >= 0);

    if (node == nullptr || *count >= capacity) {
        return;
    }

    export_node(node->left, out, capacity, count);
    if (*count < capacity) {
        out[*count] = node->key;
        *count += 1;
    }
    export_node(node->right, out, capacity, count);
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

    destroy_node(tree->root);
    free(tree);
}

void tree_set_seed(Tree* tree, uint64_t seed) {
    if (tree != nullptr) {
        tree->seed = seed;
    }
}

int tree_insert(Tree* tree, int key) {
    int inserted = 0;

    RETURN_IF(tree == nullptr, 0);

    tree->root = insert_rec(tree->root, key, &inserted);
    if (inserted) {
        tree->size += 1;
    }

    return inserted;
}

int tree_erase(Tree* tree, int key) {
    Node*** path = nullptr;
    Node** link = nullptr;
    Node* node = nullptr;
    int depth = 0;
    int i = 0;

    RETURN_IF(tree == nullptr || tree->root == nullptr, 0);

    path = (Node***) calloc((size_t) (tree->size + 2), sizeof(Node**));
    RETURN_IF(path == nullptr, 0);

    link = &tree->root;
    node = tree->root;
    while (node != nullptr && node->key != key) {
        path[depth++] = link;
        if (key < node->key) {
            link = &node->left;
        } else {
            link = &node->right;
        }
        node = *link;
    }

    if (node == nullptr) {
        CLEANUP_AND_RETURN_IF(1, free(path);, 0);
    }

    if (node->left != nullptr && node->right != nullptr) {
        Node** successor_link = nullptr;
        Node* successor = nullptr;

        path[depth++] = link;
        successor_link = &node->right;
        successor      = node->right;
        while (successor->left != nullptr) {
            path[depth++] = successor_link;
            successor_link = &successor->left;
            successor      = *successor_link;
        }

        node->key        = successor->key;
        *successor_link = successor->right;
        free(successor);
    } else {
        Node* child = node->left != nullptr ? node->left : node->right;
        *link = child;
        free(node);
    }

    tree->size -= 1;

    for (i = depth - 1; i >= 0; --i) {
        if (*path[i] != nullptr) {
            *path[i] = rebalance(*path[i]);
        }
    }

    free(path);
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
    int count = 0;
    int computed_height = 0;

    RETURN_IF(tree == nullptr, 0);

    if (!validate_node(tree->root, (long long) INT_MIN - 1LL, (long long) INT_MAX + 1LL, &count, &computed_height)) {
        return 0;
    }

    return count == tree->size;
}

int tree_export_keys(Tree* tree, int* out, int capacity) {
    int count = 0;

    RETURN_IF(tree == nullptr || out == nullptr || capacity <= 0, 0);

    export_node(tree->root, out, capacity, &count);
    return count;
}

const char* tree_name(void) {
    return "AVL";
}
