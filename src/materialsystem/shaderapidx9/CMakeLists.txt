#
#
# Project script for shaderapidx9. NO dx10 stuff here!
#
#
project(shaderapidx9 C CXX)

# Platform specific sources
set(WINDOWS_SRCS	    "wmi.cpp" "recording.cpp" "d3d_async.cpp")
set(POSIX_SRCS	        )

set(SRCS	            		"colorformatdx8.cpp"
        "${PUBLIC}/filesystem_helpers.cpp"
        "hardwareconfig.cpp"
        "meshbase.cpp"
        "meshdx8.cpp"
        "shaderapidx8.cpp"
        "shaderdevicebase.cpp"
        "shaderapibase.cpp"
        "shaderdevicedx8.cpp"
        "shadershadowdx8.cpp"
        "texturedx8.cpp"
        "TransitionTable.cpp"
        "cvballoctracker.cpp"
        "vertexdecl.cpp"
        "vertexshaderdx8.cpp")


# Preprocessor defs
set(DEFINES	                -DSHADERAPIDX9 -DSHADER_DLL_EXPORT -DPROTECTED_THINGS_DISABLE)

# Platform-specific preprocessor defines
set(WIN32_DEFINES	        )
set(POSIX32_DEFINES	        )
set(WIN64_DEFINES	        )
set(POSIX64_DEFINES	        )
set(WINDOWS_DEFINES	        )
set(POSIX_DEFINES	        )

set(PLATFORM_64BIT_DEFINES	)
set(PLATFORM_32BIT_DEFINES 	)

# Links dirs to search for link libs in
set(LINK_DIRS	            )
set(WINDOWS_LINK_DIRS	    )
set(POSIX_LINK_DIRS		    )
set(WIN32_LINK_DIRS		    )
set(WIN64_LINK_DIRS		    )
set(POSIX32_LINK_DIRS	    )
set(POSIX64_LINK_DIRS	    )

# Generic libs to link against
set(LINK_LIBS	            )

# Platform specific link libs
set(WINDOWS_LINK_LIBS       d3dx9 d3d9)
set(WIN32_LINK_LIBS		    )
set(WIN64_LINK_LIBS		    )
set(POSIX32_LINK_LIBS	    )
set(POSIX64_LINK_LIBS	    )

# Generic include dirs
set(INCLUDE_DIRS		${PUBLIC_INCLUDE}/tier3 ${PUBLIC_INCLUDE}/tier2 ${PUBLIC_INCLUDE}/vgui ${PUBLIC_INCLUDE}/tier1 ${PUBLIC_INCLUDE}/mathlib ${COMMON_INCLUDE}/vgui_surfacelib
        ${PUBLIC_INCLUDE}/tier0 ${PUBLIC_INCLUDE}/vstdlib ${PUBLIC_INCLUDE}/appframework ${PUBLIC_INCLUDE}/materialsystem ${ROOT_DIR}/materialsystem)

# Platform specific includes
set(WIN32_INCLUDE_DIRS	    )
set(WIN64_INCLUDE_DIRS	    )
set(POSIX32_INCLUDE_DIRS	)
set(POSIX64_INCLUDE_DIRS	)

# Projects this depends on
set(DEPENDENCIES            bitmap mathlib tier2 tier1 tier0 vstdlib bzip2)

# Set the variable of target for stuff
set(TARGET shaderapidx9)

# Set the output file name
set(OUTPUT_FILE_NAME shaderapidx9)

# If this is a shared lib
set(SHARED_LIB 1)

# If this is a static lib
set(STATIC_LIB 0)

# Include this to handle all the defines
include(${CMAKESCRIPTS_DIR}/library-base.cmake)
