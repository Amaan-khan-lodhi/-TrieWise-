@echo off
setlocal
cd /d "%~dp0"
if exist "C:\msys64\ucrt64\bin\g++.exe" set "PATH=C:\msys64\ucrt64\bin;%PATH%"
if not exist build\triewise.exe (
  echo First launch: building TrieWise and running the engine tests...
  call build.bat
  if errorlevel 1 goto failed
)
build\triewise.exe %*
if errorlevel 1 goto failed
exit /b 0

:failed
echo.
echo TrieWise could not start. Read the error above and START_HERE.txt.
pause
exit /b 1
