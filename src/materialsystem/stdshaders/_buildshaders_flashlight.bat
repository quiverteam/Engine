@echo off
setlocal enabledelayedexpansion
echo.

:start
set TTEXE=..\..\devtools\bin\timeprecise.exe
if not exist %TTEXE% goto no_ttexe
goto no_ttexe_end

:no_ttexe
set TTEXE=time /t
:no_ttexe_end


echo.
echo ~~~~~~~~~~~~~~~~~~~~~~~~ buildshaders_flashlight ~~~~~~~~~~~~~~~~~~~~~~~~
%TTEXE% -cur-Q
set tt_all_start=%ERRORLEVEL%
set tt_all_chkpt=%tt_start%

rem ===================================
rem ====== LAUNCH CONFIGURATIONS ======

rem == Set the absolute path to your mod's game directory here ==
@REM change this to the core folder (platform atm)
set GAMEDIR=%cd%\..\..\..\game\mod_hl2

rem == Set the relative or absolute path to the bin folder ==
set "ENGINEDIR=..\..\..\game"

rem ==  Set the Path to your mod's root source code ==
rem This should already be correct, accepts relative paths only!
set SOURCEDIR=..\..

set "targetdir=..\..\..\game\core\shaders"

set BUILD_SHADER=call _buildshaders.bat

@REM dynamic shaders only builds the required files (inc) to build stdshader_dx*.dll
set dynamic_shaders=0

rem ==== LAUNCH CONFIGURATIONS END ====
rem ===================================

REM ****************
REM PC SHADERS
REM ****************
@REM %BUILD_SHADER%  _shaderlist_flashlight_dx9_20b		-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders%
echo --------------------------------------------------------------------------------------------
%BUILD_SHADER% _shaderlist_flashlight_dx9_30		-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders% -dx9_30 -force30
echo --------------------------------------------------------------------------------------------
@REM %BUILD_SHADER% stdshader_flashlight_dx10			-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders% -dx10
@REM dx10 is empty right now
echo.

REM ****************
REM PC Shader copy
REM Publish the generated files to the output dir using XCOPY
REM This batch file may have been invoked standalone or slaved (master does final smart mirror copy)
REM ****************
:DoXCopy
if not "%dynamic_shaders%" == "1" (
	if not exist "%targetdir%" md "%targetdir%"
	xcopy "%cd%\shaders" "%cd%\%targetdir%\" /q /e /y	
)
goto end

REM ****************
REM END
REM ****************
:end

rem echo.
if not "%dynamic_shaders%" == "1" (
  echo Finished full buildshaders_flashlight
) else (
  echo Finished dynamic buildshaders_flashlight
)

%TTEXE% -diff %tt_all_start% -cur
echo.
echo Press any key to rebuild stdshader projects and exit . . .
pause >nul

..\..\devtools\bin\vpc.exe /f /define:VS2019 +stdshaders