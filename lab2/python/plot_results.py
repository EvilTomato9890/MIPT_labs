from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt  # type: ignore[import-not-found]


@dataclass(frozen=True)
class PlotMeta:
    title: str
    aliases: dict[str, str]
    order: tuple[str, ...]


PLOT_META: dict[str, PlotMeta] = {
    "point1": PlotMeta(
        title="Point 1: quadratic sorts",
        aliases={
            "bubble_simple": "bubble_sort",
            "insertion_simple": "insertion_sort",
            "selection_simple": "selection_sort",
            "shell_knuth_gap": "shell_knuth_sort",
        },
        order=("bubble_simple", "insertion_simple", "selection_simple", "shell_knuth_gap"),
    ),
    "point2": PlotMeta(
        title="Point 2: k-ary heap sort",
        aliases={f"heap_k{k}_bottom_up": f"heap_k{k}" for k in range(2, 11)},
        order=tuple(f"heap_k{k}_bottom_up" for k in range(2, 11)),
    ),
    "point3": PlotMeta(
        title="Point 3: merge sort variants",
        aliases={
            "merge_recursive_top_down": "merge_recursive",
            "merge_iterative_bottom_up": "merge_iterative",
        },
        order=("merge_recursive_top_down", "merge_iterative_bottom_up"),
    ),
    "point4": PlotMeta(
        title="Point 4: quicksort partitions",
        aliases={
            "quick_lomuto_partition": "quick_lomuto",
            "quick_hoare_partition": "quick_hoare",
            "quick_three_way_partition": "quick_three_way",
        },
        order=("quick_lomuto_partition", "quick_hoare_partition", "quick_three_way_partition"),
    ),
    "point4_dup": PlotMeta(
        title="Point 4: duplicates",
        aliases={
            "quick_lomuto_partition": "quick_lomuto",
            "quick_hoare_partition": "quick_hoare",
            "quick_three_way_partition": "quick_three_way",
        },
        order=("quick_lomuto_partition", "quick_hoare_partition", "quick_three_way_partition"),
    ),
    "point4_opt": PlotMeta(
        title="Point 4: quicksort recursion strategies",
        aliases={
            "quick_median3_recursive": "quick_recursive",
            "quick_median3_tailrec": "quick_tailrec",
            "quick_median3_iterative": "quick_iterative",
        },
        order=("quick_median3_recursive", "quick_median3_tailrec", "quick_median3_iterative"),
    ),
    "point4_opt_dup": PlotMeta(
        title="Point 4: recursion strategies on duplicates",
        aliases={
            "quick_median3_recursive": "quick_recursive",
            "quick_median3_tailrec": "quick_tailrec",
            "quick_median3_iterative": "quick_iterative",
        },
        order=("quick_median3_recursive", "quick_median3_tailrec", "quick_median3_iterative"),
    ),
    "point5": PlotMeta(
        title="Point 5: pivot strategies",
        aliases={
            "quick_3way_pivot_center": "pivot_center",
            "quick_3way_pivot_median3": "pivot_median3",
            "quick_3way_pivot_random": "pivot_random",
            "quick_3way_pivot_median3_random": "pivot_median3_random",
        },
        order=(
            "quick_3way_pivot_center",
            "quick_3way_pivot_median3",
            "quick_3way_pivot_random",
            "quick_3way_pivot_median3_random",
        ),
    ),
    "point6": PlotMeta(
        title="Point 6: introsort threshold tuning",
        aliases={
            "quick_3way_pivot_median3_baseline": "quick_baseline",
            "introsort_threshold16": "introsort_t16",
            "introsort_threshold32": "introsort_t32",
            "introsort_threshold64": "introsort_t64",
        },
        order=(
            "quick_3way_pivot_median3_baseline",
            "introsort_threshold16",
            "introsort_threshold32",
            "introsort_threshold64",
        ),
    ),
    "point7_c_scan": PlotMeta(
        title="Point 7: introsort depth coefficient",
        aliases={
            "introsort_c125": "introsort_c=1.25",
            "introsort_c150": "introsort_c=1.50",
            "introsort_c200": "introsort_c=2.00",
            "introsort_c250": "introsort_c=2.50",
            "introsort_c300": "introsort_c=3.00",
        },
        order=("introsort_c125", "introsort_c150", "introsort_c200", "introsort_c250", "introsort_c300"),
    ),
    "point7": PlotMeta(
        title="Point 7: introsort vs quicksort",
        aliases={
            "quick_3way_pivot_median3_baseline": "quick_baseline",
            "introsort_best_config": "introsort",
        },
        order=("quick_3way_pivot_median3_baseline", "introsort_best_config"),
    ),
    "point8": PlotMeta(
        title="Point 8: timsort and pdqsort",
        aliases={
            "quick_3way_pivot_median3_baseline": "quick_baseline",
            "introsort_best_config": "introsort",
            "timsort_hybrid_runs": "timsort",
            "pdqsort_pattern_defeating": "pdqsort",
        },
        order=(
            "quick_3way_pivot_median3_baseline",
            "introsort_best_config",
            "timsort_hybrid_runs",
            "pdqsort_pattern_defeating",
        ),
    ),
    "point9": PlotMeta(
        title="Point 9: radix sorts",
        aliases={
            "radix_lsd_bytewise": "radix_lsd",
            "radix_msd_bytewise": "radix_msd",
        },
        order=("radix_lsd_bytewise", "radix_msd_bytewise"),
    ),
    "point10": PlotMeta(
        title="Point 10: best algorithms vs qsort",
        aliases={
            "shell_knuth_best": "shell_knuth",
            "heap_k4_bottom_up_best": "heap_k4",
            "merge_iterative_best": "merge_iterative",
            "quick_3way_pivot_median3_best": "quick_best",
            "introsort_best_config": "introsort",
            "pdqsort_pattern_defeating": "pdqsort",
            "radix_lsd_bytewise": "radix_lsd",
            "timsort_hybrid_runs": "timsort",
            "qsort_stdlib_reference": "qsort",
        },
        order=(
            "shell_knuth_best",
            "heap_k4_bottom_up_best",
            "merge_iterative_best",
            "quick_3way_pivot_median3_best",
            "introsort_best_config",
            "pdqsort_pattern_defeating",
            "radix_lsd_bytewise",
            "timsort_hybrid_runs",
            "qsort_stdlib_reference",
        ),
    ),
}


