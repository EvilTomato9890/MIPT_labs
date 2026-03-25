# Lab3 Sortings (C)

Реализация пунктов 8–10 лабораторной (стиль `lab1`, C only).

## Что реализовано
- **Пункт 8 (полностью)**:
  - в сравнение добавлены `timsort_hybrid_runs` и `pdqsort_pattern_defeating`;
  - обе сортировки реализованы в проекте самостоятельно на C
    (`timsort_sort`, `pdqsort_sort`), без внешних библиотек.
  - сравниваются с `quick_3way_pivot_median3_baseline` и
    `introsort_threshold32_depth2_heap4`.
- **Пункт 9**:
  - реализованы и сравниваются `radix_lsd_bytewise` и `radix_msd_bytewise`.
- **Пункт 10**:
  - общий итоговый график лучших подходов + `qsort_stdlib_reference`.

## Инструменты
- Генератор: `bin/gen_random`
- Эталон: `bin/qsort_solver`
- Раннер: `bin/tester` (`p8` / `p9` / `p10`)
- Генерация датасета: `scripts/gen_tests.sh`
- Пакетный запуск пункта: `scripts/run_point.sh`
- Графики: `python/plot_results.py`

## Диапазоны для запуска
Для `p8`, `p9`, `p10` в скрипте заданы:
- `from=0`, `to=1000000`, `step=10000`, `copies=5`

(это соответствует `big_tests` из условия и выполняется за разумное время).

## Сборка
```bash
make -C lab3 all
```

## Smoke-check
```bash
make -C lab3 check
```

## Запуск пунктов
```bash
lab3/scripts/run_point.sh p8 lab3/tests/p8 lab3/results/p8.csv 2147483647 42
lab3/scripts/run_point.sh p9 lab3/tests/p9 lab3/results/p9.csv 2147483647 42
lab3/scripts/run_point.sh p10 lab3/tests/p10 lab3/results/p10.csv 2147483647 42
```

## Графики
```bash
python3 lab3/python/plot_results.py --input lab3/results/p8.csv --output lab3/results/p8.png --title "Point 8"
python3 lab3/python/plot_results.py --input lab3/results/p9.csv --output lab3/results/p9.png --title "Point 9"
python3 lab3/python/plot_results.py --input lab3/results/p10.csv --output lab3/results/p10.png --title "Point 10"
```
