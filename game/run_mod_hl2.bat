@echo off

set cmdLine=-console -sw -game mod_hl2

start "" bin\win32\quiver.exe %cmdLine%
echo bin\win32\quiver.exe %cmdLine%

timeout 5