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

set "GAMEDIR=%cd%\..\..\..\game\hl2"
set "ENGINEDIR=..\..\..\game"
set "SOURCEDIR=..\.."
set "BUILD_SHADER=call %cd%\_buildshaders.bat"

@REM dynamic shaders only builds the required files (inc) to build stdshader_*.dll
set dynamic_shaders=0

rem ==== LAUNCH CONFIGURATIONS END ====
rem ===================================

echo ==============================================================================
echo.
echo Building All DX11 Shaders
echo.

%TTEXE% -cur-Q
set tt_all_start=%ERRORLEVEL%
set tt_all_chkpt=%tt_start%

REM ****************
REM BUILD SHADERS
REM ****************
@REM  >_log_shaderlist_dx11.log
echo --------------------------------------------------------------------------------------------
%BUILD_SHADER% _shaderlist_dx11				    -game %GAMEDIR% -source %SOURCEDIR% %dynamic_shaders% 0 -dx11
echo --------------------------------------------------------------------------------------------
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

py ..\..\devtools\qpc\qpc.py -d "../.."  -a stdshader_dx9 stdshader_dbg -t vstudio -f

