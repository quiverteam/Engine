@echo off

@REM -gameui for client gameui
set cmdLine=-novid -dev -console +volume 0.25 +bind F5 screenshot +bind F12 screenshot -sw -w 1920 -h 1080 -noborder -game hl2r

start "" bin\win32\quiver.exe %cmdLine%
echo bin\win32\quiver.exe %cmdLine%

timeout 3

@REM -shadersondemand needs #define DYNAMIC_SHADER_COMPILE i think, and it also enables mat_flushshaders

@REM start "" "quiver.exe" -hijack +map test_projtex_2