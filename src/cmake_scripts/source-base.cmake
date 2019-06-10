#
#
# Source base with CMake
#
#
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

if(DEFINED POSIX OR CLANG_BUILD)
	if(NOT BUILD_64BIT)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
		set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -m32")
	endif(NOT BUILD_64BIT)
	
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -mcpu=core2 -msse3")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math -mcpu=core2 -msse3")
endif()

if(DEFINED POSIX)
	set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -lc")

	if(DEFINED POSIX64)
		set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -l:ld-linux-x86_64.so.2")
	else()
		set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -l:ld-linux.so.2")
	endif(DEFINED POSIX64)
endif()
