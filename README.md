# Quiver
Modified Version of the Source 2007 Leak

<<<<<<< HEAD
MAKE SURE TO PULL THIS REPOSITORY WITH RECURSIVE SUBMODULES FOR HL2R

This uses v121 compilers (VS2013)
=======
**Pull this repository with recursive submodules (`git clone --recursive`) to clone hl2r**

## Building

### Linux

So far Quiver builds successfully on Windows using Visual Studio 2013. Linux builds have been tested and are confirmed not to work at this point. A goal of this project is to maintain cross-platform support, so \*nix and OS X builds should be supported at some point.

### Windows
>>>>>>> master

1. run createlibprojects.bat and compile lib files in RELEASE ONLY

2. Open createhl2r.bat and compile all in release only (need to change this, as this is dumb as hell, only to compile shadercompiler.exe / .dll)

3. Install Perl if you haven't (https://www.perl.org/get.html)

4. compile buildallshaders.bat in materialsystem\stdshaders\

5. run createeverything.bat or createbinprojects.bat and compile all in release only

then go into game and edit gameinfo.txt in the game you want to run to include half-life 2's vpk's

<<<<<<< HEAD
then open run_mod_hl2.bat or run_hl2r.bat, or just use Run Game.bat to launch the game you want, whatever you want really (change this dumb thing).
=======
then open run\_mod\_hl2.bat or run\_hl2r.bat, or just use Run Game.bat to launch the game you want, whatever you want really (change this dumb thing).
>>>>>>> master
