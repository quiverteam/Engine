#
#
# Project file for tier0
#
#

add_definitions(-DTIER0_DLL_EXPORT)
set(TIER0_DEPS
	"ws2_32"
	)

if(MSVC)
	set(TIER0_SOURCES_WIN32
		"win32consoleio.cpp"
		"DESKey/ALGO.H"
		"DESKey/DK2WIN32.H"
		)

	set(TIER0_DEPS_WIN32
		"DESKey/ALGO32.LIB"
		"DESKey/DK2WIN32.LIB"
		)
else()
	set(TIER0_SOURCES_WIN32 "")

	set(TIER0_DEPS_WIN32 "")
endif()

set(TIER0_SOURCES_NOPCH
	"cpu.cpp"
	"minidump.cpp"
	"PMELib.cpp"
	)

set(TIER0_SOURCES_PCH
	"pch_tier0.cpp"
	"threadtools.cpp"
	)

set(TIER0_SOURCES
	"assert_dialog.cpp"
	"assert_dialog.rc"
	"commandline.cpp"
	"dbg.cpp"
	"fasttimer.cpp"
	"mem.cpp"
	"mem_helpers.cpp"
	"memdbg.cpp"
	"memstd.cpp"
	"memvalidate.cpp"
	"dbg.cpp"
	"pch_tier0.cpp"
	"platform.cpp"
	"pme.cpp"
	"progressbar.cpp"	
	"security.cpp"
	"systeminformation.cpp"
	"thread.cpp"
	"threadtools.cpp"
	"tslist.cpp"
	"vcrmode.cpp"
	"vprof.cpp"
	"${PUBLIC_INCLUDE}/tier0/basetypes.h"
	"${PUBLIC_INCLUDE}/tier0/commonmacros.h"
	"${PUBLIC_INCLUDE}/tier0/dbg.h"
	"${PUBLIC_INCLUDE}/tier0/dbgflag.h"
	"${PUBLIC_INCLUDE}/tier0/EventMasks.h"
	"${PUBLIC_INCLUDE}/tier0/EventModes.h"
	"${PUBLIC_INCLUDE}/tier0/fasttimer.h"
	"${PUBLIC_INCLUDE}/tier0/ia32detect.h"
	"${PUBLIC_INCLUDE}/tier0/icommandline.h"
	"${PUBLIC_INCLUDE}/tier0/IOCTLCodes.h"
	"${PUBLIC_INCLUDE}/tier0/K8PerformanceCounters.h"
	"${PUBLIC_INCLUDE}/tier0/l2cache.h"
	"${PUBLIC_INCLUDE}/tier0/mem.h"
	"${PUBLIC_INCLUDE}/tier0/memalloc.h"
	"${PUBLIC_INCLUDE}/tier0/memdbgoff.h"
	"${PUBLIC_INCLUDE}/tier0/memdbgon.h"
	"${PUBLIC_INCLUDE}/tier0/minidump.h"
	"${PUBLIC_INCLUDE}/tier0/P4PerformanceCounters.h"
	"${PUBLIC_INCLUDE}/tier0/P5P6PerformanceCounters.h"
	"pch_tier0.h"
	"${PUBLIC_INCLUDE}/tier0/platform.h"
	"${PUBLIC_INCLUDE}/tier0/PMELib.h"
	"${PUBLIC_INCLUDE}/tier0/progressbar.h"
	"${PUBLIC_INCLUDE}/tier0/protected_things.h"
	"resource.h"
	"${PUBLIC_INCLUDE}/tier0/systeminformation.h"
	"${PUBLIC_INCLUDE}/tier0/threadtools.h"
	"${PUBLIC_INCLUDE}/tier0/tslist.h"
	"${PUBLIC_INCLUDE}/tier0/validator.h"
	"${PUBLIC_INCLUDE}/tier0/valobject.h"
	"${PUBLIC_INCLUDE}/tier0/valve_off.h"
	"${PUBLIC_INCLUDE}/tier0/valve_on.h"
	"${PUBLIC_INCLUDE}/tier0/vcr_shared.h"
	"${PUBLIC_INCLUDE}/tier0/vcrmode.h"
	"${PUBLIC_INCLUDE}/tier0/vprof.h"
	"${PUBLIC_INCLUDE}/tier0/wchartypes.h"
	"mem_helpers.h"
	)
	
project(tier0 C CXX ASM "")

add_library(tier0 SHARED
	${TIER0_SOURCES}
	${TIER0_SOURCES_WIN32}
	${TIER0_SOURCES_NOPCH}
	${TIER0_SOURCES_PCH}
	)
	
set_target_properties(tier0 PROPERTIES ENABLE_EXPORTS 1)

set_source_files_properties(${TIER0_SOURCES_NOPCH}
	PROPERTIES
	COMPILE_FLAGS "/Y-"
	)

set_source_files_properties(${TIER0_SOURCES_PCH}
	PROPERTIES
	COMPILE_FLAGS "/Yc"
	)