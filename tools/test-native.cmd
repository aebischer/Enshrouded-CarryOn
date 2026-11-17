@echo off
setlocal
cd /d "%~dp0.."
call "%~dp0vcvars.cmd"
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W4 /WX /Ithird_party\minhook\include /Ibuild /Isdk\include tests\native_content.cpp build\buffer.obj build\hook.obj build\trampoline.obj build\hde64.obj build\pincer_model.res /Fo:build\native_content_test.obj /Fe:build\native_content_test.exe
if errorlevel 1 exit /b 1
build\native_content_test.exe "%~1" "%~2" "%~3" "%~4" "%~5" "%~6"
exit /b %errorlevel%


