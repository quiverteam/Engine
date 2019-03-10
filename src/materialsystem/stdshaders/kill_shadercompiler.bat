@echo off

tasklist /FI "IMAGENAME eq shadercompile.exe" 2>NUL | find /I /N "shadercompile.exe">NUL

if "%ERRORLEVEL%"=="0" (
	echo Shadercompile is running still, killing
	taskkill /f /im shadercompile.exe >nul
)