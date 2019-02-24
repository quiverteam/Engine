@echo off

set cmdLine=-console +volume 0.25 -sw -game hl2r

start "" quiver.exe %cmdLine%
echo quiver.exe %cmdLine%

timeout 3

start "" "quiver.exe" -hijack +map test_projtex