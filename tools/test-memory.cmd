@echo off
setlocal
cd /d "%~dp0.."
call "%~dp0vcvars.cmd"
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W4 /WX /Ithird_party\minhook\include /Ibuild /Isdk\include tests\memory_runtime.cpp build\buffer.obj build\hook.obj build\trampoline.obj build\hde64.obj build\pincer_model.res /Fo:build\memory_runtime_test.obj /Fe:build\memory_runtime_test.exe
if errorlevel 1 exit /b 1
build\memory_runtime_test.exe "%~1" "%~2"
exit /b %errorlevel%
