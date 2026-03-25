#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 5 ]]; then
  echo "usage: run_point.sh <point:p1|p2|p3> <tests_dir> <csv_out> <limit> <seed>"
  exit 1
fi

point="$1"
tests_dir="$2"
csv_out="$3"
limit="$4"
seed="$5"

case "$point" in
  p1)
    from=0; to=10000; step=100; copies=100
    ;;
  p2)
    from=0; to=10000000; step=100000; copies=100
    ;;
  p3)
    from=0; to=10000000; step=100000; copies=100
    ;;
  *)
    echo "unknown point: $point"
    exit 2
    ;;
esac

"$(dirname "$0")/gen_tests.sh" "$tests_dir" "$from" "$to" "$step" "$copies" "$limit" \
  lab3/bin/gen_random lab3/bin/qsort_solver "$seed"
lab3/bin/tester "$point" "$tests_dir" "$csv_out" "$from" "$to" "$step" "$copies"
