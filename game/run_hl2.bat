@echo off

set cmdLine=-console -condebug -debug -game hl2

start "" bin\win32\quiver.exe %cmdLine%
echo bin\win32\quiver.exe %cmdLine%

timeout 5