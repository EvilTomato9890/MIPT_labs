#ifndef LAB_BENCH_COMMON_H_INCLUDED
#define LAB_BENCH_COMMON_H_INCLUDED

#include "lab_common.h"

struct BenchCase {
    const char* scenario;
    int n;
    int sorted;
    int repeats;
};

static int lab_fast_enabled(void) {
    const char* value = getenv("LAB_FAST");

    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

static int lab_fast_n(int n) {
    RETURN_IF(n <= 5000, n);

    if (n > 5000) {
        return 5000;
    }

    return n;
}

static int lab_run_benchmarks(const char* tree_label,
                              const struct BenchCase* cases,
                              int case_count,
                              const char* raw_path,
                              const char* summary_path) {
    FILE* raw      = nullptr;
    FILE* summary  = nullptr;
    int case_index = 0;
    int fast       = lab_fast_enabled();

    ASSERT(tree_label != nullptr);
    ASSERT(cases != nullptr);
    ASSERT(case_count > 0);
    ASSERT(raw_path != nullptr);
    ASSERT(summary_path != nullptr);

    RETURN_IF(!lab_ensure_dir("results"), 1);

    raw = fopen(raw_path, "w");
    if (raw == nullptr) {
        fprintf(stderr, "cannot open %s\n", raw_path);
        return 1;
    }

    summary = fopen(summary_path, "w");
    if (summary == nullptr) {
        fprintf(stderr, "cannot open %s\n", summary_path);
        CLEANUP_AND_RETURN_IF(1, fclose(raw);, 1);
    }

    fprintf(raw, "tree,scenario,n,repeat,insert_seconds,erase_seconds,size_after_insert,size_after_erase,valid_after_insert,valid_after_erase\n");
    fprintf(summary, "tree,scenario,n,insert_avg,erase_avg\n");

    for (case_index = 0; case_index < case_count; ++case_index) {
        int n             = fast ? lab_fast_n(cases[case_index].n) : cases[case_index].n;
        int repeats       = fast ? 1 : cases[case_index].repeats;
        double insert_sum = 0.0;
        double erase_sum  = 0.0;
        int repeat        = 0;

        ASSERT(cases[case_index].scenario != nullptr);
        ASSERT(cases[case_index].n > 0);
        ASSERT(repeats > 0);

        printf("%s: %s n=%d repeats=%d\n", tree_label, cases[case_index].scenario, n, repeats);

        for (repeat = 0; repeat < repeats; ++repeat) {
            uint64_t seed = 0x9e3779b97f4a7c15ULL ^
                            ((uint64_t) (case_index + 1) << 32) ^
                            (uint64_t) (repeat + 1);
            int* keys              = lab_make_keys(n, cases[case_index].sorted, seed);
            Tree* tree             = nullptr;
            double start           = 0.0;
            double finish          = 0.0;
            double insert_seconds  = 0.0;
            double erase_seconds   = 0.0;
            int i                  = 0;
            int size_after_insert  = 0;
            int size_after_erase   = 0;
            int valid_after_insert = 0;
            int valid_after_erase  = 0;

            CLEANUP_AND_RETURN_IF(keys == nullptr, fclose(raw); fclose(summary);, 1);

            tree = tree_create();
            if (tree == nullptr) {
                fprintf(stderr, "cannot create tree\n");
                CLEANUP_AND_RETURN_IF(1, free(keys); fclose(raw); fclose(summary);, 1);
            }

            tree_set_seed(tree, seed ^ 0xd1b54a32d192ed03ULL);

            start = lab_now_seconds();
            for (i = 0; i < n; ++i) {
                tree_insert(tree, keys[i]);
            }
            finish = lab_now_seconds();
            insert_seconds = finish - start;

            size_after_insert  = tree_size(tree);
            valid_after_insert = tree_validate(tree);
            ASSERT(size_after_insert == n);
            ASSERT(valid_after_insert);

            start = lab_now_seconds();
            for (i = 0; i < n / 2; ++i) {
                tree_erase(tree, keys[i]);
            }
            finish = lab_now_seconds();
            erase_seconds = finish - start;

            size_after_erase  = tree_size(tree);
            valid_after_erase = tree_validate(tree);
            ASSERT(size_after_erase == n - n / 2);
            ASSERT(valid_after_erase);

            fprintf(raw,
                    "%s,%s,%d,%d,%.9f,%.9f,%d,%d,%d,%d\n",
                    tree_label,
                    cases[case_index].scenario,
                    n,
                    repeat + 1,
                    insert_seconds,
                    erase_seconds,
                    size_after_insert,
                    size_after_erase,
                    valid_after_insert,
                    valid_after_erase);

            insert_sum += insert_seconds;
            erase_sum += erase_seconds;

            tree_destroy(tree);
            free(keys);
        }

        fprintf(summary,
                "%s,%s,%d,%.9f,%.9f\n",
                tree_label,
                cases[case_index].scenario,
                n,
                insert_sum / (double) repeats,
                erase_sum / (double) repeats);
        fflush(raw);
        fflush(summary);
    }

    fclose(raw);
    fclose(summary);
    return 0;
}

#endif /* LAB_BENCH_COMMON_H_INCLUDED */
