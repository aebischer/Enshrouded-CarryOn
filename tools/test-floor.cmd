@echo off
setlocal
cd /d "%~dp0.."
call "%~dp0vcvars.cmd"
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W4 /WX tests\floor_placement.cpp /Fo:build\floor_placement_test.obj /Fe:build\floor_placement_test.exe
if errorlevel 1 exit /b 1
build\floor_placement_test.exe "%~1" "%~2"
exit /b %errorlevel%
