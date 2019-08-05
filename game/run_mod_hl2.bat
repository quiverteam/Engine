@echo off

set cmdLine=-noborder -sw -novid -dev -console -w 1920 -h 1080 -game mod_hl2

start "" bin\win32\quiver.exe %cmdLine%
echo bin\win32\quiver.exe %cmdLine%

timeout 5