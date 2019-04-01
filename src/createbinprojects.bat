@echo off
devtools\bin\vpc.exe /hl2r /hl2 /hl2mp /episodic +binaries /mksln binaries.sln /define:vs2017
devtools\bin\vpc.exe /hl2r /hl2 /hl2mp /episodic +binaries /mksln binaries_x64.sln /win64 /define:vs2017
pause