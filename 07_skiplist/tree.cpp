#include "tree.h"

#include "../asserts.h"

#include <stdint.h>
#include <stdlib.h>

#define SKIPLIST_MAX_LEVEL 32

struct Node {
    int key;
    int level;
    Node** forward;
};

struct Tree {
    Node* head;
    int level;
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

static Node* node_create(int key, int level) {
    Node* node = (Node*) calloc(1, sizeof(Node));

    RETURN_IF(node == nullptr, nullptr);
    ASSERT(level >= 0);

    node->forward = (Node**) calloc((size_t) level + 1U, sizeof(Node*));
    CLEANUP_AND_RETURN_IF(node->forward == nullptr, free(node);, nullptr);

    node->key   = key;
    node->level = level;
    return node;
}

static void node_destroy(Node* node) {
    if (node == nullptr) {
        return;
    }

    free(node->forward);
    free(node);
}

static int random_level(Tree* tree) {
    int level = 0;

    ASSERT(tree != nullptr);

    while (level < SKIPLIST_MAX_LEVEL && (next_random(&tree->seed) & 1ULL) != 0ULL) {
        level += 1;
    }

    return level;
}

Tree* tree_create(void) {
    Tree* tree = (Tree*) calloc(1, sizeof(Tree));

    RETURN_IF(tree == nullptr, nullptr);

    tree->head = node_create(0, SKIPLIST_MAX_LEVEL);
    CLEANUP_AND_RETURN_IF(tree->head == nullptr, free(tree);, nullptr);

    tree->level = 0;
    tree->seed  = 0x123456789abcdefULL;
    return tree;
}

void tree_destroy(Tree* tree) {
    Node* current = nullptr;

    if (tree == nullptr) {
        return;
    }

    current = tree->head->forward[0];
    while (current != nullptr) {
        Node* next = current->forward[0];
        node_destroy(current);
        current = next;
    }

    node_destroy(tree->head);
    free(tree);
}

void tree_set_seed(Tree* tree, uint64_t seed) {
    if (tree != nullptr) {
        tree->seed = seed == 0 ? 0x123456789abcdefULL : seed;
    }
}

int tree_insert(Tree* tree, int key) {
    Node* update[SKIPLIST_MAX_LEVEL + 1] = {};
    Node* current = nullptr;
    Node* node = nullptr;
    int level = 0;
    int i = 0;

    RETURN_IF(tree == nullptr, 0);

    current = tree->head;
    for (i = tree->level; i >= 0; --i) {
        while (current->forward[i] != nullptr && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];
    if (current != nullptr && current->key == key) {
        return 0;
    }

    level = random_level(tree);
    if (level > tree->level) {
        for (i = tree->level + 1; i <= level; ++i) {
            update[i] = tree->head;
        }
        tree->level = level;
    }

    node = node_create(key, level);
    RETURN_IF(node == nullptr, 0);

    for (i = 0; i <= level; ++i) {
        ASSERT(update[i] != nullptr);

        node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = node;
    }

    tree->size += 1;
    return 1;
}

int tree_erase(Tree* tree, int key) {
    Node* update[SKIPLIST_MAX_LEVEL + 1] = {};
    Node* current = nullptr;
    int i = 0;

    RETURN_IF(tree == nullptr, 0);

    current = tree->head;
    for (i = tree->level; i >= 0; --i) {
        while (current->forward[i] != nullptr && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];
    RETURN_IF(current == nullptr || current->key != key, 0);

    for (i = 0; i <= tree->level; ++i) {
        if (update[i]->forward[i] != current) {
            continue;
        }
        update[i]->forward[i] = current->forward[i];
    }

    node_destroy(current);
    tree->size -= 1;

    while (tree->level > 0 && tree->head->forward[tree->level] == nullptr) {
        tree->level -= 1;
    }

    return 1;
}

int tree_contains(Tree* tree, int key) {
    Node* current = nullptr;
    int i = 0;

    RETURN_IF(tree == nullptr, 0);

    current = tree->head;
    for (i = tree->level; i >= 0; --i) {
        while (current->forward[i] != nullptr && current->forward[i]->key < key) {
            current = current->forward[i];
        }
    }

    current = current->forward[0];
    return current != nullptr && current->key == key;
}

int tree_size(Tree* tree) {
    return tree == nullptr ? 0 : tree->size;
}

int tree_validate(Tree* tree) {
    int i = 0;
    int count = 0;

    RETURN_IF(tree == nullptr || tree->head == nullptr || tree->head->level != SKIPLIST_MAX_LEVEL, 0);
    if (tree->level < 0 || tree->level > SKIPLIST_MAX_LEVEL) {
        return 0;
    }

    for (i = tree->level + 1; i <= SKIPLIST_MAX_LEVEL; ++i) {
        if (tree->head->forward[i] != nullptr) {
            return 0;
        }
    }

    for (i = tree->level; i >= 0; --i) {
        Node* current = tree->head;
        while (current->forward[i] != nullptr) {
            Node* next = current->forward[i];
            if (next->level < i) {
                return 0;
            }
            if (current != tree->head && current->key >= next->key) {
                return 0;
            }
            current = next;
        }
    }

    {
        Node* current = tree->head->forward[0];
        while (current != nullptr) {
            count += 1;
            current = current->forward[0];
        }
    }

    return count == tree->size;
}

int tree_export_keys(Tree* tree, int* out, int capacity) {
    Node* current = nullptr;
    int count = 0;

    RETURN_IF(tree == nullptr || out == nullptr || capacity <= 0, 0);

    current = tree->head->forward[0];
    while (current != nullptr && count < capacity) {
        out[count] = current->key;
        count += 1;
        current = current->forward[0];
    }

    return count;
}

const char* tree_name(void) {
    return "Skip-list";
}
