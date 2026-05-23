#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: run_point.sh <point:p1|p2|p3_sparse|p3_dense> <tests_root_dir> <results_root_dir>"
  exit 1
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
source "$script_dir/common.sh"
lab_root="$(resolve_lab_root "$script_dir")"

point="$1"
guard_project_relative_path "$lab_root" "$2" "tests root"
guard_project_relative_path "$lab_root" "$3" "results root"
tests_root="$(normalize_path "$2")"
results_root="$(normalize_path "$3")"

csv_dir="$results_root/csv"
plots_dir="$results_root/plots"
tester_bin="$(resolve_binary "$lab_root" tester)"
plot_script="$script_dir/plot_results.sh"

mkdir -p "$csv_dir" "$plots_dir"

validate_dataset_files() {
  local dataset_dir="$1"
  local from="$2"
  local to="$3"
  local step="$4"
  local copies="$5"
  local expected_files=$(( ((to - from) / step + 1) * copies * 2 ))
  local actual_files=0
  local -a entries=()
  local -a missing_examples=()
  local -a empty_examples=()

  shopt -s nullglob
  entries=( "$dataset_dir"/* )
  shopt -u nullglob
  for path in "${entries[@]}"; do
    [[ -f "$path" ]] && actual_files=$((actual_files + 1))
  done

  for ((n=from; n<=to; n+=step)); do
    for ((k=0; k<copies; ++k)); do
      for ext in in out; do
        path="$dataset_dir/${n}_${k}.$ext"
        if [[ ! -f "$path" ]]; then
          if (( ${#missing_examples[@]} < 8 )); then
            missing_examples+=( "$path" )
          fi
        elif [[ ! -s "$path" ]]; then
          if (( ${#empty_examples[@]} < 8 )); then
            empty_examples+=( "$path" )
          fi
        fi
      done
    done
  done

  if (( ${#missing_examples[@]} > 0 || ${#empty_examples[@]} > 0 )); then
    {
      printf "tests directory '%s' is incomplete or corrupted\n" "$dataset_dir"
      printf "expected %d files (.in/.out), found %d\n" "$expected_files" "$actual_files"
      if (( ${#missing_examples[@]} > 0 )); then
        printf "missing examples:\n"
        for path in "${missing_examples[@]}"; do
          printf "  %s\n" "$path"
        done
      fi
      if (( ${#empty_examples[@]} > 0 )); then
        printf "empty examples:\n"
        for path in "${empty_examples[@]}"; do
          printf "  %s\n" "$path"
        done
      fi
      printf "repair the dataset first with: bash \"%s/generate_tests.sh\" \"%s\" %s %s all\n" \
        "$script_dir" "$tests_root" "$LAB4_DEFAULT_LIMIT" "$LAB4_DEFAULT_SEED"
    } >&2
    exit 1
  fi
}

run_dataset() {
  local point_name="$1"
  local dataset_dir="$2"
  local csv_name="$3"
  local plot_target="$4"
  local from="$5"
  local to="$6"
  local step="$7"
  local copies="$8"

  local csv_path="$csv_dir/$csv_name"
  local tester_tests_dir="$dataset_dir"
  local tester_csv_path="$csv_path"

  validate_dataset_files "$dataset_dir" "$from" "$to" "$step" "$copies"
  rm -f "$csv_path"

  if [[ "$tester_bin" == *.exe ]]; then
    tester_tests_dir="$(native_path "$dataset_dir")"
    tester_csv_path="$(native_path "$csv_path")"
  fi

  printf '[run_point] %s -> %s using %s\n' "$point_name" "$csv_name" "$dataset_dir"
  "$tester_bin" "$point_name" "$tester_tests_dir" "$tester_csv_path" "$from" "$to" "$step" "$copies"
  bash "$plot_script" "$plot_target" "$csv_dir" "$plots_dir"
}

run_generated_graph_point() {
  local point_name="$1"
  local csv_name="$2"
  local plot_target="$3"
  local from="$4"
  local to="$5"
  local step="$6"
  local copies="$7"

  local csv_path="$csv_dir/$csv_name"
  local tester_csv_path="$csv_path"
  local tester_placeholder="$tests_root"

  rm -f "$csv_path"

  if [[ "$tester_bin" == *.exe ]]; then
    tester_placeholder="$(native_path "$tests_root")"
    tester_csv_path="$(native_path "$csv_path")"
  fi

  printf '[run_point] %s -> %s using in-memory graph generator\n' "$point_name" "$csv_name"
  "$tester_bin" "$point_name" "$tester_placeholder" "$tester_csv_path" "$from" "$to" "$step" "$copies"
  bash "$plot_script" "$plot_target" "$csv_dir" "$plots_dir"
}

case "$point" in
  p1)
    dataset_dir="$tests_root/heap_build_tests"
    [[ -d "$dataset_dir" ]] || die "missing tests directory '$dataset_dir'; generate it first with: bash \"$script_dir/generate_tests.sh\" \"$tests_root\" $LAB4_DEFAULT_LIMIT $LAB4_DEFAULT_SEED all"
    run_dataset p1 "$dataset_dir" point1.csv point1 \
      "$LAB4_BUILD_FROM" "$LAB4_BUILD_TO" "$LAB4_BUILD_STEP" "$LAB4_BUILD_COPIES"
    ;;
  p2)
    dataset_dir="$tests_root/heap_build_tests"
    [[ -d "$dataset_dir" ]] || die "missing tests directory '$dataset_dir'; generate it first with: bash \"$script_dir/generate_tests.sh\" \"$tests_root\" $LAB4_DEFAULT_LIMIT $LAB4_DEFAULT_SEED all"
    run_dataset p2 "$dataset_dir" point2.csv point2 \
      "$LAB4_BUILD_FROM" "$LAB4_BUILD_TO" "$LAB4_BUILD_STEP" "$LAB4_BUILD_COPIES"
    ;;
  p3_sparse)
    run_generated_graph_point p3_sparse point3_sparse.csv point3_sparse \
      "$LAB4_DIJKSTRA_FROM" "$LAB4_DIJKSTRA_TO" "$LAB4_DIJKSTRA_STEP" "$LAB4_DIJKSTRA_COPIES"
    ;;
  p3_dense)
    run_generated_graph_point p3_dense point3_dense.csv point3_dense \
      "$LAB4_DIJKSTRA_FROM" "$LAB4_DIJKSTRA_TO" "$LAB4_DIJKSTRA_STEP" "$LAB4_DIJKSTRA_COPIES"
    ;;
  *)
    die "unknown point '$point'; expected p1, p2, p3_sparse, or p3_dense"
    ;;
esac
