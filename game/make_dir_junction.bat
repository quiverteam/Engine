@echo off

@REM replace this with your Half-Life 2 or Source 2013 directory
set "HL2Dir=C:\Program Files (x86)\Steam\steamapps\common\Half-Life 2"

if not exist "%HL2Dir%" (
	echo You need to set a different directory for your HL2 or Source 2013 install,
	echo As "%HL2Dir%" does not exist.
	pause
	exit
)

mklink /J "%cd%\hl2" "%HL2Dir%\hl2"
echo.
mklink /J "%cd%\episodic" "%HL2Dir%\episodic"
echo.
mklink /J "%cd%\ep2" "%HL2Dir%\ep2"
echo.

pause

