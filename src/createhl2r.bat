@echo off
devtools\bin\vpc.exe /hl2r +hl2r /define:VS2017 /mksln game_hl2r.sln
devtools\bin\vpc.exe /hl2r +hl2r /define:VS2017 /mksln game_hl2r_win64.sln /win64
pause
