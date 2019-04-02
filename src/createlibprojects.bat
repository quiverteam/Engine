@echo off
devtools\bin\vpc.exe /hl2r /hl2 /hl2mp /episodic +libraries /define:VS2019 /mksln libraries.sln
devtools\bin\vpc.exe /hl2r /hl2 /hl2mp /episodic +libraries /define:VS2019 /mksln libraries_x64.sln /win64
pause