# Quiver
Custom Source Engine branch based on Source Engine 2007, currently available on Windows in x86.

**Pull this repository with recursive submodules (`git clone --recursive`) to clone hl2r**

## Building

### Windows

1. Install [Visual Studio 2019 Community](https://visualstudio.microsoft.com/downloads/), Make sure to go to Individual components and install `C++ MFC for v142 build tools (x86 and x64)`

2. Install [Perl](http://strawberryperl.com/), open Perl Command Line and enter `cpan String::CRC32`. This is needed for building shaders.

3. Run src/createallprojects.bat and build the solution in Release.

4. Run src/materialsystem/stdshaders/buildallshaders.bat, then build stdshader projects.

5. Edit game/make_dir_junction.bat with your HL2 install directory if needed and Run. This is used so you don't add absolute paths into the gameinfo.txt file for loading assets.

You can run the engine with run_mod_hl2.bat or run_hl2r.bat.

NOTE: not all projects build at the moment.

### Linux

Currently, Linux support is incomplete, but regardless, if you wish to build under Linux, you'll need tcmalloc, SDL2 and CMake.
Installing these on Debian is pretty straight forward:
```sh
sudo apt install libsdl2-dev:i386 libgoogle-perftools-dev:i386 cmake
```
If you'd like to build Quiver for x64, you will need to install the 64-bit versions of those packages too.

Now to build Quiver:
```
mkdir build && cd build
cmake ../ <your options here> && make
```
To see a list of the available build options, do `cmake --help` in the root directory.

### MacOS
Currently no support for MacOS is planned. Apple doesn't make it easy to support Macs anymore.

## How you can help

### Supporting Non-Windows Platforms
The Source Engine was built heavily Windows-oriented, as it uses DirectX as it's graphics API which is only available on Windows, Xbox, Xbox 360 and the Xbox One. This is the major obstacle in supporting other platforms. To bring support to other platforms; we need an experienced graphics programmer who can work with OpenGL or Vulkan as these graphics APIs are universal, other than that some platform-specific code may be required.

### Other

Our [Trello](https://trello.com/b/WaxlL3kb/quiver-engine)
