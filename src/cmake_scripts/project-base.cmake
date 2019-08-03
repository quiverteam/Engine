#================================================#
#
# Base for all projects
# Separated from library-base, etc. because it's easier
# NOTE: Please add ALL of the base stuff into this
# NOTE AGAIN: I might merge this with source-base.cmake
#================================================#
#
# This is the place where all link libs are located, yes
#
#================================================#
# Vars that should be defined in including files
# TARGET: the name of the target
# 
#================================================#
# CMake vars defined by source base:
#	-	ROOT_DIR
#	-	LIBCOMMON
#	-	LIBPUBLIC
#	-	PUBLIC_INCLUDE
#	-	COMMON_INCLUDE
#	-	THIS_IS_A_LIBRARY (for libs)
#	-	THIS_IS_A_EXE (for exes)
#	-	STATIC_LIB (1 for static libs)
#	-	SHARED_LIB (1 for shared libs)
#================================================#

# Include this just in case
include(${CMAKESCRIPTS_DIR}/source-directories.cmake)
include(${CMAKESCRIPTS_DIR}/source-base.cmake)

# If this is a library...
if(THIS_IS_A_LIBRARY EQUAL 1)
	if(STATIC_LIB EQUAL 1)
		set(THIS_IS_A_STATIC_LIB 1)
		list(APPEND DEFINES -D_LIB -DLIB)
	else()
		set(THIS_IS_A_SHARED_LIB 1)
		list(APPEND DEFINES -D_USRDLL -D_SHAREDLIB -D_DLL_ -D_DLL)
	endif(STATIC_LIB EQUAL 1)
endif(THIS_IS_A_LIBRARY EQUAL 1)

#================================================#
# First we should handle the preprocessor defs
#================================================#
#
# For All debug:
#	-	_DEBUG, _CRT_SECURE_NO_DEPRECATE, _CRT_NONSTDC_NO_DEPRECATE
#	-	_HAS_ITERATOR_DEBUGGING, DEBUG, _ALLOW_RUNTIME_LIBRARY_MISMATCH
# For win debug:
# 	-	_ALLOW_MSCC_VER_MISMATCH
#
#
list(APPEND WINDOWS_DEFINES		-D_WIN32 -DWIN32 -DWINDOWS -D_ALLOW_MSC_VER_MISMATCH)

list(APPEND WIN32_DEFINES		-DPLATFORM_WINDOWS_PC32)

list(APPEND WIN64_DEFINES		-DPLATFORM_WINDOWS_PC64)

list(APPEND DEFINES				-D_CRT_SECURE_NO_DEPRECATE
								-D_CRT_NONSTDC_NO_DEPRECATE
								-D_ALLOW_RUNTIME_LIBRARY_MISMATCH
								-D_ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH
								-D_HAS_ITERATOR_DEBUGGING=0)
list(APPEND POSIX_DEFINES		-DUSE_SDL -DDX_TO_GL_ABSTRACTION)

# Some IDEs like to have header files added to their projects
# Luckily headers dont actually compile in a CMake project, so it's safe to recurse the whole
# repo in search of header files.
FILE(GLOB_RECURSE headers ${ROOT_DIR} *.h *.hxx *.hpp *.hh)

list(APPEND SRCS ${headers})

# For debug/release builds
if(RELEASE)
	list(APPEND DEFINES -DNDEBUG -D_NDEBUG -DRELEASEASSERTS)
else()
	list(APPEND DEFINES -DDEBUG -D_DEBUG)
endif(RELEASE)

if(DEFINED WINDOWS)
	list(APPEND DEFINES ${WINDOWS_DEFINES})
	# WIN32 BASE DEFINES (Common for everything windows)
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

if(THIS_IS_A_LIBRARY EQUAL 1)
	# Defines for shared libraries
	if(SHARED_LIB EQUAL 1)
	
	endif(SHARED_LIB EQUAL 1)
	
	# Defines for static libraries
	if(STATIC_LIB EQUAL 1)
	
	endif(STATIC_LIB EQUAL 1)
endif(THIS_IS_A_LIBRARY EQUAL 1)

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
# (at least the user defined ones)
#================================================#

# needed for dlls and exes on windows
if(THIS_IS_A_EXE EQUAL 1 OR THIS_IS_A_SHARED_LIB EQUAL 1)
	list(APPEND WINDOWS_LINK_LIBS	shell32.lib
									user32.lib
									advapi32.lib
									gdi32.lib
									comdlg32.lib
									ole32.lib)
endif(THIS_IS_A_EXE EQUAL 1 OR THIS_IS_A_SHARED_LIB EQUAL 1)
list(APPEND POSIX_LINK_LIBS 		tcmalloc_minimal)

