//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "pch_tier0.h"
#include <time.h>

#ifdef _WIN32
#if defined(_WIN32)
#define WINDOWS_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0502
#include <windows.h>
#include <shlwapi.h>
#include <direct.h>
#include <versionhelpers.h>
#endif
#include <assert.h>
#include "tier0/platform.h"
#ifndef _X360
#include "tier0/vcrmode.h"
#endif // _X360
#if !defined(STEAM) && !defined(NO_MALLOC_OVERRIDE)
#include "tier0/memalloc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
#endif 

#ifndef _X360
extern VCRMode_t g_VCRMode;
#endif
static LARGE_INTEGER g_PerformanceFrequency;
static LARGE_INTEGER g_MSPerformanceFrequency;
static LARGE_INTEGER g_ClockStart;

/* Mingw does not have this */
#if defined(__GNUC__) && defined(_WIN32)
#define IsWindows10OrGreater() false
#endif

static void InitTime()
{
	if( !g_PerformanceFrequency.QuadPart )
	{
		QueryPerformanceFrequency(&g_PerformanceFrequency);
		g_MSPerformanceFrequency.QuadPart = g_PerformanceFrequency.QuadPart / 1000;
		QueryPerformanceCounter(&g_ClockStart);
	}
}

double Plat_FloatTime()
{
	InitTime();

	LARGE_INTEGER CurrentTime;

	QueryPerformanceCounter( &CurrentTime );

	double fRawSeconds = (double)( CurrentTime.QuadPart - g_ClockStart.QuadPart ) / (double)(g_PerformanceFrequency.QuadPart);

	return fRawSeconds;
}

unsigned int Plat_MSTime()
{
	InitTime();

	LARGE_INTEGER CurrentTime;

	QueryPerformanceCounter( &CurrentTime );

	return (unsigned long) ( ( CurrentTime.QuadPart - g_ClockStart.QuadPart ) / g_MSPerformanceFrequency.QuadPart);
}

unsigned long long Plat_ValveTime()
{
	return 0xFFFFFFFFFFFFFFFF;
}

struct tm * Plat_localtime( const time_t *timep, struct tm *result )
{
	result = localtime( timep );
	return result;
}

void GetCurrentDate( int *pDay, int *pMonth, int *pYear )
{
	struct tm *pNewTime;
	time_t long_time;

	time( &long_time );                /* Get time as long integer. */
	pNewTime = localtime( &long_time ); /* Convert to local time. */

	*pDay = pNewTime->tm_mday;
	*pMonth = pNewTime->tm_mon + 1;
	*pYear = pNewTime->tm_year + 1900;
}

bool vtune( bool resume )
{
#ifndef _X360
	static bool bInitialized = false;
	static void (__cdecl *VTResume)(void) = NULL;
	static void (__cdecl *VTPause) (void) = NULL;

	// Grab the Pause and Resume function pointers from the VTune DLL the first time through:
	if( !bInitialized )
	{
		bInitialized = true;

		HINSTANCE pVTuneDLL = LoadLibrary( "vtuneapi.dll" );

		if( pVTuneDLL )
		{
			VTResume = (void(__cdecl *)())GetProcAddress( pVTuneDLL, "VTResume" );
			VTPause  = (void(__cdecl *)())GetProcAddress( pVTuneDLL, "VTPause" );
		}
	}

	// Call the appropriate function, as indicated by the argument:
	if( resume && VTResume )
	{
		VTResume();
		return true;

	} 
	else if( !resume && VTPause )
	{
		VTPause();
		return true;
	}
#endif
	return false;
}

bool Plat_IsInDebugSession()
{
#if defined( _WIN32 ) && !defined( _X360 )
	return (IsDebuggerPresent() != 0);
#elif defined( _WIN32 ) && defined( _X360 )
	return (XBX_IsDebuggerPresent() != 0);
#else
	return false;
#endif
}

void Plat_DebugString( const char * psz )
{
#if defined( _WIN32 ) && !defined( _X360 )
	::OutputDebugStringA( psz );
#elif defined( _WIN32 ) && defined( _X360 )
	XBX_OutputDebugString( psz );
#endif
}


