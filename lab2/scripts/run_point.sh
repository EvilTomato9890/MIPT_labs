#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: run_point.sh <point:p1..p10> <tests_root_dir> <results_root_dir>"
  exit 1
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
source "$script_dir/common.sh"
lab_root="$(resolve_lab_root "$script_dir")"

point="$1"
guard_project_relative_path "$lab_root" "$2" "tests root"
guard_project_relative_path "$lab_root" "$3" "results root"
tests_root="$2"
results_root="$3"

tests_root="$(normalize_path "$tests_root")"
results_root="$(normalize_path "$results_root")"
csv_dir="$results_root/csv"
plots_dir="$results_root/plots"
tester_bin="$(resolve_binary "$lab_root" tester)"
plot_script="$script_dir/plot_results.sh"

mkdir -p "$csv_dir" "$plots_dir"

ensure_dataset_dir() {
  local dataset_dir="$1"
  local group_name="$2"
  [[ -d "$dataset_dir" ]] || die "missing tests directory '$dataset_dir'; generate it first with: bash \"$script_dir/generate_tests.sh\" \"$tests_root\" $LAB2_DEFAULT_LIMIT $LAB2_DEFAULT_SEED $group_name"
}

resolve_duplicates_dir() {
  local legacy_dir="$tests_root/test_most_dublicates"
  local modern_dir="$tests_root/test_most_duplicates"

  if [[ -d "$legacy_dir" ]]; then
    printf '%s\n' "$legacy_dir"
    return 0
  fi
  if [[ -d "$modern_dir" ]]; then
    printf '%s\n' "$modern_dir"
    return 0
  fi

  printf '%s\n' "$legacy_dir"
}

validate_dataset_files() {
  local dataset_dir="$1"
  local group_name="$2"
  local from="$3"
  local to="$4"
  local step="$5"
  local copies="$6"
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
      printf "tests directory '%s' is incomplete or corrupted for group '%s'\n" "$dataset_dir" "$group_name"
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
      printf "repair the dataset before running benchmarks; helper: bash \"%s/generate_tests.sh\" \"%s\" %s %s %s\n" \
        "$script_dir" "$tests_root" "$LAB2_DEFAULT_LIMIT" "$LAB2_DEFAULT_SEED" "$group_name"
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
  local group_name="$9"

  local csv_path="$csv_dir/$csv_name"
  local tester_tests_dir="$dataset_dir"
  local tester_csv_path="$csv_path"
  validate_dataset_files "$dataset_dir" "$group_name" "$from" "$to" "$step" "$copies"
  rm -f "$csv_path"

  if [[ "$tester_bin" == *.exe ]]; then
    tester_tests_dir="$(native_path "$dataset_dir")"
    tester_csv_path="$(native_path "$csv_path")"
  fi

  printf '[run_point] %s -> %s using %s\n' "$point_name" "$csv_name" "$dataset_dir"
  "$tester_bin" "$point_name" "$tester_tests_dir" "$tester_csv_path" "$from" "$to" "$step" "$copies"
  bash "$plot_script" "$plot_target" "$csv_dir" "$plots_dir"
}

case "$point" in
  p1)
    ensure_dataset_dir "$tests_root/small_tests" small
    run_dataset p1 "$tests_root/small_tests" point1.csv point1 \
      "$LAB2_SMALL_FROM" "$LAB2_SMALL_TO" "$LAB2_SMALL_STEP" "$LAB2_SMALL_COPIES" small
    ;;
  p2|p3|p5|p6|p8|p9|p10)
    ensure_dataset_dir "$tests_root/big_tests" big
    run_dataset "$point" "$tests_root/big_tests" "point${point#p}.csv" "point${point#p}" \
      "$LAB2_BIG_FROM" "$LAB2_BIG_TO" "$LAB2_BIG_STEP" "$LAB2_BIG_COPIES" big
    ;;
  p4)
    ensure_dataset_dir "$tests_root/big_tests" big
    dup_tests_dir="$(resolve_duplicates_dir)"
    ensure_dataset_dir "$dup_tests_dir" dup
    run_dataset p4 "$tests_root/big_tests" point4.csv point4 \
      "$LAB2_BIG_FROM" "$LAB2_BIG_TO" "$LAB2_BIG_STEP" "$LAB2_BIG_COPIES" big
    run_dataset p4opt "$tests_root/big_tests" point4_opt.csv point4_opt \
      "$LAB2_BIG_FROM" "$LAB2_BIG_TO" "$LAB2_BIG_STEP" "$LAB2_BIG_COPIES" big
    run_dataset p4 "$dup_tests_dir" point4_dup.csv point4_dup \
      "$LAB2_DUP_FROM" "$LAB2_DUP_TO" "$LAB2_DUP_STEP" "$LAB2_DUP_COPIES" dup
    run_dataset p4opt "$dup_tests_dir" point4_opt_dup.csv point4_opt_dup \
      "$LAB2_DUP_FROM" "$LAB2_DUP_TO" "$LAB2_DUP_STEP" "$LAB2_DUP_COPIES" dup
    ;;
  p7)
    ensure_dataset_dir "$tests_root/big_tests" big
    run_dataset p7scan "$tests_root/big_tests" point7_c_scan.csv point7_c_scan \
      "$LAB2_BIG_FROM" "$LAB2_BIG_TO" "$LAB2_BIG_STEP" "$LAB2_BIG_COPIES" big
    run_dataset p7 "$tests_root/big_tests" point7.csv point7 \
      "$LAB2_BIG_FROM" "$LAB2_BIG_TO" "$LAB2_BIG_STEP" "$LAB2_BIG_COPIES" big
    ;;
  *)
    die "unknown point '$point'; expected p1..p10"
    ;;
esac
