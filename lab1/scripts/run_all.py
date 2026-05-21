#!/usr/bin/env python3

from __future__ import annotations

import argparse
import random
import shutil
import subprocess
from pathlib import Path

try:
    import matplotlib.pyplot as plt
    import pandas as pd
except ModuleNotFoundError as error:
    raise SystemExit(
        "Missing Python dependency. Install required packages: pandas matplotlib"
    ) from error

TEST3_OPERATION_COUNT = 1_000_000
TEST3_BASE_SEED = 100_000
NDEBUG_CFLAGS = "-std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -DNDEBUG"


def run_command(command: list[str], workdir: Path) -> None:
    print(f"Running: {' '.join(command)}", flush=True)
    subprocess.run(command, cwd=workdir, check=True)


def run_make_target(workdir: Path, target: str) -> None:
    run_command(["make", target, f"CFLAGS={NDEBUG_CFLAGS}"], workdir)


def detect_executable(repo_root: Path, name: str) -> Path:
    candidates = [repo_root / name, repo_root / f"{name}.exe"]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise FileNotFoundError(f"Executable {name} was not found")


def generate_test3_operations(path: Path, seed: int, operations_count: int) -> None:
    random_generator = random.Random(seed)
    chunk_size = 50_000

    with path.open("w", encoding="utf-8") as file:
        operations_chunk: list[str] = []
        for _ in range(operations_count):
            operations_chunk.append(str(random_generator.randint(1, 2)))
            if len(operations_chunk) == chunk_size:
                file.write(" ".join(operations_chunk))
                file.write("\n")
                operations_chunk.clear()
        if operations_chunk:
            file.write(" ".join(operations_chunk))
            file.write("\n")


def generate_test_files(generated_tests_dir: Path,
                        repeats: int,
                        max_n: int,
                        step: int) -> tuple[list[int], list[int], list[Path]]:
    generated_tests_dir.mkdir(parents=True, exist_ok=True)

    (generated_tests_dir / "test1_config.txt").write_text(
        "push_count=1000000\nrule=pop(size/2),push(size/4),until size<100000\n",
        encoding="utf-8",
    )
    (generated_tests_dir / "test2_config.txt").write_text(
        "push_count=1000000\nblock=100 x [pop(10000),push(10000)]\n"
        "middle=rule from test1\n",
        encoding="utf-8",
    )

    n_values = list(range(step, max_n + 1, step))
    n_values_path = generated_tests_dir / "test4_n_values.txt"
    n_values_path.write_text("\n".join(str(value) for value in n_values) + "\n", encoding="utf-8")

    seeds: list[int] = []
    operation_files: list[Path] = []
    for repeat_index in range(repeats):
        seed = TEST3_BASE_SEED + repeat_index
        seeds.append(seed)
        operation_file = generated_tests_dir / f"test3_ops_repeat_{repeat_index + 1}.txt"
        print(
            f"Generating operations for test 3, repeat {repeat_index + 1}/{repeats} (seed={seed})",
            flush=True,
        )
        generate_test3_operations(operation_file, seed, TEST3_OPERATION_COUNT)
        operation_files.append(operation_file)

    return n_values, seeds, operation_files


def run_benchmark(binary_path: Path,
                  repo_root: Path,
                  implementation: str,
                  test_id: int,
                  n_value: int | None = None,
                  operations_file: Path | None = None) -> float:
    command = [
        str(binary_path),
        "--impl", implementation,
        "--test", str(test_id),
    ]
    if n_value is not None:
        command.extend(["--n", str(n_value)])
    if operations_file is not None:
        command.extend(["--ops-file", str(operations_file)])

    completed = subprocess.run(
        command,
        cwd=repo_root,
        check=True,
        text=True,
        capture_output=True,
    )
    output_lines = completed.stdout.strip().splitlines()
    if not output_lines:
        raise RuntimeError(f"Benchmark command produced empty output: {' '.join(command)}")

    try:
        return float(output_lines[-1].strip())
    except ValueError as error:
        raise RuntimeError(
            f"Cannot parse benchmark output as float.\n"
            f"Command: {' '.join(command)}\n"
            f"STDOUT:\n{completed.stdout}\n"
            f"STDERR:\n{completed.stderr}"
        ) from error


