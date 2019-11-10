#
#
# Project script for Tier4
#
#
project(tier4 C CXX)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/ *.c *.cpp)
list(APPEND SRCS ${SOURCES})

# Preprocessor defs
set(DEFINES	-DTIER4_STATIC_LIB)

# Generic libs to link against
set(POSIX_LINK_LIBS			SDL2)

# Windows link libs
set(WIN32_LINK_LIBS		)

# Generic include dirs
set(INCLUDE_DIRS		${PUBLIC_INCLUDE}/tier4)

# Platform specific includes
set(WIN32_INCLUDE_DIRS	)
set(WIN64_INCLUDE_DIRS	)

# Set the variable of target for stuff
set(TARGET tier4)

# Set the output file name
set(OUTPUT_FILE_NAME tier4)

# If this is a shared lib
set(SHARED_LIB 0)

# If this is a static lib
set(STATIC_LIB 1)

# Include this to handle all the defines
include(${CMAKESCRIPTS_DIR}/library-base.cmake)
