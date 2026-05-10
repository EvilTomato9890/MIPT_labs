from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = Path(__file__).resolve().parent / "plots"
REPORT = Path(__file__).resolve().parent / "REPORT.md"


def read_summaries() -> pd.DataFrame:
    frames = []
    for path in sorted(ROOT.glob("0[1-7]_*/results/summary.csv")):
        frames.append(pd.read_csv(path))
    if not frames:
        raise SystemExit("No summary.csv files found. Run root `make bench` first.")
    return pd.concat(frames, ignore_index=True)


def plot_metric(data: pd.DataFrame, metric: str, title: str, filename: str) -> None:
    random_data = data[data["scenario"] == "random"].copy()
    plt.figure(figsize=(10, 6))
    for tree_name in sorted(random_data["tree"].unique()):
        part = random_data[random_data["tree"] == tree_name].sort_values("n")
        plt.plot(part["n"], part[metric], marker="o", linewidth=2, label=tree_name)
    plt.title(title)
    plt.xlabel("n")
    plt.ylabel("seconds")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(OUT_DIR / filename, dpi=160)
    plt.close()


def best_by_metric(data: pd.DataFrame, metric: str) -> str:
    random_data = data[data["scenario"] == "random"]
    latest_n = random_data["n"].max()
    latest = random_data[random_data["n"] == latest_n].sort_values(metric)
    return latest.iloc[0]["tree"]


def write_report(data: pd.DataFrame) -> None:
    best_insert = best_by_metric(data, "insert_avg")
    best_erase = best_by_metric(data, "erase_avg")
    lines = [
        "# Вывод по сравнению деревьев",
        "",
        "Бенчмарки сравнивают структуры как множество уникальных ключей: вставка строит дерево из перестановки `0..n-1`, затем удаляется первая половина вставленных ключей.",
        "",
        "## Основные наблюдения",
        "",
        "- Наивное дерево хорошо работает на случайной перестановке малого размера, но на отсортированной последовательности вырождается в список, поэтому операции становятся квадратичными.",
        "- AVL и красно-черное дерево сохраняют гарантированную логарифмическую высоту, поэтому дают стабильное время на случайных и отсортированных входах.",
        "- Декартово дерево, Splay и Skip-list являются рандомизированными/амортизированными структурами: на случайных данных они обычно близки к сбалансированным деревьям, но дают менее жесткие гарантии по отдельной операции.",
        "- B-дерево работает блоками ключей и уменьшает высоту дерева; при большом `n` это часто улучшает удаление и поиск за счет локальности памяти.",
        f"- В полученных данных самый быстрый вариант для вставки на максимальном случайном `n`: `{best_insert}`.",
        f"- В полученных данных самый быстрый вариант для удаления на максимальном случайном `n`: `{best_erase}`.",
        "",
        "## Графики",
        "",
        "- `plots/compare_insert.png` - сравнение времени вставки.",
        "- `plots/compare_erase.png` - сравнение времени удаления.",
    ]
    REPORT.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    OUT_DIR.mkdir(exist_ok=True)
    data = read_summaries()
    plot_metric(data, "insert_avg", "Insertion benchmark, random unique keys", "compare_insert.png")
    plot_metric(data, "erase_avg", "Erase benchmark, random unique keys", "compare_erase.png")
    write_report(data)
    print(f"wrote {REPORT}")


if __name__ == "__main__":
    main()
