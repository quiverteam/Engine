@echo off

set slnName="libraries"
set vpcCommands=/hl2 /hl2mp /episodic +libraries

if not exist "%slnName%.sln" (
	devtools\bin\vpc.exe %vpcCommands% /mksln %slnName%.sln
) else (
	devtools\bin\vpc.exe %vpcCommands%
)
pause