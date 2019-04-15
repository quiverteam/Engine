# Quiver
Custom Source Engine branch based on Source Engine 2007, currently available on Windows in x86.

**Pull this repository with recursive submodules (`git clone --recursive`) to clone hl2r**

## Building

### Windows

1. Install [Visual Studio 2019 Community](https://visualstudio.microsoft.com/downloads/), Make sure to go to Individual components and install `C++ MFC for v142 build tools (x86 and x64)`

2. Install [Perl](http://strawberryperl.com/), open Perl Command Line and enter `cpan String::CRC32`. This is needed for building shaders.

3. Run src/createallprojects.bat and build the solution in Release, Debug is broken currently.

4. Run src/materialsystem/stdshaders/buildallshaders.bat, then build stdshader projects.

5. Edit game/make_dir_junction.bat with your HL2 install directory if needed and Run. This is needed because the VPK support is a bit different from Source 2013, so if you have too many and/or too long search paths, you will hit the a limit in V_AppendSlash. This will hopefully be fixed soon.

You can run the engine with run_mod_hl2.bat or run_hl2r.bat.

NOTE: not all projects build at the moment.

### Linux and MacOS

Linux and MacOS support is currently being worked on in the posix-support branch. If you want to help, See Supporting Non-Windows Platforms.

## How you can help

### Supporting Non-Windows Platforms
The Source Engine was built heavily Windows-oriented, as it uses DirectX as it's graphics API which is only available on Windows, Xbox, Xbox 360 and the Xbox One. This is the major obstacle in supporting other platforms. To bring support to other platforms; we need an experienced graphics programmer who can work with OpenGL or Vulkan as these graphics APIs are universal, other than that some platform-specific code may be required.

### Other

Our [Trello](https://trello.com/b/WaxlL3kb/quiver-engine)
