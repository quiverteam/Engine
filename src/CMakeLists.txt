#
#
# Base project scripts
#
#
cmake_minimum_required(VERSION 3.5)
project(Quiver)

#set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

option(USE_DX9 "Enables DX9 builds" ON)
option(USE_TOGL "Enables ToGL builds" OFF)
option(USE_OPENGL "Build with OpenGL" OFF)
option(USE_LIBC_FOR_SIMD "Force use C standard library for SSE" OFF)
option(USE_SSE "Enables the use of SSE all the way up to SSE3" ON)
option(USE_AVX "Enables the use of AVX vectorization and intrinsics. Experiemental." OFF)
option(UNIX_CROSS "Enables the use of Windows builds on Unix" OFF)
option(CLANG_BUILD "Enables the use of Clang instead of MSVC/GCC" OFF)
option(DEV_BUILD "Enables development builds" ON)
option(SERVER_BUILD "Specifies this build as a server build" OFF)
option(BUILD_64BIT "Specifies this build as 64-bit. Experiemental." OFF)
option(RELEASE "Enable this to build as a release build" OFF)
option(GAMES_ONLY "Build only the game projects and required utils" OFF)
option(BUILD_EVERYTHING "Build everything" ON)
option(HL2 "Build HL2 game stuff" ON)
option(EP1 "Build HL2:EP1 game stuff" ON)
option(HL2MP "Build HL2MP game stuff" ON)
option(HL2R "Build HL2:Revamped game stuff" ON)
option(POSIX_UNSUPPORT "Enable project generation for things not quite ported to posix yet" OFF)
option(CLANG "Enables the use of the clang toolchain instead of the default GNU toolchain (Only for POSIX)" OFF)

# Definitions that should always be enabled
# TODO: Will NO_MALLOC_OVERRIDE work on windows?
add_definitions(-DFRAME_POINTER_OMISSION_DISABLED -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_ALLOW_RUNTIME_LIBRARY_MISMATCH -D_ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH -D_HAS_ITERATOR_DEBUGGING=0)


set(MINGW32_TOOLCHAIN "${CMAKE_HOME_DIRECTORY}/cmake_scripts/toolchain-win32-mingw.cmake")
set(MINGW64_TOOLCHAIN "${CMAKE_HOME_DIRECTORY}/cmake_scripts/toolchain-win64-mingw.cmake")

if(CLANG)
	set(CLANG_BUILD ON)
	set(POSIX64_TOOLCHAIN "${CMAKE_HOME_DIRECTORY}/cmake_scripts/toolchain-posix64-clang.cmake")
	set(POSIX32_TOOLCHAIN "${CMAKE_HOME_DIRECTORY}/cmake_scripts/toolchain-posix32-clang.cmake")
else()
	set(CLANG_BUILD OFF)
	set(POSIX64_TOOLCHAIN "${CMAKE_HOME_DIRECTORY}/cmake_scripts/toolchain-posix64-gnu.cmake")
	set(POSIX32_TOOLCHAIN "${CMAKE_HOME_DIRECTORY}/cmake_scripts/toolchain-posix32-gnu.cmake")
endif(CLANG)

# Warn about clang weirdness on source
if(CLANG_BUILD)
	message(WARNING "Clang builds are experimental, use at your own risk!")
	add_definitions(-DCLANG -DCOMPILER_CLANG)
	set(CMAKE_GENERATOR_TOOLSET "llvm")
endif(CLANG_BUILD)

# Set dedicated server vars
if(SERVER_BUILD)
	add_definitions(-DDEDICATED)
endif(SERVER_BUILD)

# Make sure the things are good
if(DEFINED CMAKE_HOST_UNIX)
	if(NOT USE_TOGL OR NOT USE_OPENGL AND NOT UNIX_CROSS)
		#message(WARNING "ToGL or OpenGL is not enabled and you're building for Unix. ToGL is enabled by default now")
		set(USE_TOGL ON)
	endif(NOT USE_TOGL OR NOT USE_OPENGL AND NOT UNIX_CROSS)
