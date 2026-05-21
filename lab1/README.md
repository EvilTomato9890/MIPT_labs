# Практическая работа 2: стеки на массиве и списке

В проекте реализованы:

- динамический массив;
- односвязный список;
- стек на динамическом массиве;
- стек на односвязном списке;
- benchmark для четырех тестов из задания;
- Python-скрипт для генерации тестовых данных, запуска измерений, усреднения результатов и построения графиков.

## Быстрый запуск

```powershell
python .\scripts\run_all.py
```

По умолчанию каждый тест запускается 3 раза. Для теста 4 значения `n` перебираются от `1000` до `1000000` с шагом `1000`.

Для короткой проверки можно уменьшить диапазон:

```powershell
python .\scripts\run_all.py --repeats 1 --max-n 1000 --step 1000
```

## Ручной запуск

### 1. Сборка

```powershell
make clean
make all
```

### 2. Дополнительные API-тесты

```powershell
make test
.\api_tests.exe
```

Эти тесты не являются частью обязательных benchmark-измерений из задания. Они нужны только для быстрой проверки базовых операций контейнеров и стеков.

### 3. Запуск benchmark_app

Формат:

```text
benchmark_app --impl <array|list> --test <1|2|3|4> [--n <value>] [--ops-file <path>]
```

Примеры:

```powershell
.\benchmark_app.exe --impl array --test 1
.\benchmark_app.exe --impl list --test 2
.\benchmark_app.exe --impl array --test 4 --n 1000000
```

Для теста 3 нужен файл операций из `1` и `2`:

```powershell
.\benchmark_app.exe --impl list --test 3 --ops-file .\tests\generated_tests\test3_ops_repeat_1.txt
```

## Результаты

`scripts/run_all.py` создает:

- `tests/generated_tests/` - сгенерированные входные данные;
- `tests/results/raw_results.csv` - все измерения;
- `tests/results/averaged_tests_1_3.csv` - средние времена для тестов 1-3;
- `tests/results/averaged_test4.csv` - средние времена для теста 4;
- `tests/results/conclusion.txt` - краткий вывод;
- `tests/plots/tests_1_3_comparison.png` - сравнение тестов 1-3;
- `tests/plots/test4_time_n.png` - график `time(n)` для двух реализаций стека на одном полотне.

Сгенерированные данные, результаты и графики не добавляются в репозиторий.
