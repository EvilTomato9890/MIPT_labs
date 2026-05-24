@echo off
setlocal EnableDelayedExpansion
if "%~3"=="" (
  echo usage: run_point.cmd ^<point:p1..p10^> ^<tests_root_dir^> ^<results_root_dir^>
  exit /b 1
)
echo WARNING: run_point.cmd is legacy. Prefer bash scripts from scripts/ instead.

where wsl.exe >nul 2>nul
if not errorlevel 1 (
  for /f "delims=" %%I in ('wsl.exe wslpath -a "%~dp0run_point.sh"') do set "SCRIPT=%%I"
  for /f "delims=" %%I in ('wsl.exe wslpath -a "%~f2"') do set "TESTS_ROOT=%%I"
  for /f "delims=" %%I in ('wsl.exe wslpath -a "%~f3"') do set "RESULTS_ROOT=%%I"
  bash -lc "bash \"!SCRIPT!\" \"%~1\" \"!TESTS_ROOT!\" \"!RESULTS_ROOT!\""
  exit /b %errorlevel%
)

pushd "%~dp0"
bash run_point.sh "%~1" "%~f2" "%~f3"
set "ERR=%errorlevel%"
popd
exit /b %ERR%
