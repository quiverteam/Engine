#
#
# Project script for ${PROJECT}
#
#
project(${PROJECT} C CXX)

# Platform specific sources 
set(WINDOWS_SRCS	)
set(POSIX_SRCS	)

# Generic sources
set(SRCS	)

# Preprocessor defs
set(DEFINES	)

# Platform-specific preprocessor defines
set(WIN32_DEFINES	)	
set(POSIX32_DEFINES	)
set(WIN64_DEFINES	)
set(POSIX64_DEFINES	)
set(WINDOWS_DEFINES	)
set(POSIX_DEFINES	)

set(PLATFORM_64BIT_DEFINES	)
set(PLATFORM_32BIT_DEFINES 	)

# Generic libs to link against
set(LINK_LIBS	)

# Links dirs to search for link libs in
set(LINK_DIRS	)
set(WINDOWS_LINK_DIRS	)
set(POSIX_LINK_DIRS		)
set(WIN32_LINK_DIRS		)
set(WIN64_LINK_DIRS		)
set(POSIX32_LINK_DIRS	)
set(POSIX64_LINK_DIRS	)

# Platform specific link libs
set(WIN32_LINK_LIBS		)
set(WIN64_LINK_LIBS		)
set(POSIX32_LINK_LIBS	)
set(POSIX64_LINK_LIBS	)

# Generic include dirs
set(INCLUDE_DIRS		)

# Platform specific includes
set(WIN32_INCLUDE_DIRS	)
set(WIN64_INCLUDE_DIRS	)
set(POSIX32_INCLUDE_DIRS	)
set(POSIX64_INCLUDE_DIRS	)

# Set the variable of target for stuff
set(TARGET ${PROJECT})

# Set the output file name
set(OUTPUT_FILE_NAME ${PROJECT})

# If this is a shared lib
set(SHARED_LIB 1)

# If this is a static lib
set(STATIC_LIB 0)

# Include this to handle all the defines
include(${CMAKESCRIPTS_DIR}/library-base.cmake)

add_library(${PROJECT} SHARED ${SRCS})