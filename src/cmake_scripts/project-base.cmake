#================================================#
#
# Base for all projects
# Separated from library-base, etc. because it's easier
# NOTE: Please add ALL of the base stuff into this
# NOTE AGAIN: I might merge this with source-base.cmake
#================================================#

#================================================#
# Vars that should be defined in including files
# TARGET: the name of the target
# 
#================================================#

# Include this just in case
include(${CMAKESCRIPTS_DIR}/source-directories.cmake)
include(${CMAKESCRIPTS_DIR}/source-base.cmake)

#================================================#
# First we should handle the preprocessor defs
#================================================#
if(DEFINED WINDOWS)
	list(APPEND DEFINES ${WINDOWS_DEFINES})

	# WIN32 defines
	if(DEFINED PLATFORM_32BITS)
		list(APPEND DEFINES ${WIN32_DEFINES})
	endif(DEFINED PLATFORM_32BITS)

	# WIN64 defines
	if(DEFINED PLATFORM_64BITS)
		list(APPEND DEFINES ${WIN64_DEFINES})
	endif(DEFINED PLATFORM_64BITS)

endif(DEFINED WINDOWS)

if(DEFINED POSIX)
	list(APPEND DEFINES ${POSIX_DEFINES})

	# POSIX32 defines
	if(DEFINED PLATFORM_32BITS)
		list(APPEND DEFINES ${POSIX32_DEFINES})
	endif(DEFINED PLATFORM_32BITS)

	# POSIX64 defines
	if(DEFINED PLATFORM_64BITS)
		list(APPEND DEFINES ${POSIX64_DEFINES})
	endif(DEFINED PLATFORM_64BITS)

endif(DEFINED POSIX)

#================================================#
# Now handle the sources
#================================================#
if(DEFINED WINDOWS)
	list(APPEND SRCS ${WINDOWS_SRCS})
endif(DEFINED WINDOWS)

if(DEFINED POSIX)
	list(APPEND SRCS ${POSIX_SRCS})
endif(DEFINED POSIX)

#================================================#
# Now handle the link libraries
#================================================#
if(DEFINED WINDOWS)
	# WIN32 libs
	if(DEFINED PLATFORM_32BITS)
		list(APPEND LINK_LIBS ${WIN32_LINK_LIBS})
	endif(DEFINED PLATFORM_32BITS)

	# WIN64 libs
	if(DEFINED PLATFORM_64BITS)
		list(APPEND LINK_LIBS ${WIN64_LINK_LIBS})
	endif(DEFINED PLATFORM_64BITS)
endif(DEFINED WINDOWS)

if(DEFINED POSIX)
	# POSIX32 libs
	if(DEFINED PLATFORM_32BITS)
		list(APPEND LINK_LIBS ${POSIX32_LINK_LIBS})
	endif(DEFINED PLATFORM_32BITS)

	# POSIX64 libs
	if(DEFINED PLATFORM_64BITS)
		list(APPEND LINK_LIBS ${POSIX64_LINK_LIBS})
	endif(DEFINED PLATFORM_64BITS)
endif(DEFINED POSIX)

#================================================#
# Handle he include dirs
#================================================#
if(DEFINED WINDOWS)
	list(APPEND INCLUDE_DIRS ${WINDOWS_INCLUDE_DIRS})
	# WIN32 includes
	if(DEFINED PLATFORM_32BITS)
		list(APPEND INCLUDE_DIRS ${WIN32_INCLUDE_DIRS})
	endif(DEFINED PLATFORM_32BITS)

	# WIN64 includes
	if(DEFINED PLATFORM_64BITS)
		list(APPEND INCLUDE_DIRS ${WIN64_INCLUDE_DIRS})
	endif(DEFINED PLATFORM_64BITS)
endif(DEFINED WINDOWS)

if(DEFINED POSIX)
	list(APPEND INCLUDE_DIRS ${POSIX_INCLUDE_DIRS})
	# POSIX32 includes
	if(DEFINED PLATFORM_32BITS)
		list(APPEND INCLUDE_DIRS ${POSIX32_INCLUDE_DIRS})
	endif(DEFINED PLATFORM_32BITS)

	# POSIX64 includes
	if(DEFINED PLATFORM_64BITS)
		list(APPEND INCLUDE_DIRS ${POSIX64_INCLUDE_DIRS})
	endif(DEFINED PLATFORM_64BITS)
endif(DEFINED POSIX)


#================================================#
# Handle he link directories
#================================================#
if(DEFINED WINDOWS)
	list(APPEND LINK_DIRS	${WINDOWS_LINK_DIRS})
	# WIN32 includlinkes
	if(DEFINED PLATFORM_32BITS)
		list(APPEND LINK_DIRS		${WIN32_LINK_DIRS})
	endif(DEFINED PLATFORM_32BITS)

	# WIN64 link
	if(DEFINED PLATFORM_64BITS)
		list(APPEND LINK_DIRS 		${WIN64_LINK_DIRS})
	endif(DEFINED PLATFORM_64BITS)
endif(DEFINED WINDOWS)

if(DEFINED POSIX)
	list(APPEND LINK_DIRS			${POSIX_LINK_DIRS})
	# POSIX32 link
	if(DEFINED PLATFORM_32BITS)
		list(APPEND LINK_DIRS		${POSIX32_LINK_DIRS})
	endif(DEFINED PLATFORM_32BITS)

	# POSIX64 link
	if(DEFINED PLATFORM_64BITS)
		list(APPEND LINK_DIRS		${POSIX64_LINK_DIRS})
	endif(DEFINED PLATFORM_64BITS)
endif(DEFINED POSIX)

#================================================#
# Add the target
#================================================#
if(NOT SHARED_LIB AND NOT STATIC_LIB)
	message(ERROR "The project ${TARGET} must be declared as a static or shared lib.")
endif(NOT SHARED_LIB AND NOT STATIC_LIB)

if(DEFINED THIS_IS_A_LIBRARY)
	if(SHARED_LIB)
		add_library(${TARGET} SHARED ${SRCS})
	else()
		add_library(${TARGET} STATIC ${SRCS})
	endif(SHARED_LIB)
endif(DEFINED THIS_IS_A_LIBRARY)

if(DEFINED THIS_IS_A_EXE)
	add_executable(${TARGET} ${SRCS})
endif(DEFINED THIS_IS_A_EXE)


# leave empty
set(ACTUAL_LIBS )

# We can loop through all the libs and use find library to
# find everything the user specified
foreach(link_lib IN ${LINK_LIBS})
	find_library(lib_path NAMES ${link_lib} PATHS ${LINK_DIRS})
	list(APPEND ACTUAL_LIBS ${lib_path})
endforeach(link_lib IN ${LINK_LIBS})

target_include_directories(${TARGET} PUBLIC ${INCLUDE_DIRS})
target_link_libraries(${TARGET} ${ACTUAL_LIBS})
set_target_properties(${TARGET} PROPERTIES LINKER_LANGUAGE CXX)