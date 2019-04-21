@echo off
devtools\bin\vpc.exe /hl2 /hl2mp /episodic /hl2r +everything /define:VS2019 /mksln everything.sln
@REM devtools\bin\vpc.exe /hl2r /hl2 /hl2mp /episodic /define:VS2019 +everything /mksln everything_x64.sln /win64
@REM devtools\bin\vpc.exe /hl2r /hl2 /hl2mp /episodic /define:VS2019 +unittests /mksln unittests.sln
pause