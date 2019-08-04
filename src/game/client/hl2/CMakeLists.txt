#
#
# Project script for client_hl2
#
#
project(client_hl2 C CXX)

include(${ROOT_DIR}/game/client/ClientFiles.cmake)

set(SRCS	)

list(APPEND SRCS ${CLIENT_BASE_FILES})
list(APPEND SRCS ${CLIENT_HL2_FILES})

# Preprocessor defs
set(DEFINES			-DNO_STRING_T -DCLIENT_DLL -DVECTOR -DPROTECTED_THINGS_ENABLE -DHL2_CLIENT_DLL)

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
set(POSIX_LINK_DIRS		)
set(WIN32_LINK_DIRS		)
set(WIN64_LINK_DIRS		)
set(POSIX32_LINK_DIRS	)
set(POSIX64_LINK_DIRS	)

# Generic libs to link against
set(LINK_LIBS			)

# Other projects this one depends on
set(DEPENDENCIES vstdlib tier1 tier2 tier3 vgui_controls bitmap choreoobjects dmxloader mathlib particles)

# Platform specific link libs
set(WINDOWS_LINK_LIBS	winmm.lib Shell32.lib User32.lib)
set(WIN32_LINK_LIBS		)
set(WIN64_LINK_LIBS		)
set(POSIX32_LINK_LIBS	)
set(POSIX64_LINK_LIBS	)

# Generic include dirs
set(INCLUDE_DIRS		${PUBLIC_INCLUDE}/tier1 ${PUBLIC_INCLUDE}/tier0 ${PUBLIC_INCLUDE}/vstdlib
						${ROOT_DIR}/vgui2/include ${ROOT_DIR}/vgui2/controls ${ROOT_DIR}/game/shared
						${CMAKE_CURRENT_SOURCE_DIR}/game_controls)

# Platform specific includes
set(WIN32_INCLUDE_DIRS	)
set(WIN64_INCLUDE_DIRS	)
set(POSIX32_INCLUDE_DIRS	)
set(POSIX64_INCLUDE_DIRS	)

# Set the variable of target for stuff
set(TARGET client_hl2)

# Set the output file name
set(OUTPUT_FILE_NAME client)

# If this is a shared lib
set(SHARED_LIB 1)

# If this is a static lib
set(STATIC_LIB 0)

# Include this to handle all the defines
include(${CMAKESCRIPTS_DIR}/library-base.cmake)
 
