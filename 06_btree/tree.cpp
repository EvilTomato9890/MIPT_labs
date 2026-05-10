#include "tree.h"

#include "../asserts.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#define BTREE_T 64
#define BTREE_MAX_KEYS (2 * BTREE_T - 1)
#define BTREE_MAX_CHILDREN (2 * BTREE_T)

struct Node {
    int n;
    int leaf;
    int keys[BTREE_MAX_KEYS];
    Node* children[BTREE_MAX_CHILDREN];
};

struct Tree {
    Node* root;
    int size;
    uint64_t seed;
};

static Node* node_create(int leaf) {
    Node* node = (Node*) calloc(1, sizeof(Node));

    RETURN_IF(node == nullptr, nullptr);

    node->leaf = leaf;
    return node;
}

static void destroy_node(Node* node) {
    int i = 0;

    if (node == nullptr) {
        return;
    }

    if (!node->leaf) {
        for (i = 0; i <= node->n; ++i) {
            destroy_node(node->children[i]);
        }
    }

    free(node);
}

static int find_key(Node* node, int key) {
    int idx = 0;

    ASSERT(node != nullptr);

    while (idx < node->n && node->keys[idx] < key) {
        idx += 1;
    }

    return idx;
}

static int node_contains(Node* node, int key) {
    ASSERT(node != nullptr);

    int idx = find_key(node, key);

    if (idx < node->n && node->keys[idx] == key) {
        return 1;
    }
    if (node->leaf) {
        return 0;
    }

    return node_contains(node->children[idx], key);
}

static void split_child(Node* parent, int index) {
    ASSERT(parent != nullptr);

    Node* full = parent->children[index];

    ASSERT(full != nullptr);

    Node* right = node_create(full->leaf);
    int j = 0;

    ASSERT(right != nullptr);

    right->n = BTREE_T - 1;

    for (j = 0; j < BTREE_T - 1; ++j) {
        right->keys[j] = full->keys[j + BTREE_T];
    }
    if (!full->leaf) {
        for (j = 0; j < BTREE_T; ++j) {
            right->children[j] = full->children[j + BTREE_T];
        }
    }

    full->n = BTREE_T - 1;

    for (j = parent->n; j >= index + 1; --j) {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[index + 1] = right;

    for (j = parent->n - 1; j >= index; --j) {
        parent->keys[j + 1] = parent->keys[j];
    }
    parent->keys[index] = full->keys[BTREE_T - 1];
    parent->n += 1;
}

static void insert_nonfull(Node* node, int key) {
    int i = node->n - 1;

    ASSERT(node != nullptr);
    ASSERT(node->n < BTREE_MAX_KEYS);

    if (node->leaf) {
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            i -= 1;
        }
        node->keys[i + 1] = key;
        node->n += 1;
        return;
    }

    while (i >= 0 && key < node->keys[i]) {
        i -= 1;
    }
    i += 1;

    if (node->children[i]->n == BTREE_MAX_KEYS) {
        split_child(node, i);
        if (key > node->keys[i]) {
            i += 1;
        }
    }

    insert_nonfull(node->children[i], key);
}

static int get_predecessor(Node* node) {
    ASSERT(node != nullptr);
    ASSERT(node->n > 0);

    while (!node->leaf) {
        node = node->children[node->n];
    }

    return node->keys[node->n - 1];
}

static int get_successor(Node* node) {
    ASSERT(node != nullptr);
    ASSERT(node->n > 0);

    while (!node->leaf) {
        node = node->children[0];
    }

    return node->keys[0];
}

static void remove_from_leaf(Node* node, int idx) {
    int i = 0;

    ASSERT(node != nullptr);
    ASSERT(node->leaf);
    ASSERT(idx >= 0 && idx < node->n);

    for (i = idx + 1; i < node->n; ++i) {
        node->keys[i - 1] = node->keys[i];
    }
    node->n -= 1;
}

