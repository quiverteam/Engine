#
#
# Project script for jpeglib
#
#
project(jpeglib C CXX)

# Platform specific sources
set(WINDOWS_SRCS	)
set(POSIX_SRCS	)

set(SRCS
        "jcapimin.c"
        "jcapistd.c"
        "jccoefct.c"
        "jccolor.c"
        "jcdctmgr.c"
        "jchuff.c"
        "jcinit.c"
        "jcmainct.c"
        "jcmarker.c"
        "jcmaster.c"
        "jcomapi.c"
        "jcparam.c"
        "jcphuff.c"
        "jcprepct.c"
        "jcsample.c"
        "jctrans.c"
        "jdapimin.c"
        "jdapistd.c"
        "jdatadst.c"
        "jdatasrc.c"
        "jdcoefct.c"
        "jdcolor.c"
        "jddctmgr.c"
        "jdhuff.c"
        "jdinput.c"
        "jdmainct.c"
        "jdmarker.c"
        "jdmaster.c"
        "jdmerge.c"
        "jdphuff.c"
        "jdpostct.c"
        "jdsample.c"
        "jdtrans.c"
        "jerror.c"
        "jfdctflt.c"
        "jfdctfst.c"
        "jfdctint.c"
        "jidctflt.c"
        "jidctfst.c"
        "jidctint.c"
        "jidctred.c"
        "jmemmgr.c"
        "jmemnobs.c"
        "jquant1.c"
        "jquant2.c"
        "jutils.c")

# Preprocessor defs
set(DEFINES			)

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
set(DEPENDENCIES		)

# Platform specific link libs
set(WINDOWS_LINK_LIBS	)
set(WIN32_LINK_LIBS		)
set(WIN64_LINK_LIBS		)
set(POSIX32_LINK_LIBS	)
set(POSIX64_LINK_LIBS	)

# Generic include dirs
set(INCLUDE_DIRS		${PUBLIC_INCLUDE}/jpeglib)

# Platform specific includes
set(WIN32_INCLUDE_DIRS	)
set(WIN64_INCLUDE_DIRS	)
set(POSIX32_INCLUDE_DIRS	)
set(POSIX64_INCLUDE_DIRS	)

# Set the variable of target for stuff
set(TARGET jpeglib)

# Set the output file name
set(OUTPUT_FILE_NAME jpeglib)

# If this is a shared lib
set(SHARED_LIB 0)

# If this is a static lib
set(STATIC_LIB 1)

# Include this to handle all the defines
include(${CMAKESCRIPTS_DIR}/library-base.cmake)
