# Lab3 Sortings (C)

Реализация всех пунктов ЛР (1..10), в стиле `lab1`, код только на C.

## Покрытие ТЗ
- `p1`: insertion / bubble / selection / shell(knuth), тесты `small_tests`.
- `p2`: k-heap (`k=2..10`, bottom-up), тесты `big_tests`.
- `p3`: merge recursive vs iterative, тесты `big_tests`.
- `p4`: quicksort (Lomuto / Hoare / 3-way), тесты `big_tests` и `test_most_dublicates`.
- `p5`: quicksort 3-way + выбор pivot (center / median3 / random / median3_random), `big_tests`.
- `p6`: подбор порога блока `t` для introsort + quick baseline, `big_tests`.
- `p7`: introspective sorting vs лучшая quicksort, `big_tests`.
- `p8`: timsort + pdqsort + сравнение из пункта 7, `big_tests`.
- `p9`: LSD/MSD radix, `big_tests`.
- `p10`: финальный график лучших + `qsort`, `big_tests`.

## Группы тестов по ТЗ
- `small_tests`: `0..1000`, шаг `50`, копий `5`, limit=`MAX_RAND`.
- `big_tests`: `0..1000000`, шаг `10000`, копий `5`, limit=`MAX_RAND`.
- `test_most_dublicates`: `0..1000000`, шаг `10000`, копий `5`, limit=`10000`.

## Инструменты
- `bin/gen_random` — генератор входов
- `bin/qsort_solver` — эталон
- `bin/tester` — запуск `p1..p10`
- `scripts/gen_tests.sh` — генерация `.in/.out`
- `scripts/run_point.sh` — полный запуск отдельного пункта
- `scripts/run_all_points.sh` — генерация 3 папок тестов и прогон всех пунктов на одинаковых данных
- `python/plot_results.py` — построение графиков

## Сборка и проверка
```bash
make -C lab3 all
make -C lab3 check
```


### Что такое `limit`
`limit` — это верхняя граница значения случайного элемента в генераторе (`gen_random`).
Массивы генерируются из диапазона `[0, limit)`.
Для `MAX_RAND` используйте `2147483647`.

## Примеры запуска
```bash
lab3/scripts/run_point.sh p1 lab3/tests lab3/results/p1 2147483647 42
lab3/scripts/run_point.sh p4 lab3/tests lab3/results/p4 2147483647 42
lab3/scripts/run_point.sh p10 lab3/tests lab3/results/p10 2147483647 42
```


### Полный прогон на одинаковых данных (по ТЗ)
```bash
lab3/scripts/run_all_points.sh lab3/tests lab3/results 2147483647 42
```
Скрипт один раз генерирует `small_tests`, `big_tests`, `test_most_dublicates` и затем запускает `p1..p10` на этих же наборах.


### Запуск в Windows
Если `.sh` открывается в редакторе, запускайте через `bash` или используйте `.cmd`-обёртки:
```powershell
lab3\scripts\run_point.cmd p1 lab3\tests lab3\results\p1 2147483647 42
lab3\scripts\run_all_points.cmd lab3\tests lab3\results 2147483647 42
```
(внутри они вызывают `bash run_point.sh` / `bash run_all_points.sh`).

## Графики
```bash
python3 lab3/python/plot_results.py --input lab3/results/p1_small.csv --output lab3/results/p1_small.png --title "Point 1"
python3 lab3/python/plot_results.py --input lab3/results/p4_big.csv --output lab3/results/p4_big.png --title "Point 4 big_tests"
python3 lab3/python/plot_results.py --input lab3/results/p4_dup.csv --output lab3/results/p4_dup.png --title "Point 4 duplicates"
python3 lab3/python/plot_results.py --input lab3/results/p10_big.csv --output lab3/results/p10_big.png --title "Point 10"
```

## Ответы на вопросы из задания

### 1) Какие квадратичные сортировки лучше и почему?
На случайных данных `shell(knuth)` почти всегда быстрее `insertion/bubble/selection`,
потому что уменьшает число дальних инверсий на больших шагах. `insertion` обычно вторая
по скорости и хорошо работает на почти отсортированных массивах.

### 2) Как влияет параметр `k` у k-ичной кучи?
При малом `k` дерево выше, при большом `k` растёт цена одного просеивания.
Практически чаще всего выигрывают средние значения (`k≈4..8`), а не крайние.

### 3) Рекурсивный vs итеративный merge sort
Обе версии имеют одинаковую асимптотику `O(n log n)` и требуют доп.память `O(n)`.
Итеративный вариант обычно чуть стабильнее по времени из-за отсутствия рекурсивных вызовов.

### 4) Какой partition в quicksort лучше?
`3-way` выигрывает на данных с повторами, потому что сразу выделяет блок равных ключей.
`Hoare` обычно даёт меньше обменов, `Lomuto` проще, но чаще медленнее.

### 5) Как лучше выбирать pivot?
`median3` и `median3_random` обычно устойчивее, чем `center/random` на разных входах,
так как уменьшают риск сильного перекоса разбиений.

### 6) Почему introsort полезен?
Introsort сочетает быстрый quicksort на «нормальных» данных и гарантии worst-case за счёт
перехода к heap sort при плохой глубине рекурсии, плюс insertion sort на малых диапазонах.

### 7) Стало ли лучше с introspective sorting?
На больших массивах introsort обычно стабильнее quicksort, потому что избегает
деградации на плохих разбиениях.

### 8) Зачем timsort и pdqsort?
- `timsort` эффективно использует естественные runs, особенно на частично отсортированных данных.
- `pdqsort` улучшает quicksort на «плохих» паттернах и старается избегать деградации.

### 9) LSD vs MSD
- `LSD` проще и обычно очень стабилен по времени.
- `MSD` может выигрывать на распределениях, где ранние биты хорошо разделяют данные,
  но чувствительнее к характеру входа.

### 10) Какой итог по лучшим алгоритмам?
Единственного абсолютного лидера нет:
- для универсальности и worst-case гарантий хорош introsort;
- для целочисленных больших наборов часто лидируют radix;
- `qsort` удобен как эталон и baseline сравнения.
