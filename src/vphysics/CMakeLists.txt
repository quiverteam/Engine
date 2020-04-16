#
#
# Project script for vphysics
#
#
project(vphysics C CXX)

# Platform specific sources
set(WINDOWS_SRCS	)
set(POSIX_SRCS	)

set(SRCS					"DebugDrawer.cpp"
        "miscmath.cpp"
        "Physics.cpp"
        "Physics_Collision.cpp"
        "Physics_CollisionSet.cpp"
        "Physics_Constraint.cpp"
        "Physics_DragController.cpp"
        "Physics_Environment.cpp"
        "Physics_FluidController.cpp"
        "Physics_FrictionSnapshot.cpp"
        "Physics_KeyParser.cpp"
        "Physics_MotionController.cpp"
        "Physics_Object.cpp"
        "Physics_ObjectPairHash.cpp"
        "Physics_PlayerController.cpp"
        "Physics_ShadowController.cpp"
        "Physics_SoftBody.cpp"
        "Physics_SurfaceProps.cpp"
        "Physics_VehicleAirboat.cpp"
        "Physics_VehicleController.cpp"
        "Physics_VehicleControllerCustom.cpp"
        "StdAfx.cpp")

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/ *.c *.cpp)
list(APPEND SRCS ${SOURCES})

# Preprocessor defs
set(DEFINES			)

# Platform-specific preprocessor defines
set(WIN32_DEFINES	)
set(POSIX32_DEFINES	)
set(WIN64_DEFINES	)
set(POSIX64_DEFINES	)
set(WINDOWS_DEFINES	)
set(POSIX_DEFINES	-DNO_MALLOC_OVERRIDE=1)

set(PLATFORM_64BIT_DEFINES	)
set(PLATFORM_32BIT_DEFINES 	)

# Links dirs to search for link libs in
set(LINK_DIRS	        )
set(WINDOWS_LINK_DIRS	)
set(POSIX_LINK_DIRS		)
set(WIN32_LINK_DIRS		)
set(WIN64_LINK_DIRS		)
set(POSIX32_LINK_DIRS	)
set(POSIX64_LINK_DIRS	)

# Projects this depends on
set(DEPENDENCIES		tier2 tier1 tier0 mathlib vstdlib)

# Generic libs to link against
set(LINK_LIBS	        BulletCollision BulletDynamics BulletSoftBody BulletMultithreaded LinearMath)

# Platform specific link libs
set(WINDOWS_LINK_LIBS	)
set(WIN32_LINK_LIBS		)
set(WIN64_LINK_LIBS		)
set(POSIX32_LINK_LIBS	)
set(POSIX64_LINK_LIBS	)

# Generic include dirs
set(INCLUDE_DIRS		${PUBLIC_INCLUDE}/tier1 ${PUBLIC_INCLUDE}/tier0)

# Platform specific includes
set(WIN32_INCLUDE_DIRS	)
set(WIN64_INCLUDE_DIRS	)
set(POSIX32_INCLUDE_DIRS	)
set(POSIX64_INCLUDE_DIRS	)

# Set the variable of target for stuff
set(TARGET vphysics)

# Set the output file name
set(OUTPUT_FILE_NAME vphysics)

# If this is a shared lib
set(SHARED_LIB 1)

# If this is a static lib
set(STATIC_LIB 0)

# Include this to handle all the defines
include(${CMAKESCRIPTS_DIR}/library-base.cmake)
