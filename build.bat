@echo off
setlocal
cd /d "%~dp0"
if exist "C:\msys64\ucrt64\bin\g++.exe" set "PATH=C:\msys64\ucrt64\bin;%PATH%"
where g++ >nul 2>nul
if errorlevel 1 (
  echo A C++17 g++ compiler was not found.
  echo See START_HERE.txt for the one-time Windows compiler setup.
  exit /b 1
)
if not exist build mkdir build
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude src\main.cpp -o build\triewise.exe
if errorlevel 1 exit /b 1
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude tests\test_engine.cpp -o build\triewise_tests.exe
if errorlevel 1 exit /b 1
build\triewise_tests.exe
if errorlevel 1 exit /b 1
echo Build and engine tests passed. Start with run.bat.
