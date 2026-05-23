#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 || $# -gt 4 ]]; then
  echo "usage: generate_tests.sh <tests_root_dir> <limit> <seed> [group:all|build]"
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
    build)
      bash "$gen_tests_helper" "$tests_root/heap_build_tests" \
        "$LAB4_BUILD_FROM" "$LAB4_BUILD_TO" "$LAB4_BUILD_STEP" "$LAB4_BUILD_COPIES" \
        "$limit" "$gen_bin" "$qsort_bin" "$seed"
      ;;
    *)
      die "unknown group '$dataset'; expected one of: all, build"
      ;;
  esac
}

case "$group" in
  all)
    generate_group build
    ;;
  build)
    generate_group build
    ;;
  *)
    die "unknown group '$group'; expected one of: all, build"
    ;;
esac
