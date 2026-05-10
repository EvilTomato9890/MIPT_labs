.PHONY: report plots clean

report: plots

plots:
	powershell -NoProfile -Command "python report.py"

clean:
	python -c "import shutil; shutil.rmtree('plots', ignore_errors=True); shutil.rmtree('results', ignore_errors=True); [p.unlink() for p in __import__('pathlib').Path('.').glob('*.md') if p.name == 'REPORT.md']"
