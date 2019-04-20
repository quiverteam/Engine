@echo off

set "disabled_projects=-bsppack -bzip2 -ep2_deathmap -vtex_dll -vtex_launcher -normal2ssbump -height2normal -height2ssbump"

devtools\bin\vpc.exe /hl2 /hl2mp /episodic /hl2r +game +utils +tools %disabled_projects% /define:VS2019 /mksln engine_and_tools.sln
@REM devtools\bin\vpc.exe /hl2 /hl2mp /episodic /hl2r +game +utils +tools %disabled_projects%  /define:VS2019 /mksln engine_and_tools_x64.sln /win64
@REM devtools\bin\vpc.exe /hl2r /hl2 /hl2mp /episodic /define:VS2019 +unittests /mksln unittests.sln

pause