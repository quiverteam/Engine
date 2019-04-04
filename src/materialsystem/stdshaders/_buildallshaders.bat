@echo off
setlocal enabledelayedexpansion

:start
set TTEXE=..\..\devtools\bin\timeprecise.exe
if not exist %TTEXE% goto no_ttexe
goto no_ttexe_end

:no_ttexe
set TTEXE=time /t
:no_ttexe_end

rem ===================================
rem ====== LAUNCH CONFIGURATIONS ======

set "GAMEDIR=%cd%\..\..\..\game\mod_hl2"
set "ENGINEDIR=..\..\..\game"
set "SOURCEDIR=..\.."
set "BUILD_SHADER=call _buildshaders.bat"

@REM dynamic shaders only builds the required files (inc) to build stdshader_*.dll
set dynamic_shaders=0

rem ==== LAUNCH CONFIGURATIONS END ====
rem ===================================

echo ==============================================================================
echo.
echo Building All Shaders
echo.

%TTEXE% -cur-Q
set tt_all_start=%ERRORLEVEL%
set tt_all_chkpt=%tt_start%

REM ****************
REM BUILD SHADERS
REM ****************
@REM  >_log_shaderlist_dx9_20b.log
echo --------------------------------------------------------------------------------------------
%BUILD_SHADER% _shaderlist_dx9_20b				-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders%
echo --------------------------------------------------------------------------------------------
%BUILD_SHADER% _shaderlist_dx9_30				-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders% -dx9_30 -force30
echo --------------------------------------------------------------------------------------------
@REM eventually, only use _shaderlist_dx9, it will have all shaders as shader model 3 in it only

@REM %BUILD_SHADER% _shaderlist_dx10			-game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders% -dx10
echo.

REM ****************
REM END
REM ****************
:end

rem echo.
if not "%dynamic_shaders%" == "1" (
  echo Finished Building All Shaders %*
) else (
  echo Finished Building All Shaders - Dynamic %*
)

%TTEXE% -diff %tt_all_start% -cur

echo.
echo ==============================================================================
echo.
%TTEXE% -diff %tt_all_start% -cur
echo.
echo Press any key to rebuild stdshader projects and exit . . .
pause >nul

..\..\devtools\bin\vpc.exe /f +stdshaders