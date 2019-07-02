#
#
# Base project scripts
#
#
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

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

# Definitions that should always be enabled
add_definitions(-DFRAME_POINTER_OMISSION_DISABLED)

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
		message(WARNING "ToGL or OpenGL is not enabled and you're building for Unix. ToGL is enabled by default now")
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

#
# WINDOWS VARS
#
if(DEFINED CMAKE_HOST_WIN32 OR UNIX_CROSS)
	set(WINDOWS_BUILD 1)
	if(NOT UNIX_CROSS)
		add_definitions(-D_DLL_EXT=".dll")
	endif(NOT UNIX_CROSS)
	if(BUILD_64BIT)
		message(WARNING "64-bit builds are experiemental!")
		set(WIN64_BUILD 1)
		set(PLATFORM_64BITS 1)
		add_definitions(-DWIN64 -D_WIN64 -DPLATFORM_WINDOWS_PC64)
		if(NOT CLANG_BUILD AND NOT UNIX_CROSS) # Unix cross uses clang
			add_definitions(-DCOMPILER_MSVC64)
		endif(NOT CLANG_BUILD)
		set(PLATFORM_SUBDIR win64)
	else()
		set(WIN32_BUILD 1)
		set(PLATFORM_32BITS 1)
		add_definitions(-DWIN32 -D_WIN32 -DPLATFORM_WINDOWS_PC32)
		if(NOT CLANG_BUILD AND NOT UNIX_CROSS)
			add_definitions(-DCOMPILER_MSVC32)
		endif(NOT CLANG_BUILD)
		set(PLATFORM_SUBDIR win32)
	endif(BUILD_64BIT)
endif(DEFINED CMAKE_HOST_WIN32 OR UNIX_CROSS)

# 
# UNIX VARS
#
if(DEFINED CMAKE_HOST_UNIX AND NOT UNIX_CROSS)
	set(POSIX 1)
	set(LINUX 1)
	add_definitions(-D_DLL_EXT=".so")
	add_definitions(-DLINUX -D_LINUX -DPOSIX -D_POSIX)
	if(NOT CLANG_BUILD)
		add_definitions(-DGNUC -DCOMPILER_GCC)
	endif(NOT CLANG_BUILD)
	if(BUILD_64BIT)
		message(WARNING "64-bit builds are experiemental!")
		set(PLATFORM_64BITS 1)
		set(LINUX64 1)
		set(POSIX64 1)
		add_definitions(-DPLATFORM_64BITS -D_x64_ -D_X64_)
		add_definitions(-DLINUX64 -D_LINUX64 -DPOSIX64 -D_POSIX64)
		set(PLATFORM_SUBDIR posix64)
	else()
		set(PLATFORM_32BITS 1)
		set(LINUX32)
		set(POSIX32)
		add_definitions(-DPLATFORM_32BITS -D_x86_ -D_X86_)
		add_definitions(-DLINUX32 -D_LINUX32 -DPOSIX32 -D_LINUX32)
		set(PLATFORM_SUBDIR posix32)
	endif(BUILD_64BIT)
endif(DEFINED CMAKE_HOST_UNIX AND NOT UNIX_CROSS)

set(CMAKESCRIPTS_DIR ${CMAKE_HOME_DIRECTORY}/cmake_scripts/)
set(ROOT_DIR ${CMAKE_HOME_DIRECTORY})

include(cmake_scripts/source-directories.cmake)
include(cmake_scripts/source-base.cmake)

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
#add_subdirectory(togl)
