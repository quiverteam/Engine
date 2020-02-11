# Quiver
Custom Source Engine branch based on Source Engine 2007, currently available on Windows in x86.

**Pull this repository with recursive submodules (`git clone --recursive`) to clone qpc and game/core folder**

## Building

### Windows

1. Install [Visual Studio 2019 Community](https://visualstudio.microsoft.com/downloads/), Make sure to go to Individual components and install `C++ MFC for v142 build tools (x86 and x64)`

2. Install [Python](https://www.python.org/downloads/), minimum version is 3.6, and make sure to check the "py launcher" option when installing

3. Run src/\_create\_allprojects.bat

4. Open the solution created, and build the "Game" folder in the solution in Release - Win32, or more projects if you want.

#### OPTIONAL - Building Shaders:

1. Install [Perl](http://strawberryperl.com/), open Perl Command Line and enter `cpan String::CRC32`.

2. (Only if you didn't build shadercompile and dx\_proxy yet) Run src/\_create_coreprojects.bat and build the solution in Release - Win32.

3. Build shaders by running src/materialsystem/stdshaders/\_buildallshaders.bat.

4. Build stdshader_xxx projects in Visual Studio again.

### Linux and MacOS

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

If you want to build the Vulkan Shader API, you'll need to install the LunarG Vulkan SDK, as detailed [here](https://vulkan.lunarg.com/doc/sdk/1.1.108.0/linux/getting_started_ubuntu.html). You will also need to install the following package:
```
sudo apt install libvulkan1:i386
```

### MacOS
Currently no support for MacOS is planned. Apple doesn't make it easy to support Macs anymore.

## Running the Game

### Windows
1. Edit game/make_dir_junction.bat with your HL2 install directory if needed and Run. This is used so you don't need to add absolute paths into the gameinfo.txt file for loading assets.

2. Run the game with any one of the game/run\_mod\_x.bat files

### Linux and MacOS
Linux and MacOS support isn't finished, however, it is possible to download one of the auto builds and use Wine or Proton.

1. Either create system links to Half-Life 2/hl2, episodic, and ep2 to the game/ folder, or set the paths in the gameinfo.txt file

2. Install [Wine](https://wiki.winehq.org/Download) or use Valve's Wine fork, [Proton](https://gaming.stackexchange.com/a/348614)

- For Wine, run the game with any one of the game/run\_mod\_x.bat files

- For Proton, add game/bin/win32/quiver.exe as a non-steam game, and copy the arguments from any one of the run_mod_x.bat files into steam launch option, and start the game with proton

## How you can help

### Supporting Non-Windows Platforms
The Source Engine was built heavily Windows-oriented, as it uses DirectX as it's graphics API which is only available on Windows, Xbox, Xbox 360 and the Xbox One. This is the major obstacle in supporting other platforms. To bring support to other platforms; we need an experienced graphics programmer who can work with OpenGL or Vulkan as these graphics APIs are universal, other than that some platform-specific code may be required.

### Links

[Discord Server](https://discord.gg/b5ExdCu)

(unused) [Trello](https://trello.com/b/WaxlL3kb/quiver-engine)
