#
#
# Project script for launcher (DLL) 
#
#
project(launcher_main C CXX)

# Platform specific sources 
set(WINDOWS_SRCS	)
set(POSIX_SRCS	)

set(SRCS	"${PUBLIC}/bsptreedata.cpp"
			"${PUBLIC}/disp_common.cpp"
			"${PUBLIC}/disp_powerinfo.cpp"
			"disp_vrad.cpp"
			"imagepacker.cpp"
			"incremental.cpp"
			"leaf_ambient_lighting.cpp"
			"lightmap.cpp"
			"${PUBLIC}/loadcmdline.cpp"
			"${PUBLIC}/lumpfiles.cpp"
			"macro_texture.cpp"
			"${UTILS_COMMON}/mpi_stats.cpp"
			"mpivrad.cpp"
			#"${UTILS_COMMON}/MySqlDatabase.cpp"
			"${UTILS_COMMON}/pacifier.cpp"
			"${UTILS_COMMON}/physdll.cpp"
			"radial.cpp"
			"samplehash.cpp"
			"trace.cpp"
			"${UTILS_COMMON}/utilmatlib.cpp"
			"vismat.cpp"
			#"${UTILS_COMMON}/vmpi_tools_shared.cpp"
			"${UTILS_COMMON}/vmpi_tools_shared.h"
			"vrad.cpp"
			"vrad_dispcoll.cpp"
			"vraddetailprops.cpp"
			"vraddisps.cpp"
			"vraddll.cpp"
			"vradstaticprops.cpp"
			"${PUBLIC}/zip_utils.cpp"
			"${UTILS_COMMON}/bsplib.cpp"
			"${PUBLIC}/builddisp.cpp"
			"${PUBLIC}/chunkfile.cpp"
			"${UTILS_COMMON}/cmdlib.cpp"
			"${PUBLIC}/dispcoll_common.cpp"
			"${UTILS_COMMON}/map_shared.cpp"
			"${UTILS_COMMON}/polylib.cpp"
			"${UTILS_COMMON}/scriplib.cpp"
			"${UTILS_COMMON}/threads.cpp"
			"${UTILS_COMMON}/tools_minidump.cpp"
			"${UTILS_COMMON}/tools_minidump.h"
			"${PUBLIC}/collisionutils.cpp"
			"${PUBLIC}/filesystem_helpers.cpp"
			"${PUBLIC}/scratchpad3d.cpp"
			"${PUBLIC}/ScratchPadUtils.cpp")

#file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/ *.c *.cpp)
#list(APPEND SRCS ${SOURCES})

# Preprocessor defs
set(DEFINES			-DPROTECTED_THINGS_DISABLE=1 -DVRAD=1) #NO VMPI

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
set(DEPENDENCIES vstdlib tier1 tier0 tier2 tier3 tier4 appframework)

# Platform specific link libs
set(WINDOWS_LINK_LIBS	)
set(POSIX_LINK_LIBS		)
set(WIN32_LINK_LIBS		)
set(WIN64_LINK_LIBS		)
set(POSIX32_LINK_LIBS	)
set(POSIX64_LINK_LIBS	)

# Generic include dirs
set(INCLUDE_DIRS		${PUBLIC_INCLUDE}/tier0 
						${PUBLIC_INCLUDE}/tier1
						${PUBLIC_INCLUDE}/tier2
						${PUBLIC_INCLUDE}/tier3
						${PUBLIC_INCLUDE}/tier4
						${PUBLIC_INCLUDE}/appframework
						${UTILS}/common
						${UTILS}/vmpi
						${UTILS}/vmpi/mysql/mysqlpp/include
						${UTILS}/vmpi/mysqlinclude)

# Platform specific includes
set(WIN32_INCLUDE_DIRS	)
set(WIN64_INCLUDE_DIRS	)
set(POSIX32_INCLUDE_DIRS	)
set(POSIX64_INCLUDE_DIRS	)

# Set the variable of target for stuff
set(TARGET vrad)

# Set the output file name
set(OUTPUT_FILE_NAME vrad)

# If this is a shared lib
set(SHARED_LIB 1)

# If this is a static lib
#set(STATIC_LIB 0)

# Include this to handle all the defines
include(${CMAKESCRIPTS_DIR}/library-base.cmake)
