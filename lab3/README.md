# Lab3 Sortings (C)

Реализация пунктов 5–7 лабораторной (стиль `lab1`, C only).

## Что реализовано
- **Пункт 5**: сравнение выбора pivot для лучшего quicksort (3-way)
  с более подробными названиями серий:
  `quick_3way_pivot_center`, `quick_3way_pivot_median3`,
  `quick_3way_pivot_random`, `quick_3way_pivot_median3_random`.
- **Пункт 6**: introsort с подбором параметров:
  `introsort_threshold16_depth2_heap2`,
  `introsort_threshold32_depth2_heap4`,
  `introsort_threshold64_depth3_heap8`,
  плюс baseline `quick_3way_pivot_median3_baseline`.
- **Пункт 7**: сравнение не-сравнительных сортировок
  `radix_lsd_bytewise`, `radix_msd_bytewise`
  с baseline-сортировками `quick_3way_pivot_median3_baseline`
  и `merge_iterative_baseline`.

Для всех пунктов: диапазон `n=0..10^7`, шаг `10^5`, `100` копий.

## Инструменты
- Генератор: `bin/gen_random`
- Эталон: `bin/qsort_solver`
- Раннер: `bin/tester` (`p5` / `p6` / `p7`)
- Генерация датасета: `scripts/gen_tests.sh`
- Пакетный запуск пункта: `scripts/run_point.sh`
- Графики: `python/plot_results.py`

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
lab3/scripts/run_point.sh p5 lab3/tests/p5 lab3/results/p5.csv 2147483647 42
lab3/scripts/run_point.sh p6 lab3/tests/p6 lab3/results/p6.csv 2147483647 42
lab3/scripts/run_point.sh p7 lab3/tests/p7 lab3/results/p7.csv 2147483647 42
```

## Графики
```bash
python3 lab3/python/plot_results.py --input lab3/results/p5.csv --output lab3/results/p5.png --title "Point 5"
python3 lab3/python/plot_results.py --input lab3/results/p6.csv --output lab3/results/p6.png --title "Point 6"
python3 lab3/python/plot_results.py --input lab3/results/p7.csv --output lab3/results/p7.png --title "Point 7"
```
