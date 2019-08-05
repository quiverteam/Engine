@echo off
devtools\bin\vpc.exe /allgames +binaries /define:VS2019 /mksln binaries.sln
@REM devtools\bin\vpc.exe /allgames +binaries /define:VS2019 /mksln binaries_x64.sln /win64
pause