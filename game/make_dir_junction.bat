@echo off

@REM replace this with your Half-Life 2 or Source 2013 directory
set "HL2Dir=D:\Steam\steamapps\common\Half-Life 2"

if not exist "%HL2Dir%\hl2" (
	echo Unable to find the directory "%HL2Dir%"
	echo If you've not done so already, please set the environment variable HL2Dir to your Source 2013 or Hl2 install directory.
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

