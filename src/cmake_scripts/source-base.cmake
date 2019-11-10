#
#
# Source base with CMake
#
#
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

if(DEFINED POSIX OR CLANG_BUILD)
	if(NOT BUILD_64BIT)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32" CACHE STRING "c++ flags" FORCE)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32" CACHE STRING "c flags" FORCE)
		set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -m32")
	endif(NOT BUILD_64BIT)
	
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -march=i686 -msse3")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math -march=i686 -msse3")
endif()

if(DEFINED UNIX_CROSS)
	# Why the FUCK does valve insist on casting const char to char??????
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-attributes -Wno-write-strings")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes -Wno-write-strings")
	# Clean up the prefixes
	set(CMAKE_STATIC_LIBRARY_PREFIX "")
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
	set(CMAKE_IMPORT_LIBRARY_PREFIX "")
endif(DEFINED UNIX_CROSS)

if(DEFINED POSIX OR UNIX_CROSS)
	set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -lc")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -Wno-narrowing -Wno-enum-compare -Wno-format-security -Wno-multichar")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpermissive -Wno-narrowing -Wno-enum-compare -Wno-format-security -Wno-multichar")
	if(DEFINED POSIX64)
		set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -l:ld-linux-x86_64.so.2")
		set(CMAKE_LIBRARY_PATH "/usr/lib/x86_64-linux-gnu/")
	else()
		set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -l:ld-linux.so.2")
		set(CMAKE_LIBRARY_PATH "/usr/lib/i386-linux-gnu/")
	endif(DEFINED POSIX64)
endif()
