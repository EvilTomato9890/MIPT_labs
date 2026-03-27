@echo off
setlocal EnableDelayedExpansion
if "%~2"=="" (
  echo usage: run_all_points.cmd ^<tests_root_dir^> ^<results_root_dir^>
  exit /b 1
)
echo WARNING: run_all_points.cmd is legacy. Prefer bash scripts from scripts/ instead.

where wsl.exe >nul 2>nul
if not errorlevel 1 (
  for /f "delims=" %%I in ('wsl.exe wslpath -a "%~dp0run_all_points.sh"') do set "SCRIPT=%%I"
  for /f "delims=" %%I in ('wsl.exe wslpath -a "%~f1"') do set "TESTS_ROOT=%%I"
  for /f "delims=" %%I in ('wsl.exe wslpath -a "%~f2"') do set "RESULTS_ROOT=%%I"
  bash -lc "bash \"!SCRIPT!\" \"!TESTS_ROOT!\" \"!RESULTS_ROOT!\""
  exit /b %errorlevel%
)

pushd "%~dp0"
bash run_all_points.sh "%~f1" "%~f2"
set "ERR=%errorlevel%"
popd
exit /b %ERR%
