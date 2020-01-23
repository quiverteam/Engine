//========= Copyright (C) 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef _WIN32

#include "tier0/platform.h"
#include "tier0/vcrmode.h"
#include "tier0/dbg.h"
#include "tier0/memalloc.h"

#include <sys/time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/ptrace.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/utsname.h>
#include <sched.h>
#include <dirent.h>
#include <errno.h>

/* Standard includes */
#include <memory>

double Plat_FloatTime()
{
        struct timeval  tp;
        static int      secbase = 0;

        gettimeofday( &tp, NULL );

        if ( !secbase )
        {
                secbase = tp.tv_sec;
                return ( tp.tv_usec / 1000000.0 );
        }

	if (VCRGetMode() == VCR_Disabled)
		return (( tp.tv_sec - secbase ) + tp.tv_usec / 1000000.0 );
	
	return VCRHook_Sys_FloatTime( ( tp.tv_sec - secbase ) + tp.tv_usec / 1000000.0 );
}

unsigned int Plat_MSTime()
{
        struct timeval  tp;
        static int      secbase = 0;

        gettimeofday( &tp, NULL );

        if ( !secbase )
        {
                secbase = tp.tv_sec;
                return ( tp.tv_usec / 1000.0 );
        }

	return (unsigned long)( ( tp.tv_sec - secbase )*1000.0f + tp.tv_usec / 1000.0 );

}



bool vtune( bool resume )
{
	return false;
}


// -------------------------------------------------------------------------------------------------- //
// Memory stuff.
// -------------------------------------------------------------------------------------------------- //

PLATFORM_INTERFACE void Plat_DefaultAllocErrorFn( unsigned long size )
{
}

typedef void (*Plat_AllocErrorFn)( unsigned long size );
Plat_AllocErrorFn g_AllocError = Plat_DefaultAllocErrorFn;

PLATFORM_INTERFACE void* Plat_Alloc( unsigned long size )
{
#ifndef NO_MALLOC_OVERRIDE
	void *pRet = g_pMemAlloc->Alloc( size );
	if ( pRet )
	{
		return pRet;
	}
	else
	{
		g_AllocError( size );
		return 0;
	}
#else
	return malloc(size);
#endif
}


PLATFORM_INTERFACE struct tm* Plat_localtime(const time_t* timep, struct tm* result) 
{
	result = localtime(timep);
	return result;
}

PLATFORM_INTERFACE unsigned long long Plat_ValveTime()
{
	return 0xFFFFFFFFFFFFFFFF;
}

PLATFORM_INTERFACE void* Plat_Realloc( void *ptr, unsigned long size )
{
#ifndef NO_MALLOC_OVERRIDE
	void *pRet = g_pMemAlloc->Realloc( ptr, size );
	if ( pRet )
	{
		return pRet;
	}
	else
	{
		g_AllocError( size );
		return 0;
	}
#else
	return realloc(ptr, size);
#endif
}


PLATFORM_INTERFACE void Plat_Free( void *ptr )
{
#ifndef NO_MALLOC_OVERRIDE
	g_pMemAlloc->Free( ptr );
#else
	free(ptr);
#endif
}


PLATFORM_INTERFACE void Plat_SetAllocErrorFn( Plat_AllocErrorFn fn )
{
	g_AllocError = fn;
}

static char g_CmdLine[ 2048 ];
PLATFORM_INTERFACE void Plat_SetCommandLine( const char *cmdLine )
{
	strncpy( g_CmdLine, cmdLine, sizeof(g_CmdLine) );
	g_CmdLine[ sizeof(g_CmdLine) -1 ] = 0;
}

PLATFORM_INTERFACE const tchar *Plat_GetCommandLine()
{
	return g_CmdLine;
}

//
// Implementation of dynamic lib loading code for posix
//
PLATFORM_INTERFACE void* Plat_LoadLibrary(const char* path)
{
	Assert(path != NULL);
	// Might change this to RTLD_NOW if it becomes an issue, but this should be fine
	return dlopen(path, RTLD_LAZY);
}

PLATFORM_INTERFACE void Plat_UnloadLibrary(void* handle)
{
	Assert(handle != NULL);
	dlclose(handle);
}

PLATFORM_INTERFACE void* Plat_FindProc(void* module_handle, const char* sym_name)
{
	Assert(module_handle != NULL);
	Assert(sym_name != NULL);
	return dlsym(module_handle, sym_name);
}

//
// Implementation of executable handling code
//


PLATFORM_INTERFACE bool Plat_CWD(char* outname, size_t outSize)
{
	return getcwd(outname, outSize) != 0;
}

PLATFORM_INTERFACE bool Plat_GetCurrentDirectory(char* outname, size_t outSize)
{
	return getcwd(outname, outSize) != 0;
}

