#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: run_all_points.sh <tests_root_dir> <results_root_dir> <limit> <seed>"
  exit 1
fi

tests_root="$1"
results_root="$2"
limit="$3"
seed="$4"

small_dir="$tests_root/small_tests"
big_dir="$tests_root/big_tests"
dup_dir="$tests_root/test_most_dublicates"

mkdir -p "$tests_root" "$results_root"

"$(dirname "$0")/gen_tests.sh" "$small_dir" 0 1000 50 5 "$limit" \
  lab3/bin/gen_random lab3/bin/qsort_solver "$seed"
"$(dirname "$0")/gen_tests.sh" "$big_dir" 0 1000000 10000 5 "$limit" \
  lab3/bin/gen_random lab3/bin/qsort_solver "$seed"
"$(dirname "$0")/gen_tests.sh" "$dup_dir" 0 1000000 10000 5 10000 \
  lab3/bin/gen_random lab3/bin/qsort_solver "$seed"

lab3/bin/tester p1 "$small_dir" "$results_root/p1_small.csv" 0 1000 50 5
lab3/bin/tester p2 "$big_dir" "$results_root/p2_big.csv" 0 1000000 10000 5
lab3/bin/tester p3 "$big_dir" "$results_root/p3_big.csv" 0 1000000 10000 5
lab3/bin/tester p4 "$big_dir" "$results_root/p4_big.csv" 0 1000000 10000 5
lab3/bin/tester p4 "$dup_dir" "$results_root/p4_dup.csv" 0 1000000 10000 5
lab3/bin/tester p5 "$big_dir" "$results_root/p5_big.csv" 0 1000000 10000 5
lab3/bin/tester p6 "$big_dir" "$results_root/p6_big.csv" 0 1000000 10000 5
lab3/bin/tester p7 "$big_dir" "$results_root/p7_big.csv" 0 1000000 10000 5
lab3/bin/tester p8 "$big_dir" "$results_root/p8_big.csv" 0 1000000 10000 5
lab3/bin/tester p9 "$big_dir" "$results_root/p9_big.csv" 0 1000000 10000 5
lab3/bin/tester p10 "$big_dir" "$results_root/p10_big.csv" 0 1000000 10000 5