static void merge_children(Node* node, int idx) {
    ASSERT(node != nullptr);
    ASSERT(idx >= 0 && idx < node->n);

    Node* child   = node->children[idx];
    Node* sibling = node->children[idx + 1];
    int i         = 0;

    ASSERT(child != nullptr);
    ASSERT(sibling != nullptr);

    child->keys[BTREE_T - 1] = node->keys[idx];

    for (i = 0; i < sibling->n; ++i) {
        child->keys[i + BTREE_T] = sibling->keys[i];
    }

    if (!child->leaf) {
        for (i = 0; i <= sibling->n; ++i) {
            child->children[i + BTREE_T] = sibling->children[i];
        }
    }

    for (i = idx + 1; i < node->n; ++i) {
        node->keys[i - 1] = node->keys[i];
    }
    for (i = idx + 2; i <= node->n; ++i) {
        node->children[i - 1] = node->children[i];
    }

    child->n += sibling->n + 1;
    node->n -= 1;
    free(sibling);
}

static void borrow_from_prev(Node* node, int idx) {
    ASSERT(node != nullptr);
    ASSERT(idx > 0 && idx <= node->n);

    Node* child   = node->children[idx];
    Node* sibling = node->children[idx - 1];
    int i         = 0;

    ASSERT(child != nullptr);
    ASSERT(sibling != nullptr);
    ASSERT(sibling->n >= BTREE_T);

    for (i = child->n - 1; i >= 0; --i) {
        child->keys[i + 1] = child->keys[i];
    }
    if (!child->leaf) {
        for (i = child->n; i >= 0; --i) {
            child->children[i + 1] = child->children[i];
        }
    }

    child->keys[0] = node->keys[idx - 1];
    if (!child->leaf) {
        child->children[0] = sibling->children[sibling->n];
    }

    node->keys[idx - 1] = sibling->keys[sibling->n - 1];
    child->n += 1;
    sibling->n -= 1;
}

static void borrow_from_next(Node* node, int idx) {
    ASSERT(node != nullptr);
    ASSERT(idx >= 0 && idx < node->n);

    Node* child   = node->children[idx];
    Node* sibling = node->children[idx + 1];
    int i         = 0;

    ASSERT(child != nullptr);
    ASSERT(sibling != nullptr);
    ASSERT(sibling->n >= BTREE_T);

    child->keys[child->n] = node->keys[idx];
    if (!child->leaf) {
        child->children[child->n + 1] = sibling->children[0];
    }

    node->keys[idx] = sibling->keys[0];

    for (i = 1; i < sibling->n; ++i) {
        sibling->keys[i - 1] = sibling->keys[i];
    }
    if (!sibling->leaf) {
        for (i = 1; i <= sibling->n; ++i) {
            sibling->children[i - 1] = sibling->children[i];
        }
    }

    child->n += 1;
    sibling->n -= 1;
}

static void fill_child(Node* node, int idx) {
    ASSERT(node != nullptr);
    ASSERT(idx >= 0 && idx <= node->n);
    ASSERT(!node->leaf);

    if (idx != 0 && node->children[idx - 1]->n >= BTREE_T) {
        borrow_from_prev(node, idx);
    } else if (idx != node->n && node->children[idx + 1]->n >= BTREE_T) {
        borrow_from_next(node, idx);
    } else {
        if (idx != node->n) {
            merge_children(node, idx);
        } else {
            merge_children(node, idx - 1);
        }
    }
}

static int validate_node(Node* node,
                         long long min_key,
                         long long max_key,
                         int depth,
                         int is_root,
                         int* leaf_depth,
                         int* count) {
    int i = 0;

    ASSERT(leaf_depth != nullptr);
    ASSERT(count != nullptr);

    if (node == nullptr) {
        return 0;
    }
    if (node->n < 0 || node->n > BTREE_MAX_KEYS) {
        return 0;
    }
    if (!is_root && node->n < BTREE_T - 1) {
        return 0;
    }
    if (is_root && node->n == 0 && !node->leaf) {
        return 0;
    }

    for (i = 0; i < node->n; ++i) {
        if (node->keys[i] <= min_key || node->keys[i] >= max_key) {
            return 0;
        }
        if (i > 0 && node->keys[i - 1] >= node->keys[i]) {
            return 0;
        }
    }

    *count += node->n;

    if (node->leaf) {
        if (*leaf_depth == -1) {
            *leaf_depth = depth;
        }
        return *leaf_depth == depth;
    }

    for (i = 0; i <= node->n; ++i) {
        long long child_min = (i == 0) ? min_key : node->keys[i - 1];
        long long child_max = (i == node->n) ? max_key : node->keys[i];

        if (!validate_node(node->children[i], child_min, child_max, depth + 1, 0, leaf_depth, count)) {
            return 0;
        }
    }

    return 1;
}

