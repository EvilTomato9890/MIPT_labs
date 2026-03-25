from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt  # type: ignore[import-not-found]


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


def draw_plot(data: dict[str, list[tuple[int, float]]], title: str, out_path: Path) -> None:
    plt.figure(figsize=(11, 7))
    for name, points in sorted(data.items()):
        x_vals = [p[0] for p in points]
        y_vals = [p[1] for p in points]
        plt.plot(x_vals, y_vals, label=name)
    plt.title(title)
    plt.xlabel("array size")
    plt.ylabel("avg time, sec")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(out_path)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--title", type=str, required=True)
    args = parser.parse_args()

    data = parse_csv(args.input)
    draw_plot(data, args.title, args.output)


if __name__ == "__main__":
    main()
