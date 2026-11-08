@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cd /d "%~dp0"
if not exist "..\Saved\ModelTests" mkdir "..\Saved\ModelTests"
cl /nologo /std:c++20 /EHsc /W4 /O2 /fp:precise "ContactPatchChecks.cpp" "..\Source\BeyondPhysics\Physics\VehicleModel.cpp" /Fo"..\Saved\ModelTests\\" /Fe"..\Saved\ModelTests\ContactPatchChecks.exe"
if errorlevel 1 exit /b 1
"..\Saved\ModelTests\ContactPatchChecks.exe"
exit /b %errorlevel%

