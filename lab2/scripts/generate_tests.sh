#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 || $# -gt 4 ]]; then
  echo "usage: generate_tests.sh <tests_root_dir> <limit> <seed> [group:all|small|big|dup]"
  exit 1
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
source "$script_dir/common.sh"
lab_root="$(resolve_lab_root "$script_dir")"

guard_project_relative_path "$lab_root" "$1" "tests root"
tests_root="$(normalize_path "$1")"
limit="$2"
seed="$3"
group="${4:-all}"

gen_tests_helper="$script_dir/gen_tests.sh"
gen_bin="$(resolve_binary "$lab_root" gen_random)"
qsort_bin="$(resolve_binary "$lab_root" qsort_solver)"

mkdir -p "$tests_root"

generate_group() {
  local dataset="$1"
  case "$dataset" in
    small)
      bash "$gen_tests_helper" "$tests_root/small_tests" \
        "$LAB2_SMALL_FROM" "$LAB2_SMALL_TO" "$LAB2_SMALL_STEP" "$LAB2_SMALL_COPIES" \
        "$limit" "$gen_bin" "$qsort_bin" "$seed"
      ;;
    big)
      bash "$gen_tests_helper" "$tests_root/big_tests" \
        "$LAB2_BIG_FROM" "$LAB2_BIG_TO" "$LAB2_BIG_STEP" "$LAB2_BIG_COPIES" \
        "$limit" "$gen_bin" "$qsort_bin" "$seed"
      ;;
    dup)
      bash "$gen_tests_helper" "$tests_root/test_most_dublicates" \
        "$LAB2_DUP_FROM" "$LAB2_DUP_TO" "$LAB2_DUP_STEP" "$LAB2_DUP_COPIES" \
        "$LAB2_DUP_LIMIT" "$gen_bin" "$qsort_bin" "$seed"
      ;;
    *)
      die "unknown group '$dataset'; expected one of: all, small, big, dup"
      ;;
  esac
}

case "$group" in
  all)
    generate_group small
    generate_group big
    generate_group dup
    ;;
  small|big|dup)
    generate_group "$group"
    ;;
  *)
    die "unknown group '$group'; expected one of: all, small, big, dup"
    ;;
esac
