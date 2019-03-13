# Quiver
Modified Version of the Source 2007 Leak

**Pull this repository with recursive submodules (`git clone --recursive`) to clone hl2r**

## Building

### Linux

So far Quiver builds successfully on Windows using Visual Studio 2013. Linux builds have been tested and are confirmed not to work at this point. A goal of this project is to maintain cross-platform support, so \*nix and OS X builds should be supported at some point.

### Windows

1. run createlibprojects.bat and compile lib files in RELEASE ONLY

2. Open createhl2r.bat and compile all in release only (need to change this, as this is dumb as hell, only to compile shadercompiler.exe / .dll)

3. Install Perl if you haven't (https://www.perl.org/get.html)

4. compile buildallshaders.bat in materialsystem\stdshaders\

5. run createeverything.bat or createbinprojects.bat and compile all in release only

then go into game and edit gameinfo.txt in the game you want to run to include half-life 2's vpk's

then open run\_mod\_hl2.bat or run\_hl2r.bat, or just use Run Game.bat to launch the game you want, whatever you want really (change this dumb thing).
