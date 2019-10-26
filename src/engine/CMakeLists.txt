#
#
# Project script for the engine
#
#
project(engine C CXX)

# Platform specific sources 
set(WINDOWS_SRCS	)
set(POSIX_SRCS		)

set(SRCS			)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/ *.c *.cpp)
list(APPEND SRCS ${SOURCES})

# Preprocessor defs
set(DEFINES			-DUSE_CONVARS
					-DVOICE_OVER_IP
					-DBUMPMAP
					-D__USEA3D
					-D_ADD_EAX_
					-DENGINE_DLL
					-DPROTECTED_THINGS_ENABLE)

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
set(DEPENDENCIES 		vstdlib 
						tier0 
						tier1 
						tier2
						tier3
						bitmap 
						vgui_controls 
						vtf
						binkw32
						matsys_controls
						jpeglib
						bzip2
						mathlib
						appframework
						Dmxloader)

# Platform specific link libs
set(WINDOWS_LINK_LIBS	dinput8.lib 
						winmm.lib 
						wsock32.lib 
						ws2_32.lib 
						wininet.lib 
						vfw32.lib 
						Rpcrt4.lib
						dsound.lib
						dxguid.lib)

set(WIN32_LINK_LIBS		)
set(WIN64_LINK_LIBS		)
set(POSIX_LINK_LIBS		)
set(POSIX32_LINK_LIBS	)
set(POSIX64_LINK_LIBS	)

# Generic include dirs
set(INCLUDE_DIRS		${PUBLIC_INCLUDE}/tier1 
						${PUBLIC_INCLUDE}/tier0
						${PUBLIC_INCLUDE}/vstdlib
						${PUBLIC_INCLUDE}/tier2
						${ROOT_DIR}/vgui2/include
						${ROOT_DIR}/vgui2/controls/
						${CMAKE_CURRENT_SOURCE_DIR}/audio/
						${CMAKE_CURRENT_SOURCE_DIR}/audio/private)

# Platform specific includes
set(WIN32_INCLUDE_DIRS	)
set(WIN64_INCLUDE_DIRS	)
set(POSIX32_INCLUDE_DIRS	)
set(POSIX64_INCLUDE_DIRS	)

# Set the variable of target for stuff
set(TARGET engine)

# Set the output file name
set(OUTPUT_FILE_NAME engine)

# If this is a shared lib
set(SHARED_LIB 1)

# If this is a static lib
set(STATIC_LIB 0)

# Include this to handle all the defines
include(${CMAKESCRIPTS_DIR}/library-base.cmake)
