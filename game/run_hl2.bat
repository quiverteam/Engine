@echo off

set cmdLine=-console -condebug -debug -game hl2

start "" bin\win32\hl2.exe %cmdLine%
echo bin\win32\hl2.exe %cmdLine%

timeout 5