endif(DEFINED CMAKE_HOST_UNIX)

if(NOT DEV_BUILD)
	add_definitions(-DDEV_BUILD)
endif(NOT DEV_BUILD)


# Platform specific
if(NOT BUILD_64BIT)
	add_definitions(-DPLATFORM_32BITS -D_x86_ -D_X86_)
else()
	add_definitions(-DPLATFORM_64BITS -D_x64_ -D_X64_ -D_x86_64_)
endif(NOT BUILD_64BIT)

# SIMD
if(USE_LIBC_FOR_SIMD)
	add_definitions(-DUSE_STDC_FOR_SIMD)
	message(WARNING "You're using LIBC for SIMD functions. You will experience a noticable slowdown.")
endif(USE_LIBC_FOR_SIMD)

#
# WINDOWS
#
if(DEFINED CMAKE_HOST_WIN32 AND NOT UNIX_CROSS)
	add_definitions(-DCOMPILER_MSVC)
endif(DEFINED CMAKE_HOST_WIN32 AND NOT UNIX_CROSS)

#========================================================
# WINDOWS VARS
#========================================================
if(DEFINED CMAKE_HOST_WIN32 OR UNIX_CROSS)
	set(WINDOWS_BUILD 1)
	set(WINDOWS 1)
	add_definitions(-D_DLL_EXT=".dll")
	add_definitions(-D_WIN32 -DWIN32 -DWINDOWS -D_ALLOW_MSC_VER_MISMATCH)
	if(BUILD_64BIT)
		if(UNIX_CROSS)
			message("Building for MINGW64")
			set(CMAKE_TOOLCHAIN_FILE "${MINGW64_TOOLCHAIN}")
		else()
			message("Building for WIN64-MSVC")
		endif(UNIX_CROSS)
		message(WARNING "64-bit builds are experiemental!")
		set(WIN64_BUILD 1)
		set(PLATFORM_64BITS 1)
		add_definitions(-DWIN64 -D_WIN64 -DPLATFORM_WINDOWS_PC64)
		if(NOT CLANG_BUILD AND NOT UNIX_CROSS) # Unix cross uses clang
			add_definitions(-DCOMPILER_MSVC64)
		endif(NOT CLANG_BUILD)
		set(PLATFORM_SUBDIR win64)
	else()
		if(UNIX_CROSS)
			message("Building for MINGW32")
			set(CMAKE_TOOLCHAIN_FILE "${MINGW32_TOOLCHAIN}")
		else()
			message("Building for WIN32-MSVC")
		endif(UNIX_CROSS)
		set(WIN32_BUILD 1)
		set(PLATFORM_32BITS 1)
		add_definitions(-DWIN32 -D_WIN32 -DPLATFORM_WINDOWS_PC32)
		if(NOT CLANG_BUILD AND NOT UNIX_CROSS)
			add_definitions(-DCOMPILER_MSVC32)
		endif(NOT CLANG_BUILD AND NOT UNIX_CROSS)
		set(PLATFORM_SUBDIR win32)
	endif(BUILD_64BIT)
endif(DEFINED CMAKE_HOST_WIN32 OR UNIX_CROSS)