def collect_raw_records(binary_path: Path,
                        repo_root: Path,
                        repeats: int,
                        n_values: list[int],
                        test3_seeds: list[int],
                        test3_operation_files: list[Path]) -> list[dict[str, object]]:
    implementations = ["array", "list"]
    records: list[dict[str, object]] = []

    for implementation in implementations:
        for repeat_index in range(repeats):
            elapsed = run_benchmark(binary_path, repo_root, implementation, 1)
            records.append({
                "test_id": 1,
                "implementation": implementation,
                "repeat": repeat_index + 1,
                "n": None,
                "seed": None,
                "elapsed_seconds": elapsed,
            })

            elapsed = run_benchmark(binary_path, repo_root, implementation, 2)
            records.append({
                "test_id": 2,
                "implementation": implementation,
                "repeat": repeat_index + 1,
                "n": None,
                "seed": None,
                "elapsed_seconds": elapsed,
            })

            elapsed = run_benchmark(
                binary_path,
                repo_root,
                implementation,
                3,
                operations_file=test3_operation_files[repeat_index],
            )
            records.append({
                "test_id": 3,
                "implementation": implementation,
                "repeat": repeat_index + 1,
                "n": None,
                "seed": test3_seeds[repeat_index],
                "elapsed_seconds": elapsed,
            })

    for implementation in implementations:
        for repeat_index in range(repeats):
            print(f"Running test 4 for {implementation}, repeat {repeat_index + 1}/{repeats}", flush=True)
            for n_value in n_values:
                elapsed = run_benchmark(
                    binary_path,
                    repo_root,
                    implementation,
                    4,
                    n_value=n_value,
                )
                records.append({
                    "test_id": 4,
                    "implementation": implementation,
                    "repeat": repeat_index + 1,
                    "n": n_value,
                    "seed": None,
                    "elapsed_seconds": elapsed,
                })

    return records


def save_results(results_dir: Path, raw_records: list[dict[str, object]]) -> tuple[pd.DataFrame, pd.DataFrame]:
    results_dir.mkdir(parents=True, exist_ok=True)

    raw_frame = pd.DataFrame.from_records(raw_records)
    raw_frame.to_csv(results_dir / "raw_results.csv", index=False)

    averaged_tests_1_3 = (
        raw_frame[raw_frame["test_id"].isin([1, 2, 3])]
        .groupby(["test_id", "implementation"], as_index=False)["elapsed_seconds"]
        .mean()
    )
    averaged_tests_1_3.to_csv(results_dir / "averaged_tests_1_3.csv", index=False)

    averaged_test4 = (
        raw_frame[raw_frame["test_id"] == 4]
        .groupby(["n", "implementation"], as_index=False)["elapsed_seconds"]
        .mean()
        .sort_values("n")
    )
    averaged_test4.to_csv(results_dir / "averaged_test4.csv", index=False)

    return averaged_tests_1_3, averaged_test4


def plot_tests_1_3(averaged_tests_1_3: pd.DataFrame, output_path: Path) -> None:
    pivot = averaged_tests_1_3.pivot(
        index="test_id",
        columns="implementation",
        values="elapsed_seconds",
    ).sort_index()

    test_labels = [f"Test {int(test_id)}" for test_id in pivot.index.tolist()]
    x_positions = list(range(len(test_labels)))
    bar_width = 0.35

    array_positions = [position - bar_width / 2.0 for position in x_positions]
    list_positions = [position + bar_width / 2.0 for position in x_positions]

    fig, axis = plt.subplots(figsize=(10, 6))
    axis.bar(array_positions, pivot["array"].tolist(), width=bar_width, label="Array stack")
    axis.bar(list_positions, pivot["list"].tolist(), width=bar_width, label="List stack")

    axis.set_title("Average runtime for tests 1-3")
    axis.set_xlabel("Test")
    axis.set_ylabel("Time, seconds")
    axis.set_xticks(x_positions)
    axis.set_xticklabels(test_labels)
    axis.grid(True, axis="y", linestyle="--", alpha=0.4)
    axis.legend()

    fig.tight_layout()
    fig.savefig(output_path, dpi=200)
    plt.close(fig)


def plot_test4(averaged_test4: pd.DataFrame, output_path: Path) -> None:
    array_frame = averaged_test4[averaged_test4["implementation"] == "array"].sort_values("n")
    list_frame = averaged_test4[averaged_test4["implementation"] == "list"].sort_values("n")

    fig, axis = plt.subplots(figsize=(12, 6))
    axis.plot(
        array_frame["n"],
        array_frame["elapsed_seconds"],
        label="Array stack",
        linewidth=1.8,
    )
    axis.plot(
        list_frame["n"],
        list_frame["elapsed_seconds"],
        label="List stack",
        linewidth=1.8,
    )

    axis.set_title("time(n) for test 4")
    axis.set_xlabel("n (number of pushes)")
    axis.set_ylabel("Average time, seconds")
    axis.grid(True, linestyle="--", alpha=0.4)
    axis.legend()

    fig.tight_layout()
    fig.savefig(output_path, dpi=200)
    plt.close(fig)


