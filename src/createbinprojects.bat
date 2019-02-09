@echo off

set slnName="binaries"
set vpcCommands=/hl2 /hl2mp /episodic +binaries

if not exist "%slnName%.sln" (
	devtools\bin\vpc.exe %vpcCommands% /mksln %slnName%.sln
) else (
	devtools\bin\vpc.exe %vpcCommands%
)
pause