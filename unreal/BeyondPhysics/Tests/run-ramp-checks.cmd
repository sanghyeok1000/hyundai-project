@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cd /d "%~dp0"
if not exist "..\Saved\ModelTests" mkdir "..\Saved\ModelTests"
if not exist "..\SourceArt\Ramp" mkdir "..\SourceArt\Ramp"
cl /nologo /std:c++20 /EHsc /W4 /O2 /fp:precise "RampChecks.cpp" "..\Source\BeyondPhysics\Physics\VehicleModel.cpp" /Fo"..\Saved\ModelTests\\" /Fe"..\Saved\ModelTests\RampChecks.exe"
if errorlevel 1 exit /b 1
"..\Saved\ModelTests\RampChecks.exe" "..\SourceArt\Ramp\road.csv"
exit /b %errorlevel%