def write_conclusion(results_dir: Path,
                     averaged_tests_1_3: pd.DataFrame,
                     averaged_test4: pd.DataFrame) -> None:
    pivot = averaged_tests_1_3.pivot(
        index="test_id",
        columns="implementation",
        values="elapsed_seconds",
    ).sort_index()

    array_wins = 0
    list_wins = 0
    draw_count = 0

    for test_id in [1, 2, 3]:
        array_value = float(pivot.loc[test_id, "array"])
        list_value = float(pivot.loc[test_id, "list"])

        if abs(array_value - list_value) < 1e-12:
            draw_count += 1
        elif array_value < list_value:
            array_wins += 1
        else:
            list_wins += 1

    array_test4_mean = float(
        averaged_test4[averaged_test4["implementation"] == "array"]["elapsed_seconds"].mean()
    )
    list_test4_mean = float(
        averaged_test4[averaged_test4["implementation"] == "list"]["elapsed_seconds"].mean()
    )

    if abs(array_test4_mean - list_test4_mean) < 1e-12:
        draw_count += 1
    elif array_test4_mean < list_test4_mean:
        array_wins += 1
    else:
        list_wins += 1

    if array_wins > list_wins:
        recommendation = "Лучше использовать стек на динамическом массиве."
    elif list_wins > array_wins:
        recommendation = "Лучше использовать стек на односвязном списке."
    else:
        recommendation = "Обе реализации показали сопоставимый результат."

    conclusion_text = (
        "Вывод по результатам измерений\n"
        f"Победы массива: {array_wins}\n"
        f"Победы списка: {list_wins}\n"
        f"Ничьи: {draw_count}\n"
        f"Среднее время test4 (array): {array_test4_mean:.9f} s\n"
        f"Среднее время test4 (list): {list_test4_mean:.9f} s\n"
        f"{recommendation}\n"
    )
    (results_dir / "conclusion.txt").write_text(conclusion_text, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate tests, run all benchmarks, and build plots for lab 2.",
    )
    parser.add_argument("--repeats", type=int, default=3, help="Number of repeats for each test.")
    parser.add_argument("--max-n", type=int, default=1_000_000, help="Maximum n for test 4.")
    parser.add_argument("--step", type=int, default=1_000, help="Step for n in test 4.")
    args = parser.parse_args()

    if args.repeats < 1:
        raise ValueError("--repeats must be positive")
    if args.max_n < 1 or args.step < 1:
        raise ValueError("--max-n and --step must be positive")

    repo_root = Path(__file__).resolve().parent.parent
    if shutil.which("make") is None:
        raise RuntimeError("`make` was not found in PATH.")

    tests_dir = repo_root / "tests"
    generated_tests_dir = tests_dir / "generated_tests"
    results_dir = tests_dir / "results"
    plots_dir = tests_dir / "plots"
    plots_dir.mkdir(parents=True, exist_ok=True)

    n_values, seeds, operation_files = generate_test_files(
        generated_tests_dir,
        args.repeats,
        args.max_n,
        args.step,
    )

    run_command(["make", "clean"], repo_root)
    run_make_target(repo_root, "all")

    benchmark_binary = detect_executable(repo_root, "benchmark_app")
    raw_records = collect_raw_records(
        benchmark_binary,
        repo_root,
        args.repeats,
        n_values,
        seeds,
        operation_files,
    )

    averaged_tests_1_3, averaged_test4 = save_results(results_dir, raw_records)
    plot_tests_1_3(averaged_tests_1_3, plots_dir / "tests_1_3_comparison.png")
    plot_test4(averaged_test4, plots_dir / "test4_time_n.png")
    write_conclusion(results_dir, averaged_tests_1_3, averaged_test4)

    print("Done. Files saved in:", flush=True)
    print(f"  {generated_tests_dir}", flush=True)
    print(f"  {results_dir}", flush=True)
    print(f"  {plots_dir}", flush=True)

    print("StAcK oN dYnAmIc ArRaY iS bEtTeR!!!!!!!!-_-!!!!!!!! (On this list realization)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