PLATFORM_INTERFACE bool Plat_GetExecutablePath(char* outname, size_t len)
{
	Assert(outname);

	// Full path to the directory, we will read from procfs
	char pathbuf[128];
	sprintf(pathbuf, "/proc/%i/exe", getpid());
	int nlen = readlink(pathbuf, outname, len);

	Assert(nlen != -1);
	Assert(nlen <= len);

	// Null terminate the string
	outname[nlen] = '\0';

	return true;
}

PLATFORM_INTERFACE bool Plat_GetExecutableDirectory(char* outpath, size_t len)
{
	Assert(outpath);

	// Full path to the directory, we will read from procfs
	char pathbuf[32];
	sprintf(pathbuf, "/proc/%i/exe", getpid());
	int nlen = readlink(pathbuf, outpath, len);
	Assert(nlen <= len);
	
	if(nlen == -1)
		return false;

	Assert(nlen <= len);

	// walk back until we hit a '/'
	for(size_t i = nlen; i >= 0; i--)
	{
		if(pathbuf[i] == '/')
		{
			pathbuf[i] = '\0';
			break;
		}
	}
	return true;
}

PLATFORM_INTERFACE int Plat_GetPriority(int pid)
{
	return getpriority(PRIO_PROCESS, pid);
}

PLATFORM_INTERFACE void Plat_SetPriority(int priority, int pid)
{
	static const int MaxPriority = sched_get_priority_max(SCHED_OTHER);
	static const int MinPriority = sched_get_priority_min(SCHED_OTHER);

	switch(priority)
	{
		case PRIORITY_MAX:
			priority = MaxPriority;
			break;
		case PRIORITY_MIN:
			priority = MinPriority;
			break;
	}
	setpriority(PRIO_PROCESS, pid, priority);
}

PLATFORM_INTERFACE int Plat_GetPID()
{
	return (int)getpid();
}

PLATFORM_INTERFACE unsigned int Plat_GetTickCount()
{
	FILE* fs = fopen("/proc/uptime", "r");
	if(!fs)
		return 0;
	
	char buf[64];
	fgets(buf, 64, fs);
	fclose(fs);

	// we only need the first number (processor uptime)
	char* uptime = strtok(buf, " ");

	if(!uptime)
		return 0;

	return (unsigned)(atof(uptime) / 1000.0f);
}

bool Plat_IsInDebugSession()
{
	return false; /* None of dat shit */
}

PLATFORM_INTERFACE ProcInfo_t Plat_QueryProcInfo(int pid)
{
	ProcInfo_t pinfo;
	return pinfo;
}

PLATFORM_INTERFACE OSInfo_t Plat_QueryOSInfo()
{
	OSInfo_t osinfo;
	utsname info;
	uname(&info);
	memcpy(osinfo.osname, info.sysname, strlen(info.sysname));
	memcpy(osinfo.osver, info.version, strlen(info.version));
	memcpy(osinfo.kver, info.release, strlen(info.release));
	memcpy(osinfo.kname, "Linux", strlen("Linux")); //TODO: lol
	return osinfo;
}


PLATFORM_INTERFACE void Plat_Chdir(const char* dir)
{
	chdir(dir);
}

PLATFORM_INTERFACE int Plat_DirExists(const char* dir)
{
	DIR* pdir = opendir(dir);
	if(pdir)
	{
		closedir(pdir);
		return 1;
	}
	else
		return 0;
}

PLATFORM_INTERFACE int Plat_FileExists(const char* file)
{
	return access(file, F_OK) != -1;
}

/*

Page allocation API

*/

PLATFORM_INTERFACE size_t Plat_GetPageSize()
{
	return getpagesize();
}

const static size_t m_nPageSize = Plat_GetPageSize();

/* Need a quick descriptor to hold some stuff */
/* This will simply hold some info about a page region */
struct linux_page_region
{
	void* start;
	size_t length;
};

/* Note: This doesn't mean the max allocated pages, just the regions */
/* Each time you call PageAlloc an entry is created, but generally you SHOULD be resizing your pages instead of allocating more regions */
/* May change this in the future */
#define MAX_ALLOCATED_PAGES	64

static linux_page_region g_AllocatedPages[MAX_ALLOCATED_PAGES];

/* Added to ease changes later */
linux_page_region* find_page_region(void* blk)
{
	for(int i = 0; i < MAX_ALLOCATED_PAGES; i++)
	{
		if(blk == g_AllocatedPages[i].start)
			return &g_AllocatedPages[i];
	}
	return nullptr;
}

