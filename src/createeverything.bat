@echo off
devtools\bin\vpc.exe /hl2r /hl2 /hl2mp /episodic +everything /mksln everything.sln
devtools\bin\vpc.exe /hl2r /hl2 /hl2mp /episodic +everything /mksln everything_x64.sln /win64
devtools\bin\vpc.exe /hl2r /hl2 /hl2mp /episodic +unittests /mksln unittests.sln
pause