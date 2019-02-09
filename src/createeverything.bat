@echo off

set slnName="everything"
set vpcCommands=/hl2 /hl2mp /episodic +everything

if not exist "%slnName%.sln" (
	devtools\bin\vpc.exe %vpcCommands% /mksln %slnName%.sln
) else (
	devtools\bin\vpc.exe %vpcCommands%
)
pause