/*
Allocates memory in the virtual memory space of this program, very similar to VirtualAlloc.

Params:
	-	Starting addr of the region to allocate
	-	Size of the region
*/
PLATFORM_INTERFACE void* Plat_PageAlloc(void* start, size_t regsz)
{
	if(start) Assert((size_t)start % m_nPageSize == 0); /* Alignment check! */
	Assert(regsz != 0);
	linux_page_region* page = find_page_region(0);
	if(page)
	{
		/* Map a portion of memory, last two args are ignored with MAP_ANON */
		void* ret = mmap(start, regsz, PROT_WRITE | PROT_READ, MAP_ANONYMOUS, 0, 0);
		if(ret == MAP_FAILED)
		{
			AssertMsg(ret != MAP_FAILED, "Failed to allocate page. Errno=%u", errno);
			return nullptr;	
		}
		page->start = ret;
		page->length = regsz;
		return ret;
	}
	AssertMsg(0, "Plat_PageAlloc: Internal error while allocating page region: Internal page descriptor pool full");
	return nullptr;
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
	Assert((size_t)blk % m_nPageSize == 0); /* Alignment check */
	return munmap(blk, sz);
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
	Assert((size_t)blk % m_nPageSize == 0); /* Alignment check */
	Assert(blk);
	return mlock(blk, sz);
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
	Assert((size_t)blk % m_nPageSize == 0);
	Assert(pgsz > 0);
	linux_page_region* page = find_page_region(blk);
	if(!page)
	{
		AssertMsg(page, "Plat_PageResize: Invalid page. blk=0x%X", (size_t)blk);
		return nullptr;
	}
	void* ret = mremap(page->start, page->length, pgsz, MREMAP_MAYMOVE);
	if(ret == MAP_FAILED)
	{
		/* EAGAIN is returned if there is not enough memory to do a swap with the region locked in */
		if(errno == EAGAIN)
		{
			munlock(page->start, page->length);
			return Plat_PageResize(blk, pgsz);
		}
		AssertMsg(ret != MAP_FAILED, "Plat_PageResize: Unknown error. errno=%u blk=0x%X new page size=%u", errno, (size_t)blk, pgsz);
		return nullptr;
	}
	page->length = pgsz;
	page->start = ret;
	return ret;
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
	Assert((size_t)blk % m_nPageSize == 0);
	return munlock(blk, sz);
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
	Assert((size_t)blk % m_nPageSize);
	int realflags = 0;
	if(flags & PAGE_PROT_EXEC)
		realflags |= PROT_EXEC;
	if(flags & PAGE_PROT_READ)
		realflags |= PROT_READ;
	if(flags & PAGE_PROT_WRITE)
		realflags |= PROT_WRITE;
	if(flags & PAGE_PROT_NONE)
		realflags = 0;
	return mprotect(blk, sz, realflags);
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
	Assert((size_t)blk % m_nPageSize == 0);
	int realflags = 0;
	if(advice & PAGE_ADVICE_NEED)
		realflags |= MADV_WILLNEED;
	if(advice & PAGE_ADVICE_DONTNEED)
		realflags |= MADV_DONTNEED;
	if(advice & PAGE_ADVICE_RANDOM)
		realflags |= MADV_RANDOM;
	if(advice & PAGE_ADVICE_SEQ)
		realflags |= MADV_SEQUENTIAL;
	if(advice & PAGE_ADVICE_NORMAL)
		realflags |= MADV_NORMAL;
	return madvise(blk, sz, realflags);
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

	static const char* tmppath = "/tmp/";
	static size_t tmppath_len = strlen(tmppath);
	for(int i = 0; i < buflen; i++)
		buf[i] = tmppath[i];
	return tmppath_len;
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
	size_t dirlen = strlen(dir);
	size_t preflen = strlen(prefix);
	Assert(dirlen < MAX_PATH-14);

	/* Just going to ignore the seed for now */
	/* Write dir into buffer */
	int i;
	for(i = 0; i < dirlen; i++)
		buf[i] = dir[i];
	for(int g = 0; g < preflen; i++,g++)
		buf[i] = prefix[g];
	for(int g = 0; i < MAX_PATH && g < 20; i++, g++)
		buf[i] = static_cast<char>(0x61 + (rand() % 26));
	return (seed == 0 ? 32943 : seed);
}

/*
Creates a new process

Params:
	-	The process image name, for example C:/Users/poop.exe
	-	Command line to pass to the new process
	-	Environment vars to pass to the new process, if NULL, it will use the calling process' envs
Returns:
	-	Boolean if the process started OK
*/
PLATFORM_INTERFACE bool Plat_CreateProcess(const char* exe, const char** cmdline, const char** _environ)
{
	Assert(exe);
	pid_t pid = fork();
	if(pid)
	{
		const char** realenviron = (const char**)environ;
		if(_environ)
			realenviron = _environ;
		if(!cmdline)
			cmdline = (const char**)&"";
		execve(exe, (char* const*)cmdline, (char* const*)realenviron);
		/* No exit required */
	}
	return true;
}

#endif //_WIN32
