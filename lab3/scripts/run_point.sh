#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 5 ]]; then
  echo "usage: run_point.sh <point:p8|p9|p10> <tests_dir> <csv_out> <limit> <seed>"
  exit 1
fi

point="$1"
tests_dir="$2"
csv_out="$3"
limit="$4"
seed="$5"

case "$point" in
  p8)
    from=0; to=1000000; step=10000; copies=5
    ;;
  p9)
    from=0; to=1000000; step=10000; copies=5
    ;;
  p10)
    from=0; to=1000000; step=10000; copies=5
    ;;
  *)
    echo "unknown point: $point"
    exit 2
    ;;
esac

"$(dirname "$0")/gen_tests.sh" "$tests_dir" "$from" "$to" "$step" "$copies" "$limit" \
  lab3/bin/gen_random lab3/bin/qsort_solver "$seed"
lab3/bin/tester "$point" "$tests_dir" "$csv_out" "$from" "$to" "$step" "$copies"