#========================================================
# UNIX VARS
#========================================================
if(DEFINED CMAKE_HOST_UNIX AND NOT UNIX_CROSS)
	set(POSIX 1)
	set(LINUX 1)
	add_definitions(-D_DLL_EXT=".so")
	add_definitions(-DLINUX -D_LINUX -DPOSIX -D_POSIX)
	add_definitions(-DNO_MALLOC_OVERRIDE -DUSE_SDL -DDX_TO_GL_ABSTRACTION)
	if(NOT CLANG_BUILD)
		add_definitions(-DGNUC -DCOMPILER_GCC)
	endif(NOT CLANG_BUILD)
	if(BUILD_64BIT)
		message("Building for POSIX64")
		message(WARNING "64-bit builds are experiemental!")
		set(PLATFORM_64BITS 1)
		set(LINUX64 1)
		set(POSIX64 1)
		add_definitions(-DPLATFORM_64BITS -D_x64_ -D_X64_ -D__x86_64__)
		add_definitions(-DLINUX64 -D_LINUX64 -DPOSIX64 -D_POSIX64)
		set(PLATFORM_SUBDIR posix64)
		set(CMAKE_TOOLCHAIN_FILE "${POSIX64_TOOLCHAIN}")
	else()
		message("Building for POSIX32")
		set(PLATFORM_32BITS 1)
		set(LINUX32)
		set(POSIX32)
		add_definitions(-DPLATFORM_32BITS -D_x86_ -D_X86_)
		add_definitions(-DLINUX32 -D_LINUX32 -DPOSIX32 -D_LINUX32)
		set(PLATFORM_SUBDIR posix32)
		set(CMAKE_TOOLCHAIN_FILE "${POSIX32_TOOLCHAIN}")
	endif(BUILD_64BIT)
endif(DEFINED CMAKE_HOST_UNIX AND NOT UNIX_CROSS)

#
# Important fixes
#
if(NOT USE_AVX)
	# SDL2 will forcefully include AVX headers unless told not to.
	add_definitions(-DSDL_DISABLE_IMMINTRIN_H)
endif(NOT USE_AVX)


set(CMAKESCRIPTS_DIR ${CMAKE_HOME_DIRECTORY}/cmake_scripts/)
set(ROOT_DIR ${CMAKE_HOME_DIRECTORY})
set(PUBLIC ${ROOT_DIR}/public)
set(ROOT ${ROOT_DIR})
set(TOP ${ROOT_DIR})
set(UTILS_COMMON ${ROOT_DIR}/utils/common)
set(UTILS ${ROOT}/utils)
include(cmake_scripts/source-directories.cmake)
include(cmake_scripts/source-base.cmake)

# Choose the correct opengl library
set(OpenGL_GL_PREFERENCE "GLVND")

# All the project dirs!
add_subdirectory(mathlib)
add_subdirectory(tier0)
add_subdirectory(tier1)
add_subdirectory(tier2)
add_subdirectory(tier3)
add_subdirectory(tier4)
add_subdirectory(vstdlib)
add_subdirectory(filesystem)
add_subdirectory(appframework)
add_subdirectory(launcher)
add_subdirectory(launcher_main)
add_subdirectory(inputsystem)
add_subdirectory(vtf)
add_subdirectory(bitmap)
add_subdirectory(choreoobjects)
add_subdirectory(studiorender)
add_subdirectory(utils/bsppack)
add_subdirectory(materialsystem/shaderlib)
add_subdirectory(materialsystem)
add_subdirectory(materialsystem/shaderapiempty)
add_subdirectory(materialsystem/shaderapigl)
add_subdirectory(game)
add_subdirectory(engine)
add_subdirectory(networksystem)
add_subdirectory(utils/vrad)
add_subdirectory(utils/vrad_launcher)
add_subdirectory(dmxloader)
add_subdirectory(vgui2/matsys_controls)
add_subdirectory(vgui2/vgui_surfacelib)
add_subdirectory(tools/toolutils)

if(VULKAN)
	add_subdirectory(materialsystem/shaderapivk)
endif(VULKAN)

if(DEFINED WINDOWS_BUILD OR UNIX_CROSS)
	add_subdirectory(materialsystem/shaderapidx9)
	add_subdirectory(vgui2/vgui_controls)
	add_subdirectory(vgui2/src)
	add_subdirectory(hammer)
	add_subdirectory(hammer_launcher)
endif(DEFINED WINDOWS_BUILD OR UNIX_CROSS)

