## Быстрый запус
```powershell
python .\scripts\run_all.py
```

## Ручной запуск
### 1. Сборка
```powershell
make clean
make all
```

### 2. API-тесты
```powershell
make test
.\api_tests.exe
```

### 3. Запуск benchmark_app
Формат:
```text
benchmark_app --impl <array|list> --test <1|2|3|4> [--n <value>] [--ops-file <path>]
```

Примеры:
```powershell
.\benchmark_app.exe --impl array --test 1
.\benchmark_app.exe --impl list  --test 2
.\benchmark_app.exe --impl array --test 4 --n 1000000
```

Для `test 3` нужен файл операций (`1` = push, `2` = pop), например:
```powershell
.\benchmark_app.exe --impl list --test 3 --ops-file .\tests\generated_tests\test3_ops_repeat_1.txt
```

## Где результаты
- `tests/generated_tests` - сгенерированные тестовые файлы
- `tests/results` - CSV и `conclusion.txt`
- `tests/plots` - графики PNG
