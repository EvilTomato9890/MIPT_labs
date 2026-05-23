#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: plot_results.sh <target:all|point1|point2|point3_sparse|point3_dense> <csv_root_dir> <plots_root_dir>"
  exit 1
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
source "$script_dir/common.sh"
lab_root="$(resolve_lab_root "$script_dir")"

target="$1"
guard_project_relative_path "$lab_root" "$2" "csv root"
guard_project_relative_path "$lab_root" "$3" "plots root"
csv_root="$(normalize_path "$2")"
plots_root="$(normalize_path "$3")"
python_bin="$(resolve_python_bin)"
plotter="$lab_root/python/plot_results.py"

mkdir -p "$plots_root"

plot_one() {
  local point_target="$1"
  local csv_path="$csv_root/${point_target}.csv"
  local plot_path="$plots_root/${point_target}.png"

  [[ -f "$csv_path" ]] || die "missing csv '$csv_path'; run the corresponding benchmark first"
  "$python_bin" "$plotter" --target "$point_target" --input "$csv_path" --output "$plot_path"
}

if [[ "$target" == "all" ]]; then
  for point_target in point1 point2 point3_sparse point3_dense; do
    [[ -f "$csv_root/${point_target}.csv" ]] || continue
    plot_one "$point_target"
  done
  exit 0
fi

case "$target" in
  point1|point2|point3_sparse|point3_dense)
    plot_one "$target"
    ;;
  *)
    die "unknown target '$target'; expected all, point1, point2, point3_sparse, or point3_dense"
    ;;
esac
