#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 5 ]]; then
  echo "usage: run_point.sh <point:p1..p10> <tests_root_dir> <csv_out_prefix> <limit> <seed>"
  exit 1
fi

point="$1"
tests_root="$2"
csv_prefix="$3"
limit="$4"
seed="$5"

small_dir="$tests_root/small_tests"
big_dir="$tests_root/big_tests"
dup_dir="$tests_root/test_most_dublicates"

mkdir -p "$tests_root"

"$(dirname "$0")/gen_tests.sh" "$small_dir" 0 1000 50 5 "$limit" \
  lab3/bin/gen_random lab3/bin/qsort_solver "$seed"
"$(dirname "$0")/gen_tests.sh" "$big_dir" 0 1000000 10000 5 "$limit" \
  lab3/bin/gen_random lab3/bin/qsort_solver "$seed"
"$(dirname "$0")/gen_tests.sh" "$dup_dir" 0 1000000 10000 5 10000 \
  lab3/bin/gen_random lab3/bin/qsort_solver "$seed"

case "$point" in
  p1)
    lab3/bin/tester p1 "$small_dir" "${csv_prefix}_small.csv" 0 1000 50 5
    ;;
  p2|p3|p5|p6|p7|p8|p9|p10)
    lab3/bin/tester "$point" "$big_dir" "${csv_prefix}_big.csv" 0 1000000 10000 5
    ;;
  p4)
    lab3/bin/tester p4 "$big_dir" "${csv_prefix}_big.csv" 0 1000000 10000 5
    lab3/bin/tester p4 "$dup_dir" "${csv_prefix}_dup.csv" 0 1000000 10000 5
    ;;
  *)
    echo "unknown point: $point"
    exit 2
    ;;
esac
