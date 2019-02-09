@echo off
title Choose a Game
:choose
set /p name=Please choose a game: 

if "%name%"=="" goto invalid
if "%name%"=="hl2" goto hl2
if "%name%"=="episodic" goto episodic
if "%name%"=="lostcoast" goto lostcoast
if "%name%"=="ep2" goto ep2
if "%name%"=="portal" goto portal
if "%name%"=="tf" goto tf
if "%name%"=="cstrike" goto cstrike
if "%name%"=="hl1" goto hl1
if "%name%"=="hl1mp" goto hl1mp
if "%name%"=="dod" goto dod

:invalid
echo '%name%' is not a game!
goto choose

:hl2
start hl2.exe -game hl2 -windowed
exit

:episodic
start hl2.exe -game episodic -windowed
exit

:lostcoast
start hl2.exe -game ep2 -windowed
exit

:ep2
start hl2.exe -game lostcoast -windowed
exit

:portal
start hl2.exe -game portal -windowed
exit

:tf
start hl2.exe -game tf -windowed
exit

:cstrike
start hl2.exe -game cstrike -windowed
exit

:hl1
start hl2.exe -game hl1 -windowed
exit

:hl1mp
start hl2.exe -game hl1mp -windowed
exit

:dod
start hl2.exe -game dod -windowed
exit