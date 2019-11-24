//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "pch_tier0.h"

#ifdef _WINDOWS
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#elif defined(_LINUX)
#include <x86intrin.h>
#endif

#include <stdio.h>

static bool cpuid(unsigned int function, unsigned int& out_eax, unsigned int& out_ebx, unsigned int& out_ecx, unsigned int& out_edx)
{
#ifdef _POSIX
	unsigned int tmp1, tmp2, tmp3, tmp4;
	asm("movl %4, %%eax\n\t"
		"cpuid\n\t"
		"movl %%eax, %0\n\t"
		"movl %%ebx, %1\n\t"
		"movl %%ecx, %2\n\t"
		"movl %%edx, %3\n\t"
		:	"=m"(tmp1), "=m"(tmp2), "=m"(tmp3), "=m"(tmp4)
		: "m"(function)
		: "eax", "ebx", "ecx", "edx");
		out_eax = tmp1;
		out_ebx = tmp2;
		out_ecx = tmp3;
		out_edx = tmp4;
	return true;
#else
	int ret[4];
	__cpuid(ret, function);
	out_eax = ret[0];
	out_ebx = ret[1];
	out_ecx = ret[2];
	out_edx = ret[3];
	return true;
#endif
}

bool CheckMMXTechnology(void)
{
    unsigned int eax,ebx,edx,unused;
    if ( !cpuid(1,eax,ebx,unused,edx) )
		return false;

    return ( edx & 0x800000 ) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: This is a bit of a hack because it appears 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
static bool IsWin98OrOlder()
{
	return false;
}


/* Checks if SSE is supported */
bool CheckSSETechnology(void)
{
    unsigned int eax,ebx,edx,unused;
    if ( !cpuid(1,eax,ebx,unused,edx) )
	{
		return false;
	}

    return ( edx & 0x2000000L ) != 0;
}

/* Checks if SSE2 is supported */
bool CheckSSE2Technology(void)
{
	unsigned int eax,ebx,edx,unused;
    if ( !cpuid(1,eax,ebx,unused,edx) )
		return false;

    return ( edx & 0x04000000 ) != 0;
}

/* Checks for SSE3 */
bool CheckSSE3Technology(void)
{
	unsigned int eax, ebx, edx, ecx;
	if(!cpuid(1,eax,ebx,ecx,edx))
		return false;
	return (ecx & 1) == 1;
}

/* Check for SSE4.1 */
bool CheckSSE41(void)
{
	unsigned int eax, ebx, edx, ecx;
	if(!cpuid(1,eax,ebx,ecx,edx))
		return false;
	return (ecx & (1<<19) >> 19) == 1;
}

bool CheckSSE42(void)
{
	unsigned int eax, ebx, edx, ecx;
	if(!cpuid(1,eax,ebx,ecx,edx))
		return false;
	return (ecx & (1<<20) >> 20) == 1;
}

bool CheckAVX(void)
{
	unsigned int eax, ebx, edx, ecx;
	if(!cpuid(1,eax,ebx,ecx,edx))
		return false;
	return (ecx & (1<<28) >> 28) == 1;
}

bool CheckAVX2(void)
{
	unsigned int eax, ebx, edx, ecx;
	if(!cpuid(7,eax,ebx,ecx,edx))
		return false;
	return (ecx & (1<<5) >> 5) == 1;
}

/* Checks for 3DNow */
/* Leave this as false, since 3dnow is gone */
bool Check3DNowTechnology(void)
{
	return false;
}

/* CHeck for CMov */
bool CheckCMOVTechnology()
{
	unsigned int eax,ebx,edx,unused;
    if ( !cpuid(1,eax,ebx,unused,edx) )
		return false;

    return ( edx & (1<<15) ) != 0;
}

bool CheckFCMOVTechnology(void)
{
    unsigned int eax,ebx,edx,unused;
    if ( !cpuid(1,eax,ebx,unused,edx) )
		return false;

    return ( edx & (1<<16) ) != 0;
}

bool CheckRDTSCTechnology(void)
{
	unsigned int eax,ebx,edx,unused;
    if ( !cpuid(1,eax,ebx,unused,edx) )
		return false;

    return ( edx & 0x10 ) != 0;
}

// Return the Processor's vendor identification string, or "Generic_x86" if it doesn't exist on this CPU
const tchar* GetProcessorVendorId()
{
	unsigned int unused, VendorIDRegisters[3];

	static tchar VendorID[13];
	
	memset( VendorID, 0, sizeof(VendorID) );
	if ( !cpuid(0,unused, VendorIDRegisters[0], VendorIDRegisters[2], VendorIDRegisters[1] ) )
	{
		if ( IsPC() )
		{
			_tcscpy( VendorID, _T( "Generic_x86" ) ); 
		}
		else if ( IsX360() )
		{
			_tcscpy( VendorID, _T( "PowerPC" ) ); 
		}
	}
	else
	{
		memcpy( VendorID+0, &(VendorIDRegisters[0]), sizeof( VendorIDRegisters[0] ) );
		memcpy( VendorID+4, &(VendorIDRegisters[1]), sizeof( VendorIDRegisters[1] ) );
		memcpy( VendorID+8, &(VendorIDRegisters[2]), sizeof( VendorIDRegisters[2] ) );
	}

	return VendorID;
}

// Returns non-zero if Hyper-Threading Technology is supported on the processors and zero if not.  This does not mean that 
// Hyper-Threading Technology is necessarily enabled.
static bool HTSupported(void)
{
	const unsigned int HT_BIT		 = 0x10000000;  // EDX[28] - Bit 28 set indicates Hyper-Threading Technology is supported in hardware.
	const unsigned int FAMILY_ID     = 0x0f00;      // EAX[11:8] - Bit 11 thru 8 contains family processor id
	const unsigned int EXT_FAMILY_ID = 0x0f00000;	// EAX[23:20] - Bit 23 thru 20 contains extended family  processor id
	const unsigned int PENTIUM4_ID   = 0x0f00;		// Pentium 4 family processor id

	unsigned int unused,
				  reg_eax = 0, 
				  reg_edx = 0,
				  vendor_id[3] = {0, 0, 0};

	// verify cpuid instruction is supported
	if( !cpuid(0,unused, vendor_id[0],vendor_id[2],vendor_id[1]) 
	 || !cpuid(1,reg_eax,unused,unused,reg_edx) )
	 return false;

	//  Check to see if this is a Pentium 4 or later processor
	if (((reg_eax & FAMILY_ID) ==  PENTIUM4_ID) || (reg_eax & EXT_FAMILY_ID))
		/* Converted multichar constants to hex constants to get the compiler to be quiet */
		if(vendor_id[0] == 0x756E6547 && vendor_id[1] == 0x49656E69 && vendor_id[2] == 0x6C65746E)
			return (reg_edx & HT_BIT) != 0;	// Genuine Intel Processor with Hyper-Threading Technology

	return false;  // This is not a genuine Intel processor. :/
}

// Returns the number of logical processors per physical processors.
static uint8 LogicalProcessorsPerPackage(void)
{
	// EBX[23:16] indicate number of logical processors per package
	const unsigned NUM_LOGICAL_BITS = 0x00FF0000;

    unsigned int unused, reg_ebx = 0;

	if ( !HTSupported() ) 
		return 1; 

	if ( !cpuid(1,unused,reg_ebx,unused,unused) )
		return 1;

	return (uint8) ((reg_ebx & NUM_LOGICAL_BITS) >> 16);
}

// Measure the processor clock speed by sampling the cycle count, waiting
// for some fraction of a second, then measuring the elapsed number of cycles.
static int64 CalculateClockSpeed()
{
#if defined( _WIN32 )
#if !defined( _CERT )
	LARGE_INTEGER waitTime, startCount, curCount;
	CCycleCount start, end;

	// Take 1/32 of a second for the measurement.
	QueryPerformanceFrequency( &waitTime );
	int scale = 5;
	waitTime.QuadPart >>= scale;

	QueryPerformanceCounter( &startCount );
	start.Sample();
	do
	{
		QueryPerformanceCounter( &curCount );
	}
	while ( curCount.QuadPart - startCount.QuadPart < waitTime.QuadPart );
	end.Sample();

	return (end.m_Int64 - start.m_Int64) << scale;
#else
	return 3200000000LL;
#endif
#elif defined(_LINUX)
	uint64 CalculateCPUFreq(); // from cpu_linux.cpp
	int64 freq =(int64)CalculateCPUFreq();
	if ( freq == 0 ) // couldn't calculate clock speed
	{
		Error( "Unable to determine CPU Frequency\n" );
	}
	return freq;
#endif
}

const CPUInformation* GetCPUInformation()
{
	static CPUInformation pi;

	// Has the structure already been initialized and filled out?
	if ( pi.m_Size == sizeof(pi) )
		return &pi;

	// Redundant, but just in case the user somehow messes with the size.
	memset(&pi, 0x0, sizeof(pi));

	// Fill out the structure, and return it:
	pi.m_Size = sizeof(pi);

	// Grab the processor frequency:
	pi.m_Speed = CalculateClockSpeed();
	
	// Get the logical and physical processor counts:
	pi.m_nLogicalProcessors = LogicalProcessorsPerPackage();

#if defined(_WIN32) && !defined( _X360 )
	SYSTEM_INFO si;
	ZeroMemory( &si, sizeof(si) );

	GetSystemInfo( &si );

	pi.m_nPhysicalProcessors = (unsigned char)(si.dwNumberOfProcessors / pi.m_nLogicalProcessors);
	pi.m_nLogicalProcessors = (unsigned char)(pi.m_nLogicalProcessors * pi.m_nPhysicalProcessors);

	// Make sure I always report at least one, when running WinXP with the /ONECPU switch, 
	// it likes to report 0 processors for some reason.
	if ( pi.m_nPhysicalProcessors == 0 && pi.m_nLogicalProcessors == 0 )
	{
		pi.m_nPhysicalProcessors = 1;
		pi.m_nLogicalProcessors  = 1;
	}
#elif defined( _X360 )
	pi.m_nPhysicalProcessors = 3;
	pi.m_nLogicalProcessors  = 6;
#elif defined(_LINUX)
	// TODO: poll /dev/cpuinfo when we have some benefits from multithreading
	pi.m_nPhysicalProcessors = 1;
	pi.m_nLogicalProcessors  = 1;
#endif

	// Determine Processor Features:
	pi.m_bRDTSC			= CheckRDTSCTechnology();
	pi.m_bCMOV			= CheckCMOVTechnology();
	pi.m_bFCMOV			= CheckFCMOVTechnology();
	pi.m_bMMX			= CheckMMXTechnology();
	pi.m_bSSE			= CheckSSETechnology();
	pi.m_bSSE2			= CheckSSE2Technology();
	pi.m_b3DNow			= Check3DNowTechnology();
	pi.m_szProcessorID	= (tchar*)GetProcessorVendorId();
	pi.m_bHT			= HTSupported();
	pi.m_bSSE3			= CheckSSE3Technology();
	pi.m_bAVX2			= CheckAVX2();
	pi.m_bAVX			= CheckAVX();
	pi.m_bSSE41			= CheckSSE41();
	pi.m_bSSE42			= CheckSSE42();
	
	return &pi;
}