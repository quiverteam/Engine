@echo off

set cmdLine=-novid -dev -console +volume 0.25 +bind F5 screenshot +bind F12 screenshot -sw -w 1905 -h 1000 -game hl2r

start "" quiver.exe %cmdLine%
echo quiver.exe %cmdLine%

timeout 3

@REM -shadersondemand needs #define DYNAMIC_SHADER_COMPILE i think, and it also enables mat_flushshaders

@REM start "" "quiver.exe" -hijack +map test_projtex_2