@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist "%~dp0..\build\tests" mkdir "%~dp0..\build\tests"
cl /nologo /std:c++17 /EHsc /W4 "%~dp0fresh_response_test.cpp" /Fo"%~dp0..\build\tests\fresh_response_test.obj" /Fe"%~dp0..\build\tests\fresh_response_test.exe"
if errorlevel 1 exit /b 1
"%~dp0..\build\tests\fresh_response_test.exe"
