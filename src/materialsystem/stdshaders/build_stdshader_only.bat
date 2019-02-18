@echo off
setlocal

rem Use dynamic shaders to build .inc files only
rem set dynamic_shaders=1
rem == Setup path to nmake.exe, from vc 2013 common tools directory ==
rem this batch file doesn't exist in vs2017
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"

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

rem ==== PATH CONFIGURATIONS END ====
rem =================================

set ARG_EXTRA=

@REM dynamic shaders only builds the required files (inc) to build stdshader_dx*.dll
set dynamic_shaders=1

REM ****************
REM PC SHADERS
REM ****************
%BUILD_SHADER% stdshader_dx9_20b	-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders%
echo --------------------------------------------------------------------------------------------
@REM %BUILD_SHADER% stdshader_dx9_20b_new	-game %GAMEDIR% -source %SOURCEDIR% -dx9_30
%BUILD_SHADER% stdshader_dx9_20b_new	-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders% -dx9_30
echo --------------------------------------------------------------------------------------------
%BUILD_SHADER% stdshader_dx9_30		-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders% -dx9_30	-force30
echo --------------------------------------------------------------------------------------------
echo.
@REM %BUILD_SHADER% stdshader_dx10     	-dx10
@REM dx10 is empty right now


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