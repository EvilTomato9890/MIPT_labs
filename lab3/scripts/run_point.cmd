@echo off
setlocal
if "%~5"=="" (
  echo usage: run_point.cmd ^<point:p1..p10^> ^<tests_root_dir^> ^<csv_out_prefix^> ^<limit^> ^<seed^>
  exit /b 1
)

pushd "%~dp0"
bash run_point.sh %1 %2 %3 %4 %5
set "ERR=%errorlevel%"
popd
exit /b %ERR%
