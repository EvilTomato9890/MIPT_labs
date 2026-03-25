#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 9 ]]; then
  echo "usage: gen_tests.sh <dir> <from> <to> <step> <copies> <limit> <gen_bin> <qsort_bin> <seed>"
  exit 1
fi

out_dir="$1"
from="$2"
to="$3"
step="$4"
copies="$5"
limit="$6"
gen_bin="$7"
qsort_bin="$8"
seed="$9"

mkdir -p "$out_dir"
RANDOM="$seed"

for ((n=from; n<=to; n+=step)); do
  for ((k=0; k<copies; ++k)); do
    in_file="$out_dir/${n}_${k}.in"
    out_file="$out_dir/${n}_${k}.out"
    "$gen_bin" "$n" "$limit" > "$in_file"
    "$qsort_bin" < "$in_file" > "$out_file"
  done
done
