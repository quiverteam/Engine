# Quiver
Custom Source Engine branch based on Source Engine 2007, currently available on Windows in x86.

**Pull this repository with recursive submodules (`git clone --recursive`) to clone hl2r**

## Building

### Windows

1. Install [Perl](http://strawberryperl.com/).

2. Open Perl Command Line and enter `cpan String::CRC32`

3. Install [Python](https://www.python.org/) (Optional).

4. Run createcoreprojects.bat and build the solution in Release.

5. Run src/materialsystem/buildallshaders.bat.

6. Run src/createallprojects.bat or src/createbinprojects.bat and build the solution in Release.

7. Edit game/make_dir_junction.bat with your HL2 install directory if needed and Run

You can run the engine with run_mod_hl2.bat, run_mod_episodic.bat, or run_hl2r.bat.

### Linux

Linux support is unavailable at the moment, you can help. See Supporting Non-Windows Platforms.

### macOS
macOS support is unavailable at the moment, you can help. See Supporting Non-Windows Platforms.

## How you can help

### Supporting Non-Windows Platforms
The Source Engine was built heavily Windows-oriented, as it uses DirectX as it's graphics API which is only available on Windows, Xbox, Xbox 360 and the Xbox One. This is the major obstacle in supporting other platforms. To bring support to other platforms; we need an experienced graphics programmer who can work with OpenGL or Vulkan as these graphics APIs are universal, other than that some platform-specific code may be required.

### Other

Our [Trello](https://trello.com/b/WaxlL3kb/quiver-engine)
