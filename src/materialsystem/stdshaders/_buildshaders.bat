@echo off

rem == Setup path to nmake.exe ==
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
	for /f "usebackq tokens=1* delims=: " %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop`) do (
		if /i "%%i"=="installationPath" (
			set VSDIR=%%j
			call "!VSDIR!\Common7\Tools\VsDevCmd.bat" >nul
			echo Using Visual Studio 2017+ nmake
			goto :start
		)
	)	
) else if exist "%VS140COMNTOOLS%vsvars32.bat" (
	call "%VS140COMNTOOLS%vsvars32.bat"
	echo Using Visual Studio 2015 nmake
	
) else if exist "%VS120COMNTOOLS%vsvars32.bat" (
	call "%VS120COMNTOOLS%vsvars32.bat"
	echo Using Visual Studio 2013 nmake
	
) else if exist "%VS100COMNTOOLS%vsvars32.bat" (
	call "%VS100COMNTOOLS%vsvars32.bat"
	echo Using Visual Studio 2010 nmake
	
) else (
	echo.
	echo Install Either Visual Studio Version 2010, 2013, 2015, or 2017
	pause
	exit
)

:start
set TTEXE=..\..\devtools\bin\timeprecise.exe
if not exist %TTEXE% goto no_ttexe
goto no_ttexe_end

:no_ttexe
set TTEXE=time /t
:no_ttexe_end

rem echo ==================== buildshaders %* ==================
%TTEXE% -cur-Q
set tt_start=%ERRORLEVEL%
set tt_chkpt=%tt_start%


REM ****************
REM usage: buildshaders <shaderProjectName>
REM ****************

setlocal
set arg_filename=%1
set platform=win32
set shadercompilecommand=shadercompile.exe
set targetdir=..\..\..\game\core\shaders
set SrcDirBase=..\..
set shaderDir=shaders
@REM your total thread count
set /A threadcount=%NUMBER_OF_PROCESSORS%
@REM this increases performance greatly
set force_compile=0

@REM should be removed, idk
set EngineBinDir=../../../game/bin/%platform%

if "%1" == "" goto usage
set inputbase=%1

set DIRECTX_SDK_VER=pc09.00
set DIRECTX_SDK_BIN_DIR=dx_proxy\dx9_00\%platform%

if /i "%8" == "-dx9_30" goto dx_sdk_dx9_30
if /i "%8" == "-dx10" goto dx_sdk_dx10
if /i "%8" == "-dx11" goto dx_sdk_dx11
goto dx_sdk_end
:dx_sdk_dx9_30
			set DIRECTX_SDK_VER=pc09.30
			set DIRECTX_SDK_BIN_DIR=dx_proxy\dx9_30\%platform%
			goto dx_sdk_end
:dx_sdk_dx10
			set DIRECTX_SDK_VER=pc10.00
			set DIRECTX_SDK_BIN_DIR=dx_proxy\dx10_40\%platform%
			goto dx_sdk_end
:dx_sdk_dx11
			set DIRECTX_SDK_VER=pc11.00
			set DIRECTX_SDK_BIN_DIR=dx_proxy\dx11_00\%platform%
			goto dx_sdk_end
:dx_sdk_end

if /i "%9" == "-force30" goto set_force30_arg
goto set_force_end
:set_force30_arg
			set DIRECTX_FORCE_MODEL=30
			goto set_force_end
:set_force_end

if /i "%7" == "1" set force_compile=1

if %force_compile%==1 (
	@REM force compile, azure pipelines perl is broken on windows-2019, so this is a work around
	set perl=C:\Strawberry\perl\bin\perl
) else (
	set perl=perl
)

if /i "%2" == "-game" goto set_mod_args
if /i "%6" == "1" set dynamic_shaders=1
goto build_shaders

REM ****************
REM USAGE
REM ****************
:usage
echo.
echo "usage: buildshaders <shaderProjectName> [-dx10 or -game] [gameDir if -game was specified] [-source sourceDir]"
echo "       gameDir is where gameinfo.txt is (where it will store the compiled shaders)."
echo "       sourceDir is where the source code is (where it will find scripts and compilers)."
echo "ex   : buildshaders myshaders"
echo "ex   : buildshaders myshaders -game c:\steam\steamapps\sourcemods\mymod -source c:\mymod\src"
goto :end

REM ****************
REM MOD ARGS - look for -game or the vproject environment variable
REM ****************
:set_mod_args

if not exist "%EngineBinDir%\shadercompile.exe" goto NoShaderCompile

if /i "%4" NEQ "-source" goto NoSourceDirSpecified
set SrcDirBase=%~5

REM ** use the -game parameter to tell us where to put the files
@REM set targetdir=%~3\shaders
@REM set SDKArgs=-nompi -nop4 -game "%~3"

if not exist "%~3\gameinfo.txt" goto InvalidGameDirectory
goto build_shaders

REM ****************
REM ERRORS
REM ****************
:InvalidGameDirectory
echo -
echo Error: "%~3" is not a valid game directory.
echo (The -game directory must have a gameinfo.txt file)
echo -
goto end

:NoSourceDirSpecified
echo ERROR: If you specify -game on the command line, you must specify -source.
goto usage
goto end

:NoShaderCompile
echo -
echo - ERROR: shadercompile.exe doesn't exist in %EngineBinDir%
echo -
goto end

REM ****************
REM BUILD SHADERS
REM ****************
:build_shaders

rem echo --------------------------------
rem echo %inputbase%
rem echo --------------------------------
REM make sure that target dirs exist
REM files will be built in these targets and copied to their final destination
if not exist %shaderDir% mkdir %shaderDir%
if not exist %shaderDir%\fxc mkdir %shaderDir%\fxc

REM Nuke some files that we will add to later.
if exist filelist.txt del /f /q filelist.txt
if exist filestocopy.txt del /f /q filestocopy.txt
if exist filelistgen.txt del /f /q filelistgen.txt
if exist inclist.txt del /f /q inclist.txt
if exist vcslist.txt del /f /q vcslist.txt
if exist uniquefilestocopy.txt del /f /q uniquefilestocopy.txt
if exist makefile.%inputbase% del /f /q makefile.%inputbase%

REM ****************
REM Generate a makefile for the shader project
REM ****************
echo Creating makefile for %inputbase%...
@REM should just have it setup to install String::CRC32 now
if %force_compile%==1 (
	echo Force Compiling Shaders, skipping crc check
	%perl% "%SrcDirBase%\devtools\bin\updateshaders_force.pl" -source "%SrcDirBase%" %inputbase%
) else (
	%perl% "%SrcDirBase%\devtools\bin\updateshaders.pl" -source "%SrcDirBase%" %inputbase%
)

REM ****************
REM Run the makefile, generating minimal work/build list for fxc files, go ahead and compile vsh and psh files.
REM ****************
rem nmake /S /C -f makefile.%inputbase% clean > clean.txt 2>&1
@REM echo Building inc files, asm vcs files, and VMPI worklist for %inputbase%...
echo Building makefile...
nmake /S /C -f makefile.%inputbase%

REM ****************
REM Copy the inc files to their target
REM ****************
if exist "inclist.txt" (
	echo Publishing shader inc files to target...
	%perl% %SrcDirBase%\devtools\bin\copyshaderincfiles.pl inclist.txt
)

REM ****************
REM Add the executables to the worklist.
REM ****************
if /i "%DIRECTX_SDK_VER%" == "pc09.00" (
	rem echo "Copy extra files for dx 9 std
)
if /i "%DIRECTX_SDK_VER%" == "pc09.30" (
	echo %SrcDirBase%\devtools\bin\d3dx9_33.dll >> filestocopy.txt
)
if /i "%DIRECTX_SDK_VER%" == "pc10.00" (
	echo %SrcDirBase%\devtools\bin\d3dx10_33.dll >> filestocopy.txt
)

echo %SrcDirBase%\%DIRECTX_SDK_BIN_DIR%\dx_proxy.dll >> filestocopy.txt

echo %EngineBinDir%\shadercompile.exe >> filestocopy.txt
echo %EngineBinDir%\shadercompile_dll.dll >> filestocopy.txt
echo %EngineBinDir%\vstdlib.dll >> filestocopy.txt
echo %EngineBinDir%\tier0.dll >> filestocopy.txt

REM ****************
REM Cull duplicate entries in work/build list
REM ****************
if exist filestocopy.txt type filestocopy.txt | %perl% "%SrcDirBase%\devtools\bin\uniqifylist.pl" > uniquefilestocopy.txt
if exist filelistgen.txt if not "%dynamic_shaders%" == "1" (
    echo Generating action list...
    copy filelistgen.txt filelist.txt >nul
)

REM ****************
REM Execute distributed process on work/build list
REM ****************
set shader_path_cd=%cd%
if exist "filelist.txt" if exist "uniquefilestocopy.txt" if not "%dynamic_shaders%" == "1" (
	@REM checking if shader compile is running
	call _kill_shadercompiler.bat
	
	echo Building shaders...
	cd /D %EngineBinDir%
	
	@REM %shadercompilecommand% -mpi_MaxWorkers %shadercompileworkers% -shaderpath "%shader_path_cd:/=\%" -allowdebug
	@REM -verbose -subprocess X
	@REM it now has commands for d3dcompiler settings, for now, it just uses the fastest setting, /Od, though it doesn't seem much faster tbh
	echo.
	%shadercompilecommand% /Od -nompi -threads %threadcount% -shaderpath "%shader_path_cd:/=\%" -allowdebug
	
	echo.
	cd /D %shader_path_cd%
)

@REM delete the temporary files
if exist filelist.txt del /f /q filelist.txt
if exist filestocopy.txt del /f /q filestocopy.txt
if exist filelistgen.txt del /f /q filelistgen.txt
if exist inclist.txt del /f /q inclist.txt
if exist vcslist.txt del /f /q vcslist.txt
if exist uniquefilestocopy.txt del /f /q uniquefilestocopy.txt
if exist makefile.%inputbase% del /f /q makefile.%inputbase%

REM ****************
REM PC Shader copy
REM Publish the generated files to the output dir using XCOPY
REM This batch file may have been invoked standalone or slaved (master does final smart mirror copy)
REM ****************
:DoXCopy
if not "%dynamic_shaders%" == "1" (
	if not exist "%ENGINEDIR%\core\shaders" md "%ENGINEDIR%\core\shaders"
	xcopy "%cd%\shaders" "%cd%\%ENGINEDIR%\core\shaders" /q /e /y	
)
goto end

REM ****************
REM END
REM ****************
:end


%TTEXE% -diff %tt_start%