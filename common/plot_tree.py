from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


def plot_tree(folder: Path, title: str) -> None:
    summary_path = folder / "results" / "summary.csv"
    plots_dir = folder / "plots"

    if not summary_path.exists():
        raise SystemExit(f"{summary_path} not found. Run `make bench` first.")

    data = pd.read_csv(summary_path)
    plots_dir.mkdir(exist_ok=True)

    for metric, suffix, ylabel in [
        ("insert_avg", "insert", "insert seconds"),
        ("erase_avg", "erase", "erase seconds"),
    ]:
        plt.figure(figsize=(9, 5))
        for scenario in sorted(data["scenario"].unique()):
            part = data[data["scenario"] == scenario].sort_values("n")
            plt.plot(part["n"], part[metric], marker="o", linewidth=2, label=scenario)
        plt.title(f"{title}: {suffix}")
        plt.xlabel("n")
        plt.ylabel(ylabel)
        plt.grid(True, alpha=0.3)
        plt.legend()
        plt.tight_layout()
        plt.savefig(plots_dir / f"{suffix}.png", dpi=160)
        plt.close()
