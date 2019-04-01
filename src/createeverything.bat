@echo off
devtools\bin\vpc.exe /hl2r +everything /mksln everything.sln /define:vs2017
@REM devtools\bin\vpc.exe /hl2r /hl2 /hl2mp /episodic +everything /mksln everything_x64.sln /win64 /define:vs2017
@REM devtools\bin\vpc.exe /hl2r /hl2 /hl2mp /episodic +unittests /mksln unittests.sln /define:vs2017
pause