const tchar *Plat_GetCommandLine()
{
#ifdef TCHAR_IS_WCHAR
	return GetCommandLineW();
#else
	return GetCommandLine();
#endif
}

const char *Plat_GetCommandLineA()
{
	return GetCommandLineA();
}

// -------------------------------------------------------------------------------------------------- //
// Memory stuff. 
//
// DEPRECATED. Still here to support binary back compatability of tier0.dll
//
// -------------------------------------------------------------------------------------------------- //
#ifndef _X360
#if !defined(STEAM) && !defined(NO_MALLOC_OVERRIDE)

typedef void (*Plat_AllocErrorFn)( unsigned long size );

void Plat_DefaultAllocErrorFn( unsigned long size )
{
}

Plat_AllocErrorFn g_AllocError = Plat_DefaultAllocErrorFn;
#endif

#ifndef _X360
CRITICAL_SECTION g_AllocCS;
class CAllocCSInit
{
public:
	CAllocCSInit()
	{
		InitializeCriticalSection( &g_AllocCS );
	}
} g_AllocCSInit;
#endif

#ifndef _X360
PLATFORM_INTERFACE void* Plat_Alloc( unsigned long size )
{
	EnterCriticalSection( &g_AllocCS );
#if !defined(STEAM) && !defined(NO_MALLOC_OVERRIDE)
		void *pRet = g_pMemAlloc->Alloc( size );
#else
		void *pRet = malloc( size );
#endif
	LeaveCriticalSection( &g_AllocCS );
	if ( pRet )
	{
		return pRet;
	}
	else
	{
#if !defined(STEAM) && !defined(NO_MALLOC_OVERRIDE)
		g_AllocError( size );
#endif
		return 0;
	}
}
#endif

#ifndef _X360
PLATFORM_INTERFACE void* Plat_Realloc( void *ptr, unsigned long size )
{
	EnterCriticalSection( &g_AllocCS );
#if !defined(STEAM) && !defined(NO_MALLOC_OVERRIDE)
		void *pRet = g_pMemAlloc->Realloc( ptr, size );
#else
		void *pRet = realloc( ptr, size );
#endif
	LeaveCriticalSection( &g_AllocCS );
	if ( pRet )
	{
		return pRet;
	}
	else
	{
#if !defined(STEAM) && !defined(NO_MALLOC_OVERRIDE)
		g_AllocError( size );
#endif
		return 0;
	}
}
#endif

#ifndef _X360
PLATFORM_INTERFACE void Plat_Free( void *ptr )
{
	EnterCriticalSection( &g_AllocCS );
#if !defined(STEAM) && !defined(NO_MALLOC_OVERRIDE)
		g_pMemAlloc->Free( ptr );
#else
		free( ptr );
#endif
	LeaveCriticalSection( &g_AllocCS );
}
#endif

#ifndef _X360
#if !defined(STEAM) && !defined(NO_MALLOC_OVERRIDE)
PLATFORM_INTERFACE void Plat_SetAllocErrorFn( Plat_AllocErrorFn fn )
{
	g_AllocError = fn;
}
#endif
#endif

#endif

PLATFORM_INTERFACE void* Plat_LoadLibrary(const char* path)
{
	Assert(path != NULL);
	return (void*)LoadLibrary(path);
}

PLATFORM_INTERFACE void Plat_UnloadLibrary(void* handle)
{
	Assert(handle != NULL);
	FreeLibrary((HMODULE)handle);
}

PLATFORM_INTERFACE void* Plat_FindProc(void* module_handle, const char* sym_name)
{
	Assert(module_handle != NULL);
	Assert(sym_name != NULL);
	return (void*)GetProcAddress((HMODULE)module_handle, sym_name);
}

PLATFORM_INTERFACE bool Plat_CWD(char* outname, size_t outSize)
{
	return ::GetCurrentDirectory(outSize, outname) != 0;
}

PLATFORM_INTERFACE bool Plat_GetCurrentDirectory(char* outname, size_t outSize)
{
	return ::GetCurrentDirectory(outSize, outname) != 0;
}

