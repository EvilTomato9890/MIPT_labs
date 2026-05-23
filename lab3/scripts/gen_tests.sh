#!/usr/bin/env bash
set -euo pipefail

readonly GEN_TESTS_USAGE_ARGC=9
readonly GEN_TESTS_INVALID_STEP_EXIT_CODE=2
readonly GEN_TESTS_PROGRESS_BAR_WIDTH=28

if [[ $# -ne "$GEN_TESTS_USAGE_ARGC" ]]; then
  echo "usage: gen_tests.sh <dir> <from> <to> <step> <copies> <limit> <gen_bin> <qsort_bin> <seed>"
  exit 1
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
source "$script_dir/common.sh"

render_progress() {
  local label="$1"
  local current="$2"
  local total="$3"
  local width="$GEN_TESTS_PROGRESS_BAR_WIDTH"
  local filled=$(( current * width / total ))
  local percent=$(( current * 100 / total ))

  printf '\r[' >&2
  for ((i=0; i<width; ++i)); do
    if (( i < filled )); then
      printf '#' >&2
    else
      printf '-' >&2
    fi
  done
  printf '] %3d%%  %-20s %d/%d' "$percent" "$label" "$current" "$total" >&2
}

out_dir="$1"
from="$2"
to="$3"
step="$4"
copies="$5"
limit="$6"
gen_bin="$7"
qsort_bin="$8"
seed="$9"

out_dir="$(normalize_path "$out_dir")"
gen_bin="$(normalize_path "$gen_bin")"
qsort_bin="$(normalize_path "$qsort_bin")"

if (( step <= 0 )); then
  echo "gen_tests.sh: step must be > 0" >&2
  exit "$GEN_TESTS_INVALID_STEP_EXIT_CODE"
fi

mkdir -p "$out_dir"
total_jobs=$(( ((to - from) / step + 1) * copies ))
completed=0
label="$(basename "$out_dir")"
current_in_tmp=""
current_out_tmp=""

cleanup_partial() {
  [[ -n "$current_in_tmp" ]] && rm -f -- "$current_in_tmp"
  [[ -n "$current_out_tmp" ]] && rm -f -- "$current_out_tmp"
}

trap cleanup_partial EXIT INT TERM

for ((n=from; n<=to; n+=step)); do
  for ((k=0; k<copies; ++k)); do
    in_file="$out_dir/${n}_${k}.in"
    out_file="$out_dir/${n}_${k}.out"
    current_in_tmp="$in_file.part"
    current_out_tmp="$out_file.part"
    rm -f "$current_in_tmp" "$current_out_tmp"
    current_seed=$((seed + ((n - from) / step) * copies + k))
    "$gen_bin" "$n" "$limit" "$current_seed" > "$current_in_tmp"
    "$qsort_bin" < "$current_in_tmp" > "$current_out_tmp"
    mv -f "$current_in_tmp" "$in_file"
    mv -f "$current_out_tmp" "$out_file"
    current_in_tmp=""
    current_out_tmp=""
    completed=$((completed + 1))
    render_progress "$label" "$completed" "$total_jobs"
  done
done

if (( total_jobs > 0 )); then
  printf '\n' >&2
fi

trap - EXIT INT TERM
