# Лабораторная: деревья поиска

Проект содержит реализации:

- `01_naive` - наивное бинарное дерево поиска;
- `02_avl` - AVL-дерево;
- `03_treap` - декартово дерево;
- `04_splay` - Splay-дерево;
- `05_rbtree` - красно-черное дерево;
- `06_btree` - B-дерево с минимальной степенью `t=64`;
- `07_skiplist` - Skip-list;
- `08_conclusion` - общий отчет и сравнительные графики.

## Запуск

Для macOS нужны Command Line Tools и Python-зависимости:

```sh
xcode-select --install
python3 -m pip install pandas matplotlib
```

```sh
make test
make bench
make plots
make report
```

Полный запуск:

```sh
make all
```

Быстрая проверка пайплайна без полного миллиона элементов:

```sh
LAB_FAST=1 make all
```

В PowerShell:

```powershell
$env:LAB_FAST='1'; make all
```

Очистка сгенерированных файлов:

```sh
make clean
```