def parse_csv(path: Path) -> dict[str, list[tuple[int, float]]]:
    result: dict[str, list[tuple[int, float]]] = defaultdict(list)
    with path.open("r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            name = row["algorithm"]
            size = int(row["size"])
            secs = float(row["seconds"])
            result[name].append((size, secs))
    for points in result.values():
        points.sort(key=lambda x: x[0])
    return dict(result)


def draw_plot(data: dict[str, list[tuple[int, float]]], target: str, out_path: Path) -> None:
    meta = PLOT_META[target]

    plt.style.use("default")
    fig, ax = plt.subplots(figsize=(14, 8), dpi=150)
    fig.patch.set_facecolor("white")
    ax.set_facecolor("white")

    ordered_names = [name for name in meta.order if name in data]
    ordered_names.extend(name for name in data if name not in ordered_names)

    for raw_name in ordered_names:
        points = data[raw_name]
        x_vals = [p[0] for p in points]
        y_vals = [p[1] for p in points]
        display_name = meta.aliases.get(raw_name, raw_name)
        ax.plot(
            x_vals,
            y_vals,
            label=display_name,
            marker="o",
            linewidth=2.2,
            markersize=4.8,
        )

    ax.set_title(meta.title, fontsize=18, pad=12)
    ax.set_xlabel("Array size", fontsize=14)
    ax.set_ylabel("Average seconds", fontsize=14)
    ax.tick_params(labelsize=11)
    ax.grid(True, linestyle="--", linewidth=0.9, alpha=0.45, color="#b8c2cc")
    ax.legend(
        loc="upper left",
        frameon=True,
        facecolor="white",
        edgecolor="#cfcfcf",
        framealpha=0.95,
        fontsize=12,
    )
    ax.margins(x=0.02, y=0.05)

    for spine in ax.spines.values():
        spine.set_linewidth(1.0)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", type=str, required=True)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    if args.target not in PLOT_META:
        available = ", ".join(sorted(PLOT_META))
        raise SystemExit(f"unknown target '{args.target}', expected one of: {available}")

    data = parse_csv(args.input)
    draw_plot(data, args.target, args.output)


if __name__ == "__main__":
    main()
