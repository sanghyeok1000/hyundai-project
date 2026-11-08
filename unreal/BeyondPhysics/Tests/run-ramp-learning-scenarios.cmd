@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cd /d "%~dp0"
cl /nologo /std:c++20 /EHsc /W4 /O2 /fp:precise "RampLearningScenarios.cpp" "..\Source\BeyondPhysics\Physics\VehicleModel.cpp" /Fo"..\Saved\ModelTests\\" /Fe"..\Saved\ModelTests\RampLearningScenarios.exe"
if errorlevel 1 exit /b 1
"..\Saved\ModelTests\RampLearningScenarios.exe"
exit /b %errorlevel%
