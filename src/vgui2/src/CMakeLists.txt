#
#
# Project script for vgui_dll
#
#
project(vgui_dll C CXX)

# Platform specific sources
set(WINDOWS_SRCS	)
set(POSIX_SRCS	)

# We need to append damn lzma shit
set(SRCS	)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/ *.c *.cpp)
list(APPEND SRCS ${SOURCES})

# Preprocessor defs
set(DEFINES	        -DDONT_PROTECT_FILEIO_FUNCTIONS)

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
set(LINK_LIBS	)

# Platform specific link libs
set(WINDOWS_LINK_LIBS   Imm32.lib Shlwapi.lib odbc32.lib odbccp32.lib winmm.lib)
set(WIN32_LINK_LIBS		)
set(WIN64_LINK_LIBS		)
set(POSIX32_LINK_LIBS	)
set(POSIX64_LINK_LIBS	)

# Generic include dirs
set(INCLUDE_DIRS		${PUBLIC_INCLUDE}/tier3 ${PUBLIC_INCLUDE}/tier2 ${PUBLIC_INCLUDE}/vgui ${PUBLIC_INCLUDE}/tier1 ${PUBLIC_INCLUDE}/mathlib ${COMMON_INCLUDE}/vgui_surfacelib
                        ${PUBLIC_INCLUDE}/tier0 ${PUBLIC_INCLUDE}/vstdlib ${PUBLIC_INCLUDE}/appframework)

# Platform specific includes
set(WIN32_INCLUDE_DIRS	    )
set(WIN64_INCLUDE_DIRS	    )
set(POSIX32_INCLUDE_DIRS	)
set(POSIX64_INCLUDE_DIRS	)

# Projects this depends on
set(DEPENDENCIES            tier2 tier3 vgui_surfacelib)

# Set the variable of target for stuff
set(TARGET vgui_dll)

# Set the output file name
set(OUTPUT_FILE_NAME vgui2)

# If this is a shared lib
set(SHARED_LIB 1)

# If this is a static lib
set(STATIC_LIB 0)

# Include this to handle all the defines
include(${CMAKESCRIPTS_DIR}/library-base.cmake)
