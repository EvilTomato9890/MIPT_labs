.PHONY: report plots clean

ifeq ($(OS),Windows_NT)
PYTHON ?= $(shell powershell -NoProfile -Command "(Get-Command python).Source -replace '\\','/'")
else
PYTHON ?= python3
endif

report: plots

plots:
	$(PYTHON) report.py

clean:
	$(PYTHON) -c "import shutil; shutil.rmtree('plots', ignore_errors=True); shutil.rmtree('results', ignore_errors=True); [p.unlink() for p in __import__('pathlib').Path('.').glob('*.md') if p.name == 'REPORT.md']"
