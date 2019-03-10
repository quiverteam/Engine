@echo off
setlocal enabledelayedexpansion
echo.
rem == Setup path to nmake.exe ==

@REM find vs2017 directory, if vswhere doesn't exist, skip
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
	for /f "usebackq tokens=1* delims=: " %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop`) do (
		if /i "%%i"=="installationPath" (
			set VSDIR=%%j
			call "!VSDIR!\Common7\Tools\VsDevCmd.bat" >nul
			echo Using VS2017 tools
			goto :start
		)
	)	
) else if exist "%VS140COMNTOOLS%vsvars32.bat" (
	call "%VS140COMNTOOLS%vsvars32.bat"
	echo Using VS2015 tools
	
) else if exist "%VS120COMNTOOLS%vsvars32.bat" (
	call "%VS120COMNTOOLS%vsvars32.bat"
	echo Using VS2013 tools
	
) else if exist "%VS100COMNTOOLS%vsvars32.bat" (
	call "%VS100COMNTOOLS%vsvars32.bat"
	echo Using VS2010 tools
	
) else echo Install Either Visual Studio Version 2010, 2013, 2015, or 2017

:start
set TTEXE=..\..\devtools\bin\timeprecise.exe
if not exist %TTEXE% goto no_ttexe
goto no_ttexe_end

:no_ttexe
set TTEXE=time /t
:no_ttexe_end


echo.
echo ~~~~~~ buildallshaders %* ~~~~~~
%TTEXE% -cur-Q
set tt_all_start=%ERRORLEVEL%
set tt_all_chkpt=%tt_start%

rem =================================
rem ====== PATH CONFIGURATIONS ======

rem == Set the absolute path to your mod's game directory here ==
@REM change this to the core folder (platform atm)
set GAMEDIR=%cd%\..\..\..\game\mod_hl2

rem == Set the relative or absolute path to the bin folder ==
set "ENGINEDIR=..\..\..\game"

rem ==  Set the Path to your mod's root source code ==
rem This should already be correct, accepts relative paths only!
set SOURCEDIR=..\..

set "targetdir=..\..\..\game\hl2\shaders"

set BUILD_SHADER=call buildshaders.bat

rem Use dynamic shaders to build .inc files only
set dynamic_shaders=1

rem ==== PATH CONFIGURATIONS END ====
rem =================================

set ARG_EXTRA=


REM ****************
REM BUILD SHADERS
REM ****************
@REM shove the full log into another file 
%BUILD_SHADER% stdshader_dx8_1x			-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders%
echo --------------------------------------------------------------------------------------------
%BUILD_SHADER% stdshader_dx9_20b_nointercept		-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders%
echo --------------------------------------------------------------------------------------------
%BUILD_SHADER% stdshader_dx9_20b		-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders%
@REM %BUILD_SHADER% stdshader_dx9_20b_testing	-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders% >buildallshaders_stdshader_dx9_20b.txt
echo --------------------------------------------------------------------------------------------
@REM %BUILD_SHADER% stdshader_dx9_30_test			-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders% -dx9_30 -force30 >buildallshaders_stdshader_dx9_30.txt
%BUILD_SHADER% stdshader_dx9_30			-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders% -dx9_30 -force30
echo --------------------------------------------------------------------------------------------
@REM %BUILD_SHADER% stdshader_dx10			-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders% -dx10
@REM dx10 is empty right now
echo.


REM ****************
REM END
REM ****************
:end

rem echo.
if not "%dynamic_shaders%" == "1" (
  echo Finished full buildallshaders %*
) else (
  echo Finished dynamic buildallshaders %*
)

%TTEXE% -diff %tt_all_start% -cur
echo.

pause