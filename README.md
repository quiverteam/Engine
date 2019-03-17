# Quiver
Modified version of Source Engine 2007

**Pull this repository with recursive submodules (`git clone --recursive`) to clone hl2r**

## Building

### Linux

So far Quiver builds successfully on Windows using Visual Studio 2013. Linux builds have been tested and are confirmed not to work at this point. A goal of this project is to maintain cross-platform support, so \*nix and OS X builds should be supported at some point.

### Windows

1. Run createlibprojects.bat and compile lib files in Release

2. Install [Perl](https://www.perl.org/get.html).

3. Run buildallshaders.bat in materialsystem/stdshaders

4. Run createallprojects.bat or createbinprojects.bat and build the solution in Release.

5. Run (and edit) Make Junctions.bat

You can run the engine with run_mod_hl2.bat or run_mod_episodic.bat