static void export_node(Node* node, int* out, int capacity, int* count) {
    int i = 0;

    ASSERT(out != nullptr);
    ASSERT(count != nullptr);
    ASSERT(capacity >= 0);

    if (node == nullptr || *count >= capacity) {
        return;
    }

    for (i = 0; i < node->n; ++i) {
        if (!node->leaf) {
            export_node(node->children[i], out, capacity, count);
        }
        if (*count < capacity) {
            out[*count] = node->keys[i];
            *count += 1;
        }
    }
    if (!node->leaf) {
        export_node(node->children[node->n], out, capacity, count);
    }
}

Tree* tree_create(void) {
    Tree* tree = (Tree*) calloc(1, sizeof(Tree));

    RETURN_IF(tree == nullptr, nullptr);

    tree->root = node_create(1);
    tree->seed = 1;
    CLEANUP_AND_RETURN_IF(tree->root == nullptr, free(tree);, nullptr);

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
    Node* old_root = nullptr;
    Node* new_root = nullptr;

    RETURN_IF(tree == nullptr || tree->root == nullptr, 0);
    if (node_contains(tree->root, key)) {
        return 0;
    }

    old_root = tree->root;
    if (old_root->n == BTREE_MAX_KEYS) {
        new_root = node_create(0);
        RETURN_IF(new_root == nullptr, 0);
        tree->root             = new_root;
        new_root->children[0] = old_root;
        split_child(new_root, 0);
        insert_nonfull(new_root, key);
    } else {
        insert_nonfull(old_root, key);
    }

    tree->size += 1;
    return 1;
}

int tree_erase(Tree* tree, int key) {
    Node* old_root = nullptr;
    int removed    = 0;
    Node* node     = nullptr;

    RETURN_IF(tree == nullptr || tree->root == nullptr, 0);

    node = tree->root;
    while (node != nullptr) {
        int idx = find_key(node, key);

        if (idx < node->n && node->keys[idx] == key) {
            if (node->leaf) {
                remove_from_leaf(node, idx);
                removed = 1;
                break;
            }

            if (node->children[idx]->n >= BTREE_T) {
                int predecessor = get_predecessor(node->children[idx]);
                node->keys[idx] = predecessor;
                key             = predecessor;
                node            = node->children[idx];
            } else if (node->children[idx + 1]->n >= BTREE_T) {
                int successor = get_successor(node->children[idx + 1]);
                node->keys[idx] = successor;
                key             = successor;
                node            = node->children[idx + 1];
            } else {
                merge_children(node, idx);
                node = node->children[idx];
            }
        } else {
            int at_last_child = 0;

            if (node->leaf) {
                break;
            }

            at_last_child = (idx == node->n);
            if (node->children[idx]->n < BTREE_T) {
                fill_child(node, idx);
                if (at_last_child && idx > node->n) {
                    idx -= 1;
                }
            }
            node = node->children[idx];
        }
    }

    if (!removed) {
        return 0;
    }

    tree->size -= 1;

    if (tree->root->n == 0 && !tree->root->leaf) {
        old_root   = tree->root;
        tree->root = tree->root->children[0];
        free(old_root);
    }

    return 1;
}

int tree_contains(Tree* tree, int key) {
    return tree != nullptr && tree->root != nullptr && node_contains(tree->root, key);
}

int tree_size(Tree* tree) {
    return tree == nullptr ? 0 : tree->size;
}

int tree_validate(Tree* tree) {
    int leaf_depth = -1;
    int count = 0;

    RETURN_IF(tree == nullptr || tree->root == nullptr, 0);

    if (!validate_node(tree->root,
                       (long long) INT_MIN - 1LL,
                       (long long) INT_MAX + 1LL,
                       0,
                       1,
                       &leaf_depth,
                       &count)) {
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
    return "B-Tree";
}
