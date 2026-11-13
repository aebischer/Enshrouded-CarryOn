@echo off
setlocal
cd /d "%~dp0"
set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%vswhere%" (
 echo Visual Studio Installer with C++ x64 tools is required.
 exit /b 1
)
for /f "usebackq tokens=*" %%i in (`"%vswhere%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "vsroot=%%i"
if not defined vsroot exit /b 1
call "%vsroot%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
if not exist build mkdir build
if not exist dist\Mods\carryon mkdir dist\Mods\carryon
pushd build
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W4 /WX /LD /I..\sdk\include ..\src\carryon.cpp /Fe:..\dist\Mods\carryon\carryon.dll /link /IMPLIB:carryon.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W4 /WX /I..\sdk\include ..\tests\integration.cpp /Fe:integration.exe
if errorlevel 1 exit /b 1
integration.exe
if errorlevel 1 exit /b 1
popd
copy /y src\carryon.dll.mod dist\Mods\carryon\carryon.dll.mod >nul
exit /b %errorlevel%
