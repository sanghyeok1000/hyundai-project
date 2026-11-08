@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist "%~dp0out" mkdir "%~dp0out"
cl /nologo /std:c++17 /EHsc /O2 "%~dp0FeedbackChecks.cpp" "%~dp0..\Source\BeyondPhysics\Feedback\FeedbackPolicy.cpp" /Fe:"%~dp0out\FeedbackChecks.exe" /Fo:"%~dp0out\\"
if errorlevel 1 exit /b 1
"%~dp0out\FeedbackChecks.exe"
