#
#
# Project script for ${PROJECT}
#
#
project(${PROJECT} C CXX)

# Platform specific sources 
set(WIN32_SRCS	)
set(POSIX_SRCS	)

# Generic sources
set(SRCS	)

# Directories where all source files will be taken from
set(EXTRA_SRC_DIRS	)

# Preprocessor defs
set(DEFINES	)

# Platform-specific preprocessor defines
set(WIN32_DEFINES	)
set(POSIX_DEFINES	)
set(PLATFORM_64BIT_DEFINES	)
set(PLATFORM_32BIT_DEFINES 	)

# Generic libs to link against
set(LINK_LIBS	)

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

# Include this to handle all the defines
include(${CMAKESCRIPTS_DIR}/library-base.cmake)

add_library(${PROJECT} SHARED ${SRCS})