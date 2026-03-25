@echo off
setlocal
if "%~4"=="" (
  echo usage: run_all_points.cmd ^<tests_root_dir^> ^<results_root_dir^> ^<limit^> ^<seed^>
  exit /b 1
)

pushd "%~dp0"
bash run_all_points.sh %1 %2 %3 %4
set "ERR=%errorlevel%"
popd
exit /b %ERR%
