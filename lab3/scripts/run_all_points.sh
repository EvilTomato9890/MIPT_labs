#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: run_all_points.sh <tests_root_dir> <results_root_dir>"
  exit 1
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
source "$script_dir/common.sh"
lab_root="$(resolve_lab_root "$script_dir")"

guard_project_relative_path "$lab_root" "$1" "tests root"
guard_project_relative_path "$lab_root" "$2" "results root"
tests_root="$(normalize_path "$1")"
results_root="$(normalize_path "$2")"

points=(p1 p2 p3_sparse p3_dense)
total_points="${#points[@]}"
index=0

for point in "${points[@]}"; do
  index=$((index + 1))
  printf '[run_all_points] (%d/%d) %s\n' "$index" "$total_points" "$point"
  bash "$script_dir/run_point.sh" "$point" "$tests_root" "$results_root"
done
