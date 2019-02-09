@echo off
title VPC Complete Project File Generator
REM // This gets inputs from the user, and puts an output corresponding to it
REM // Then it dumps everything into the vpc command line, which you see a preview of it before you run it

:choose
set /p mksln=Create New Solution File? (y or nothing): 
if "%mksln%" == "y" (set /p sln_name=Enter a name for the solution file: )
if "%mksln%" == "y" (set sln_name="%sln_name%" )
if "%mksln%" == "y" ( set mksln=/mksln )

set /p win64=Use win64 compiler? (y or nothing for win32): 
REM maybe use the same system for groups and projects for vsversion?
set /p vsver=Choose a VS version (nothing for 2013): 
set /p force=Force Rebuild all projects? (y or nothing): 

REM Groups
echo =============================================
echo Choose groups to build:
echo 	Nothing for everything
echo 	1 - binary files
echo 	2 - libraries
echo 	3 - gamedlls
echo 	4 - hammer
echo 	5 - shaders
echo 	6 - physics
echo 	7 - tools
echo 	8 - game
echo Or type in 0 to type in a group: 
echo -------------------------------------
echo (no spaces allowed, commas not needed)
set /p group="Selections: "

REM ==========================================
REM Projects
echo =============================================
echo Choose projects to build:
echo 	Nothing for all default projects
echo 	0 - no projects
echo 	1 - hl2
echo 	2 - hl2mp
echo 	3 - episodic
echo Or type in + to type in a project: 
echo -------------------------------------
echo (no spaces allowed, commas not needed)
set /p project="Selections: "

REM ========================================================================================================================
:select_compiler
if "%win64%" == "y" ( goto win64 ) else goto select_vsversion

:win64
set win64=/define:WIN64 
set dp=/dp 
goto select_vsversion

:select_vsversion
if "%vsver%" == "2017" ( set vsver=/define:2017 
) else if "%vsver%" == "2015" ( set vsver=/define:2015 
) else if "%vsver%" == "2012" ( set vsver=/2012 
) else if "%vsver%" == "2010" ( set vsver=/2010 
) else goto select_force

:select_force
if "%force%" == "y" ( set force=/f & goto select_group_everything
) else goto select_group_everything

REM ============================================================
REM groups

:select_group_everything
REM checks if the variable is empty
if [%group%] == [] (set groups=+everything & goto select_project_default
) else goto select_group_0

:select_group_0
REM for some reason, when you select a project, this doesnt work
REM this searches for the character 0 in %group%, and goes to group_custom if it finds it, else it goes to select_group_1
(echo %group% | findstr /i /c:"0" >nul) && (goto group_add) || (goto select_group_1)

:select_group_1
REM this searches for the character 1 in %group%, and goes to group_1 if it finds it, else it goes to select_group_2
(echo %group% | findstr /i /c:"1" >nul) && (set grp_01=+bin ) || (goto select_group_2)

:select_group_2
(echo %group% | findstr /i /c:"2" >nul) && (set grp_02=+libraries ) || (goto select_group_3)

:select_group_3
(echo %group% | findstr /i /c:"3" >nul) && (set grp_03=+gamedlls ) || (goto select_group_4)

:select_group_4
(echo %group% | findstr /i /c:"4" >nul) && (set grp_04=+hammer ) || (goto select_group_5)

:select_group_5
(echo %group% | findstr /i /c:"5" >nul) && (set grp_05=+shaders ) || (goto select_group_6)

:select_group_6
(echo %group% | findstr /i /c:"6" >nul) && (set grp_06=+physics ) || (goto select_group_7)

:select_group_7
(echo %group% | findstr /i /c:"7" >nul) && (set grp_07=+tools ) || (goto select_group_8)

:select_group_8
(echo %group% | findstr /i /c:"8" >nul) && (set grp_08=+game ) || (goto group_combine)

:group_add
echo ------------------------------
echo Enter project groups you want
echo Make sure each group looks like this: +example
echo And space each word out

set /p grp_add=Groups: 
echo ------------------------------
goto select_group_1

:group_combine
REM combines all project values into one
call set groups=%grp_01%%grp_02%%grp_03%%grp_04%%grp_05%%grp_06%%grp_07%%grp_08%%grp_add% 
goto select_project_default

REM ========================================================================================================================
REM projects

:select_project_default
REM this is for default projects
if [%project%] == [] (set projects=/hl2 /hl2mp /episodic & goto createprojects
) else goto select_project_add

:select_project_add
REM this is for adding custom projects
(echo %project% | findstr /i /c:"+" >nul) && (goto project_add) || (goto select_project_0)

:project_add
echo ------------------------------
echo Enter projects you want
echo Make sure each project looks like this: /example 
echo And space each word out

set /p proj_add=Projects: 
echo ------------------------------
goto select_project_1

:select_project_0
REM this is for no projects
(echo %project% | findstr /i /c:"0" >nul) && (goto project_combine) || (goto select_project_1)

:select_project_1
(echo %project% | findstr /i /c:"1" >nul) && (set proj_01=/hl2 ) || (goto select_project_2)

:select_project_2
(echo %project% | findstr /i /c:"2" >nul) && (set proj_02=/hl2mp ) || (goto select_project_3)

:select_project_3
(echo %project% | findstr /i /c:"3" >nul) && (set proj_03=/episodic ) || (goto project_combine)

:project_combine
REM combines all project values into one
call set projects=%proj_01%%proj_02%%proj_03%%proj_add% 
goto createprojects

REM ========================================================================================================================
:createprojects
call set complete_vpc=%groups%%force%%projects%%mksln%%sln_name%%vsver%%win64%%dp%
echo =============================================
echo Current VPC command line:
echo %complete_vpc%

pause
echo =============================================
devtools\bin\vpc %complete_vpc%
pause