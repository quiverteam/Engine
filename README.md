# Quiver
Modified Version of the Source 2007 Leak

MAKE SURE TO PULL THIS REPOSITORY WITH RECURSIVE SUBMODULES FOR HL2R

This uses v121 compilers (VS2013)

1. run createlibprojects.bat and compile lib files in RELEASE ONLY

(need to change this, as this is dumb as hell, only to compile shadercompiler.exe / .dll)
2. Open createhl2r.bat and compile all in release only

3. Install Perl if you haven't (I use strawberry perl: http://strawberryperl.com/ )

4. compile buildallshaders.bat in materialsystem\stdshaders\

5. run createeverything.bat or createbinprojects.bat and compile all in release only

then go into game and edit gameinfo.txt in the game you want to run to include half-life 2's vpk's

then open run_mod_hl2.bat or run_hl2r.bat, or just use Run Game.bat to launch the game you want, whatever you want really (change this dumb thing).
