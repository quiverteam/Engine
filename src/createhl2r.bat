@echo off
devtools\bin\vpc.exe /hl2r +gamedlls +shaders_all /mksln game_hl2r.sln
devtools\bin\vpc.exe /hl2r +gamedlls +shaders_all /mksln game_hl2r_x64.sln /win64
pause