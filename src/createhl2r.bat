@echo off
devtools\bin\vpc.exe /hl2r +gamedlls +shaders_all /mksln game_hl2r.sln /define:vs2017
devtools\bin\vpc.exe /hl2r +gamedlls +shaders_all /mksln game_hl2r_x64.sln /win64 /define:vs2017
pause