PLATFORM_INTERFACE bool Plat_GetExecutableDirectory(char* outpath, size_t len)
{
	Assert(outpath);
	::GetModuleFileName(NULL, outpath, len);
#ifdef _DEBUG
	Assert(GetLastError() == ERROR_SUCCESS);
#endif
}

PLATFORM_INTERFACE bool Plat_GetExecutablePath(char* outname, size_t len)
{
	Assert(outname);
}

PLATFORM_INTERFACE int Plat_GetPriority(int pid)
{
	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, TRUE, (DWORD)pid);
	int ret = (int)GetPriorityClass(hProc);
	CloseHandle(hProc);
	return ret;
}

PLATFORM_INTERFACE void Plat_SetPriority(int priority, int pid)
{
	DWORD prio = IDLE_PRIORITY_CLASS;
	switch(priority)
	{
		case PRIORITY_MAX:
			prio = HIGH_PRIORITY_CLASS;
			break;
		default:
			break;
	}

	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, TRUE, (DWORD)pid);
	SetPriorityClass(hProc, prio);
	CloseHandle(hProc);
}

PLATFORM_INTERFACE int Plat_GetPID()
{
	return (int)GetCurrentProcessId();
}

PLATFORM_INTERFACE unsigned int Plat_GetTickCount()
{
	return GetTickCount();
}

PLATFORM_INTERFACE ProcInfo_t Plat_QueryProcInfo(int pid)
{
	ProcInfo_t ret;
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	ret.cpus = info.dwNumberOfProcessors;
	return ret;
	// AM lazy, do it later	
}

PLATFORM_INTERFACE OSInfo_t Plat_QueryOSInfo()
{
	static OSInfo_t info;
	static bool init = false;
	if(!init)
	{
		memcpy(info.kname, "NT", strlen("NT"));
		memcpy(info.kver, "Unknown", strlen("Unknown"));
		memcpy(info.osname, "Windows", strlen("Windows"));
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		switch(sysinfo.wProcessorArchitecture)
		{
			case PROCESSOR_ARCHITECTURE_AMD64:
				memcpy(info.plat, "x86_64", strlen("x86_64"));
				break;
			case PROCESSOR_ARCHITECTURE_ARM:
				memcpy(info.plat, "ARM", 4);
				break;
			case 12:
				memcpy(info.plat, "ARM64", 6);
				break;
			case PROCESSOR_ARCHITECTURE_IA64:
				memcpy(info.plat, "IA64", 5);
				break;
			default:
				memcpy(info.plat, "x86", 4);
				break;
		}
		if(IsWindows10OrGreater())
			memcpy(info.osver, "10", 3);
		else if(IsWindows8OrGreater())
			memcpy(info.osver, "8", 2);
		else if(IsWindows7OrGreater())
			memcpy(info.osver, "7", 2);
		else if(IsWindowsVistaOrGreater())
			memcpy(info.osver, "Vista", strlen("Vista"));
		else if(IsWindowsXPOrGreater())
			memcpy(info.osver, "XP", 3);
		else
			memcpy(info.osver, "Fucking Ancient!", strlen("Fucking Ancient!"));
		init = true;
	}
	return info;
}

PLATFORM_INTERFACE void Plat_Chdir(const char* dir)
{
	_chdir(dir);
}

PLATFORM_INTERFACE int Plat_DirExists(const char* dir)
{
	return PathFileExistsA(dir) == TRUE;
}

PLATFORM_INTERFACE int Plat_FileExists(const char* file)
{
	return PathFileExistsA(file) == TRUE;
}

/*
Allocates memory in the virtual memory space of this program, very similar to VirtualAlloc.

Params:
	-	Starting addr of the region to allocate
	-	Size of the region
*/
PLATFORM_INTERFACE void* Plat_PageAlloc(void* start, size_t regsz)
{
	if(start) Assert(start % m_nPageSize == 0); /* Alignment check! */
	Assert(regsz != 0);
	return VirtualAlloc(start, (SIZE_T)regsz, MEM_COMMIT, PAGE_READWRITE);
}

