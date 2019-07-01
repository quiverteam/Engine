@echo off
devtools\bin\vpc.exe /allgames +everything /define:VS2019 /mksln everything.sln
@REM devtools\bin\vpc.exe /allgames +everything /define:VS2019 /mksln everything_x64.sln /win64
pause