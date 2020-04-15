#
#
# Project script for filesystem
#
#
project(filesystem_stdio C CXX)

# Platform specific sources 
set(WINDOWS_SRCS	)
set(POSIX_SRCS		"linux_support.cpp")

set(SRCS			"basefilesystem.cpp"
		"filetracker.cpp"
		"filesystem_async.cpp"
		"filesystem_stdio.cpp"
		"${ROOT}/public/kevvaluescompiler.cpp"
		"${ROOT}/public/zip_utils.cpp"
		"QueuedLoader.cpp")

# Preprocessor defs
set(DEFINES			-DFILESYSTEM_STDIO_EXPORTS
					-DDONT_PROTECT_FILEIO_FUNCTIONS
					-DPROTECTED_THINGS_ENABLE
					-D_USE_32BIT_TIME_T)

# Platform-specific preprocessor defines
set(WIN32_DEFINES	)	
set(POSIX32_DEFINES	)
set(WIN64_DEFINES	)
set(POSIX64_DEFINES	)
set(WINDOWS_DEFINES	)
set(POSIX_DEFINES	)

set(PLATFORM_64BIT_DEFINES	)
set(PLATFORM_32BIT_DEFINES 	)

# Links dirs to search for link libs in
set(LINK_DIRS	)
set(WINDOWS_LINK_DIRS	)
set(POSIX_LINK_DIRS		)
set(WIN32_LINK_DIRS		)
set(WIN64_LINK_DIRS		)
set(POSIX32_LINK_DIRS	)
set(POSIX64_LINK_DIRS	)

# Generic libs to link against
set(LINK_LIBS			)

# Other projects this one depends on
set(DEPENDENCIES tier1 tier0 tier2 vstdlib)

# Platform specific link libs
set(WINDOWS_LINK_LIBS	)
set(WIN32_LINK_LIBS		)
set(WIN64_LINK_LIBS		)
set(POSIX32_LINK_LIBS	)
set(POSIX64_LINK_LIBS	)

# Generic include dirs
set(INCLUDE_DIRS		${PUBLIC_INCLUDE}/tier1 
						${PUBLIC_INCLUDE}/tier0
						${PUBLIC_INCLUDE}/vstdlib
						${PUBLIC_INCLUDE}/mathlib
						${PUBLIC_INCLUDE}/filesystem)

# Platform specific includes
set(WIN32_INCLUDE_DIRS	)
set(WIN64_INCLUDE_DIRS	)
set(POSIX32_INCLUDE_DIRS	)
set(POSIX64_INCLUDE_DIRS	)

# Set the variable of target for stuff
set(TARGET filesystem_stdio)

# Set the output file name
set(OUTPUT_FILE_NAME filesystem_stdio)

# If this is a shared lib
set(SHARED_LIB 1)

# If this is a static lib
set(STATIC_LIB 0)

# Include this to handle all the defines
include(${CMAKESCRIPTS_DIR}/library-base.cmake)
