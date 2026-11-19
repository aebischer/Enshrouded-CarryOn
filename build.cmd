@echo off
setlocal
cd /d "%~dp0"
call "%~dp0tools\vcvars.cmd"
if errorlevel 1 exit /b 1
if not exist build mkdir build
if not exist dist\Mods\EnshroudedModLoader\carryon mkdir dist\Mods\EnshroudedModLoader\carryon
powershell -NoProfile -ExecutionPolicy Bypass -File tools\embed-icon.ps1
if errorlevel 1 exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\embed-texts.ps1
if errorlevel 1 exit /b 1
rc /nologo /fo build\pincer_model.res src\pincer_model.rc
if errorlevel 1 exit /b 1
pushd build
cl /nologo /O2 /MT /c /I..\third_party\minhook\include ..\third_party\minhook\src\buffer.c ..\third_party\minhook\src\hook.c ..\third_party\minhook\src\trampoline.c ..\third_party\minhook\src\hde\hde64.c
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W4 /WX /LD /I..\sdk\include /I..\third_party\minhook\include /I. ..\src\carryon.cpp ..\src\native_content.cpp buffer.obj hook.obj trampoline.obj hde64.obj pincer_model.res /Fe:..\dist\Mods\EnshroudedModLoader\carryon\carryon.dll /link /IMPLIB:carryon.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W4 /WX /I..\sdk\include ..\tests\integration.cpp /Fe:integration.exe
if errorlevel 1 exit /b 1
integration.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W4 /WX ..\tests\transport_selection.cpp /Fe:transport_selection.exe
if errorlevel 1 exit /b 1
transport_selection.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W4 /WX ..\tests\transport_move.cpp /Fe:transport_move.exe
if errorlevel 1 exit /b 1
transport_move.exe
if errorlevel 1 exit /b 1
popd
rem Manifest generated and validated by embed-texts.ps1.
exit /b %errorlevel%
