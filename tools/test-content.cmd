@echo off
setlocal
cd /d "%~dp0.."
call "%~dp0vcvars.cmd"
if errorlevel 1 exit /b 1
if not exist build mkdir build
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W4 /WX tests\content_catalog.cpp /Fo:build\content_catalog.obj /Fe:build\content_catalog_test.exe
if errorlevel 1 exit /b 1
build\content_catalog_test.exe "%~1" "%~2" "%~3"
exit /b %errorlevel%