/*
Frees memory in the virtual memory space of this program, very similar to VirtualFree

Params:
	-	The block to free
Returns:
	-	None
*/
PLATFORM_INTERFACE int Plat_PageFree(void* blk, size_t sz)
{
	Assert(blk);
	Assert(blk % m_nPageSize == 0); /* Alignment check */
	return VirtualFree(blk, (SIZE_T)sz, MEM_DECOMMIT) == 0 ? -1 : 0;
}

/*
Locks a page into the process memory, which guarantees that a page fault will not be triggered when the memory
is next accessed.

Params:
	-	The block to lock into memory
Returns:
	-	None
*/
PLATFORM_INTERFACE int Plat_PageLock(void* blk, size_t sz)
{
	Assert(blk % m_nPageSize == 0); /* Alignment check */
	Assert(blk);
	return VirtualLock(blk, sz) == 0 ? -1 : 0;
}

/*
Remaps the specified page to a new address, can also resize it.

Params:
	-	the block of memory (page aligned)
	-	old size
	-	flags
*/
PLATFORM_INTERFACE void* Plat_PageResize(void* blk, size_t pgsz)
{
	Assert(blk % m_nPageSize == 0);
	Assert(pgsz > 0);
	return VirtualAlloc(blk, pgsz, MEM_COMMIT, PAGE_READWRITE);
}

/*
Unlocks a page from the process memory, the next access to it will trigger a page fault

Params:
	-	The block of memory (page aligned)
Returns:
	-	0 for OK, other for errors
*/
PLATFORM_INTERFACE int Plat_PageUnlock(void* blk, size_t sz)
{
	Assert(blk);
	Assert(blk % m_nPageSize == 0);
	return VirtualUnlock(blk, sz) == 0 ? -1 : 0;
}

/*
Changes protection flags on the given region of memory

Params:
	-	The start of the page block
	-	The flags to apply
Returns:
	-	0 for OK, other for errors
*/
PLATFORM_INTERFACE int Plat_PageProtect(void* blk, size_t sz, int flags)
{
	Assert(blk);
	Assert(blk % m_nPageSize);
	int realflags = 0;
	if(flags & PAGE_PROT_EXEC && flags & PAGE_PROT_READ && flags & PAGE_PROT_WRITE)
		realflags |= PAGE_EXECUTE_READWRITE;
	else if(flags & PAGE_PROT_EXEC && flags & PAGE_PROT_READ)
		realflags |= PAGE_EXECUTE_READ;
	else if(flags & PAGE_PROT_READ && flags & PAGE_PROT_WRITE)
		realflags |= PAGE_READWRITE;
	else if(flags & PAGE_PROT_READ)
		realflags |= PAGE_READONLY;
	else
		realflags |= PAGE_NOACCESS;
	DWORD fuck = 0;
	return VirtualProtect(blk, sz, realflags, &fuck) == 0 ? -1 : 0;
}

/*
Gives advice on the page. This is based on the madvise syscall on Linux
May not be implemented for Windows, but can be useful on Linux/FreeBSD

Params:
	-	The block
	-	Advice to give
Returns:
	-	0 for OK, other for errors
*/
PLATFORM_INTERFACE int Plat_PageAdvise(void* blk, size_t sz, int advice)
{
	Assert(blk);
	Assert(blk % m_nPageSize == 0);
	return 0;
}

/*
Returns the path to the system temp directory

Params:
	-	The length of the buffer
	-	The buffer
Returns:
	-   Number of chars written into the buffer
*/
PLATFORM_INTERFACE size_t Plat_GetTmpDirectory(size_t buflen, char* buf)
{
	Assert(buf != NULL);
	return (size_t)GetTempPathA((DWORD)buflen, (LPSTR)buf);
}

/*
Returns a new temp file path

Params:
	-   Directory path for the temp file
	-	Prefix string
    -   Random number generator seed
    -   Pointer to a buffer that will hold the temp file name. Make it at least MAX_PATH chars
Returns:
	-   The unique number used in the random number generator
*/
PLATFORM_INTERFACE uint Plat_GetTmpFileName(const char* dir, const char* prefix, uint seed, char* buf)
{
	Assert(buf != NULL);
	return (uint)GetTempFileNameA((LPCSTR)dir, (LPCSTR)prefix, (UINT)seed, (LPSTR)buf);
}

#endif // _LINUX
