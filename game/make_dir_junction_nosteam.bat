@echo off

@REM replace this with your Half-Life 2 or Source 2013 directory
set "hl2_dir=C:\Program Files (x86)\Steam\steamapps\common\Half-Life 2"
set "hl2mp_dir=C:\Program Files (x86)\Steam\steamapps\common\Half-Life 2 Deathmatch\hl2mp"

if not exist "%hl2_dir%\hl2" (
	echo Unable to find the directory "%hl2_dir%"
	echo If you've not done so already, please set the environment variable hl2_dir to your Source 2013 SP or Hl2 install directory.
	pause
	exit
)

mklink /J "%cd%\hl2" "%hl2_dir%\hl2"
echo.
mklink /J "%cd%\episodic" "%hl2_dir%\episodic"
echo.
mklink /J "%cd%\ep2" "%hl2_dir%\ep2"
echo.

if not exist "%hl2mp_dir%" (
	echo Unable to find the directory "%hl2mp_dir%"
	echo If you've not done so already, please set the environment variable hl2mp_dir to your Source 2013 MP or Hl2DM install directory.
	pause
	exit
)

mklink /J "%cd%\hl2mp" "%hl2mp_dir%"
echo.

pause