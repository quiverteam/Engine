@echo off

set slnName="game_hl2r"
set vpcCommands=/hl2r +gamedlls

if not exist "%slnName%.sln" (
	devtools\bin\vpc.exe %vpcCommands% /mksln %slnName%.sln
) else (
	devtools\bin\vpc.exe %vpcCommands%
)
pause