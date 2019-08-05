@echo off

echo Checking if needed files exist
echo.

echo current dir: %cd%
echo.

set engine_bin=%cd%\..\..\..\game\bin\win32\

echo engine_bin: %engine_bin%
if exist %engine_bin% (
	echo Exists: True
) else (
	echo Exists: False
)
echo.

@REM check for shadercompile
set shadercompile=%engine_bin%\shadercompile.exe

echo shadercompile: %shadercompile%
if exist %shadercompile% (
	echo Exists: True
) else (
	echo Exists: False
)
echo.

@REM check dx_proxy
set srcdir=%cd%\..\..
set dx_proxy_20=%srcdir%\dx_proxy\dx9_00\win32\dx_proxy.dll

echo dx_proxy_20: %dx_proxy_20%
if exist %dx_proxy_20% (
	echo Exists: True
) else (
	echo Exists: False
)
echo.

set dx_proxy_30=%srcdir%\dx_proxy\dx9_30\win32\dx_proxy.dll

echo dx_proxy_30: %dx_proxy_30%
if exist %dx_proxy_30% (
	echo Exists: True
) else (
	echo Exists: False
)
echo.


echo.
pause
