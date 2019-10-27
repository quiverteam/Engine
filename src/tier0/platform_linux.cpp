//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "tier0/platform.h"
#include "tier0/vcrmode.h"
#include "tier0/dbg.h"
#include "tier0/memalloc.h"

#include <sys/time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/utsname.h>
#include <sched.h>
#include <dirent.h>

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

PLATFORM_INTERFACE struct tm* Plat_localtime(const time_t *timep, struct tm *result)
{
	result = localtime( timep );
	return result;
}

PLATFORM_INTERFACE unsigned long long Plat_ValveTime()
{
	return 0xFFFFFFFFFFFFFFFF;
}