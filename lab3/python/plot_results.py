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
        title="Binary heap build: linear heapify vs inserts",
        aliases={
            "binary_heap_linear": "linear_heapify",
            "binary_heap_inserts": "repeated_inserts",
        },
        order=("binary_heap_linear", "binary_heap_inserts"),
    ),
    "point2": PlotMeta(
        title="Binomial heap build by repeated inserts",
        aliases={
            "binomial_heap_inserts": "binomial_inserts",
        },
        order=("binomial_heap_inserts",),
    ),
    "point3_sparse": PlotMeta(
        title="Dijkstra on sparse graphs",
        aliases={
            "dijkstra_naive": "naive_scan",
            "dijkstra_binary_heap": "binary_heap",
            "dijkstra_binomial_heap": "binomial_heap",
            "dijkstra_fibonacci_heap": "fibonacci_heap",
        },
        order=(
            "dijkstra_naive",
            "dijkstra_binary_heap",
            "dijkstra_binomial_heap",
            "dijkstra_fibonacci_heap",
        ),
    ),
    "point3_dense": PlotMeta(
        title="Dijkstra on dense graphs",
        aliases={
            "dijkstra_naive": "naive_scan",
            "dijkstra_binary_heap": "binary_heap",
            "dijkstra_binomial_heap": "binomial_heap",
            "dijkstra_fibonacci_heap": "fibonacci_heap",
        },
        order=(
            "dijkstra_naive",
            "dijkstra_binary_heap",
            "dijkstra_binomial_heap",
            "dijkstra_fibonacci_heap",
        ),
    ),
}


def parse_csv(path: Path) -> dict[str, list[tuple[int, float]]]:
    result: dict[str, list[tuple[int, float]]] = defaultdict(list)
    with path.open("r", encoding="utf-8", newline="") as file:
        reader = csv.DictReader(file)
        for row in reader:
            result[row["algorithm"]].append((int(row["size"]), float(row["seconds"])))

    for points in result.values():
        points.sort(key=lambda pair: pair[0])
    return dict(result)


def draw_plot(data: dict[str, list[tuple[int, float]]], target: str, out_path: Path) -> None:
    meta = PLOT_META[target]

    plt.style.use("default")
    figure, axis = plt.subplots(figsize=(13, 7), dpi=150)
    figure.patch.set_facecolor("white")
    axis.set_facecolor("white")

    ordered_names = [name for name in meta.order if name in data]
    ordered_names.extend(name for name in data if name not in ordered_names)

    for raw_name in ordered_names:
        points = data[raw_name]
        x_values = [pair[0] for pair in points]
        y_values = [pair[1] for pair in points]
        display_name = meta.aliases.get(raw_name, raw_name)
        axis.plot(x_values, y_values, label=display_name, marker="o", linewidth=2.2, markersize=5.0)

    axis.set_title(meta.title, fontsize=18, pad=12)
    axis.set_xlabel("Input size", fontsize=14)
    ylabel = "Average build time, seconds" if target in {"point1", "point2"} else "Average runtime, seconds"
    axis.set_ylabel(ylabel, fontsize=14)
    axis.tick_params(labelsize=11)
    axis.grid(True, linestyle="--", linewidth=0.9, alpha=0.45, color="#b8c2cc")
    axis.legend(
        loc="upper left",
        frameon=True,
        facecolor="white",
        edgecolor="#cfcfcf",
        framealpha=0.95,
        fontsize=12,
    )
    axis.margins(x=0.02, y=0.05)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    figure.tight_layout()
    figure.savefig(out_path, dpi=150)
    plt.close(figure)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", required=True)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    if args.target not in PLOT_META:
        available = ", ".join(sorted(PLOT_META))
        raise SystemExit(f"unknown target '{args.target}', expected one of: {available}")

    draw_plot(parse_csv(args.input), args.target, args.output)


if __name__ == "__main__":
    main()
