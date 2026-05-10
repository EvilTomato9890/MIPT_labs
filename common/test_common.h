#ifndef LAB_TEST_COMMON_H_INCLUDED
#define LAB_TEST_COMMON_H_INCLUDED

#include "lab_common.h"

static int lab_expect(int condition, const char* expression, const char* file, int line) {
    ASSERT(expression != nullptr);
    ASSERT(file != nullptr);
    ASSERT(line > 0);

    RETURN_IF(condition, 1);

    fprintf(stderr, "test failed: %s at %s:%d\n", expression, file, line);
    return 0;
}

#define LAB_EXPECT(expr)                                                       \
    do {                                                                       \
        if (!lab_expect((expr) != 0, #expr, __FILE__, __LINE__)) {             \
            ASSERT(expr);                                                      \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int lab_run_basic_tree_tests(const char* label) {
    int values[]        = {5, 2, 8, 1, 3, 7, 9, 4, 6, 0};
    int remove_values[] = {5, 0, 9, 4};
    int exported[4096]  = {};
    int keys[2000]      = {};
    int i               = 0;
    int count           = 0;
    Tree* tree          = nullptr;

    ASSERT(label != nullptr);

    printf("running correctness tests for %s\n", label);

    tree = tree_create();
    LAB_EXPECT(tree != nullptr);
    tree_set_seed(tree, 0x1111111111111111ULL);

    for (i = 0; i < 10; ++i) {
        LAB_EXPECT(tree_insert(tree, values[i]) == 1);
        LAB_EXPECT(tree_validate(tree) == 1);
    }

    LAB_EXPECT(tree_size(tree) == 10);
    LAB_EXPECT(tree_insert(tree, values[3]) == 0);
    LAB_EXPECT(tree_size(tree) == 10);

    for (i = 0; i < 10; ++i) {
        LAB_EXPECT(tree_contains(tree, i) == 1);
    }

    count = tree_export_keys(tree, exported, 20);
    LAB_EXPECT(count == 10);
    LAB_EXPECT(lab_is_sorted_unique(exported, count) == 1);
    for (i = 0; i < 10; ++i) {
        LAB_EXPECT(exported[i] == i);
    }

    LAB_EXPECT(tree_erase(tree, 42) == 0);
    for (i = 0; i < 4; ++i) {
        LAB_EXPECT(tree_erase(tree, remove_values[i]) == 1);
        LAB_EXPECT(tree_contains(tree, remove_values[i]) == 0);
        LAB_EXPECT(tree_validate(tree) == 1);
    }

    LAB_EXPECT(tree_size(tree) == 6);
    count = tree_export_keys(tree, exported, 20);
    LAB_EXPECT(count == 6);
    LAB_EXPECT(lab_is_sorted_unique(exported, count) == 1);

    tree_destroy(tree);

    tree = tree_create();
    LAB_EXPECT(tree != nullptr);
    tree_set_seed(tree, 0x2222222222222222ULL);
    lab_fill_sequence(keys, 2000);
    lab_shuffle_ints(keys, 2000, 0x3333333333333333ULL);

    for (i = 0; i < 2000; ++i) {
        LAB_EXPECT(tree_insert(tree, keys[i]) == 1);
        if ((i % 250) == 0) {
            LAB_EXPECT(tree_validate(tree) == 1);
        }
    }

    LAB_EXPECT(tree_size(tree) == 2000);
    LAB_EXPECT(tree_validate(tree) == 1);

    for (i = 0; i < 1000; ++i) {
        LAB_EXPECT(tree_erase(tree, keys[i]) == 1);
        if ((i % 200) == 0) {
            LAB_EXPECT(tree_validate(tree) == 1);
        }
    }

    LAB_EXPECT(tree_size(tree) == 1000);
    LAB_EXPECT(tree_validate(tree) == 1);
    count = tree_export_keys(tree, exported, 4096);
    LAB_EXPECT(count == 1000);
    LAB_EXPECT(lab_is_sorted_unique(exported, count) == 1);

    for (i = 0; i < 1000; ++i) {
        LAB_EXPECT(tree_contains(tree, keys[i]) == 0);
    }
    for (i = 1000; i < 2000; ++i) {
        LAB_EXPECT(tree_contains(tree, keys[i]) == 1);
    }

    tree_destroy(tree);

    printf("%s correctness tests passed\n", label);
    return 0;
}

#endif /* LAB_TEST_COMMON_H_INCLUDED */
