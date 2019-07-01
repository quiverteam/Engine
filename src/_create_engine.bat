@echo off

devtools\bin\vpc.exe /allgames +game /define:VS2019 /mksln engine.sln
@REM devtools\bin\vpc.exe /allgames +game /define:VS2019 /mksln engine.sln /win64

pause