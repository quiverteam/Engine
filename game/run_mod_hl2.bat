@echo off

set cmd_line=-console -sw -noborder -game mod_hl2

start "" bin\win32\quiver.exe %cmd_line%
echo bin\win32\quiver.exe %cmd_line%

timeout 5