# Lab3 Sortings (C)

Реализация пунктов 1–3 лабораторной (стиль `lab1`, C only).

## Что реализовано
- **Пункт 1**: insertion / bubble / selection / shell(Knuth), случайные массивы,
  диапазон `n=0..10000`, шаг `100`, `100` копий.
- **Пункт 2**: `k`-ичная куча с bottom-up построением, сравнение `k=2..10`,
  диапазон `n=0..10^7`, шаг `10^5`, `100` копий.
- **Пункт 3**: merge sort (recursive vs iterative),
  диапазон `n=0..10^7`, шаг `10^5`, `100` копий.

## Инструменты
- Генератор: `bin/gen_random`
- Эталон: `bin/qsort_solver`
- Раннер: `bin/tester` (`p1` / `p2` / `p3`)
- Генерация датасета: `scripts/gen_tests.sh`
- Графики: `python/plot_results.py`

## Сборка
```bash
make -C lab3 all
```

## Smoke-check
```bash
make -C lab3 check
```

## Формирование набора тестов
```bash
lab3/scripts/gen_tests.sh lab3/tests/p1 0 10000 100 100 2147483647 lab3/bin/gen_random lab3/bin/qsort_solver 42
```

## Запуск пунктов
```bash
lab3/bin/tester p1 lab3/tests/p1 lab3/results/p1.csv 0 10000 100 100
lab3/bin/tester p2 lab3/tests/p2 lab3/results/p2.csv 0 10000000 100000 100
lab3/bin/tester p3 lab3/tests/p3 lab3/results/p3.csv 0 10000000 100000 100
```

## Графики
```bash
python3 lab3/python/plot_results.py --input lab3/results/p1.csv --output lab3/results/p1.png --title "Point 1"
python3 lab3/python/plot_results.py --input lab3/results/p2.csv --output lab3/results/p2.png --title "Point 2"
python3 lab3/python/plot_results.py --input lab3/results/p3.csv --output lab3/results/p3.png --title "Point 3"
```