if(DEFINED WINDOWS)
	# Generic windows libs
	list(APPEND LINK_LIBS ${WINDOWS_LINK_LIBS})
	
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
	# Generic posix libs
	list(APPEND LINK_LIBS ${POSIX_LINK_LIBS})
	
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
# Handle the include dirs
#================================================#

# For windows only
list(APPEND INCLUDE_DIRS	${DX9SDK}/Include/)

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

include_directories(${ROOT_DIR})


#================================================#
# Handle the link directories
#================================================#
list(APPEND POSIX_LINK_DIRS		/usr/lib/
								/usr/lib32/)
list(APPEND POSIX32_LINK_DIRS	/usr/lib/i386-linux-gnu/)
list(APPEND POSIX64_LINK_DIRS	/usr/lib/x86_64-linux-gnu/)

# For windows only
list(APPEND WINDOWS_LINK_DIRS	${DX9SDK}/Lib/)

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
# Handle special cases
#================================================#
set(ACTUAL_LIBS	)
# Find vulkan
if(USE_VULKAN EQUAL 1)
	find_package(Vulkan REQUIRED)
	if(NOT Vulkan_FOUND)
		message(FATAL_ERROR "Unable to find vulkan!")
	else()
		list(APPEND ACTUAL_LIBS ${Vulkan_LIBRARIES})
		list(APPEND INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})
	endif(NOT Vulkan_FOUND)
endif(USE_VULKAN EQUAL 1)

# Find OpenGL
if(USE_OPENGL EQUAL 1)
	find_package(OpenGL REQUIRED)
	if(NOT OpenGL_FOUND)
		message(FATAL_ERROR "Unable to find OpenGL!")
	else()
		list(APPEND ACTUAL_LIBS ${OpenGL_LIBRARIES})
		list(APPEND INCLUDE_DIRS ${OpenGL_INCLUDE_DIRS})
	endif(NOT OpenGL_FOUND)
endif(USE_OPENGL EQUAL 1)

# For needed packages
foreach(package IN LISTS PACKAGES)
	find_package(${package} REQUIRED)
	if(NOT ${package}_FOUND)
		message(FATAL_ERROR "Unable to find package ${package}!")
	else()
		list(APPEND ACTUAL_LIBS ${${package}_LIBRARIES})
		list(APPEND INCLUDE_DIRS ${${package}_INCLUDE_DIRS})
	endif(NOT ${package}_FOUND)
endforeach(package IN LISTS PACKAGES)

#================================================#
# Add the target
#================================================#
if(NOT SHARED_LIB AND NOT STATIC_LIB AND THIS_IS_A_LIBRARY)
	message("The project ${TARGET} must be declared as a static or shared lib.")
endif(NOT SHARED_LIB AND NOT STATIC_LIB AND THIS_IS_A_LIBRARY)

if(DEFINED THIS_IS_A_LIBRARY)
	if(SHARED_LIB)
		add_library(${TARGET} SHARED ${SRCS})
	else()
		add_library(${TARGET} STATIC ${SRCS})
	endif(SHARED_LIB)
	set_target_properties(${TARGET} PROPERTIES ENABLE_EXPORTS 1)
endif(DEFINED THIS_IS_A_LIBRARY)

if(DEFINED THIS_IS_A_EXE)
	add_executable(${TARGET} ${SRCS})
endif(DEFINED THIS_IS_A_EXE)

#================================================#
# Handle all the link libraries
#================================================#
#
# For Win32, we need to link against these by default:
#	-	shell32.lib
#	-	user32.lib
#	-	advapi32.lib
#	-	gdi32.lib
#	-	comdlg32.lib
#	-	ole32.lib
#
# For all projects, we need to link against these:
# 	-	vstdlib
# 	-	tier0
# 	-	tier1
#
#

# We can loop through all the libs and use find library to
# find everything the user specified
foreach(link_lib IN LISTS LINK_LIBS)
	set(LIB_${link_lib} "lib_path-NOTFOUND")
	find_library(LIB_${link_lib} NAMES ${link_lib} PATHS ${LINK_DIRS})
	
	# If libs are not found...
	if(LIB_${link_lib} EQUAL "LIB_${link_lib}-NOTFOUND")
		message(WARN "Unable to find library ${link_lib}!")
	else()
		list(APPEND ACTUAL_LIBS ${LIB_${link_lib}})
	endif(LIB_${link_lib} EQUAL "LIB_${link_lib}-NOTFOUND")
	
endforeach(link_lib IN LISTS LINK_LIBS)

target_include_directories(${TARGET} PUBLIC ${INCLUDE_DIRS})
target_link_libraries(${TARGET} ${ACTUAL_LIBS})
target_link_libraries(${TARGET} ${DEPENDENCIES})
set_target_properties(${TARGET} PROPERTIES LINKER_LANGUAGE CXX)
