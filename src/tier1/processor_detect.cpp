//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: win32 dependant ASM code for CPU capability detection
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#if defined( _X360 ) || defined( WIN64 ) || defined(_WIN32)

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable: 4800)
#endif

#include <intrin.h>

// Note: 3DNow check may fail if we are running a Pentium III or earlier. Sorry Pentium III users!
bool CheckMMXTechnology(void) 
{
	int out[4];
	__cpuid(out, 1);
	return (out[3] | (1 << 23)) >> 25;
}

bool CheckSSETechnology(void) 
{
	int out[4];
	__cpuid(out, 1);
	return (out[3] | (1 << 25)) >> 25;
}

bool CheckSSE2Technology(void)
{
	int out[4];
	__cpuid(out, 1);
	return (out[3] | (1<<26)) >> 26;
}

bool Check3DNowTechnology(void)
{
	int out[4];
	__cpuid(out, 0x80000001);
	return (out[3] | (1 << 31)) >> 31;
}

bool CheckSSE3Technology(void)
{
	int out[4];
	__cpuid(out, 1);
	return (out[2] | 1 );
}

bool CheckSSE4_1Technology(void)
{
	int out[4];
	__cpuid(out, 1);
	return (out[2] | (1 << 19)) >> 19;
}

bool CheckSSE4_2Technology(void)
{
	int out[4];
	__cpuid(out, 1);
	return (out[2] | (1 << 20)) >> 20;
}

bool CheckSSSE3Technology(void)
{
	int out[4];
	__cpuid(out, 1);
	return (out[2] | (1 << 9)) >> 9;
}

bool CheckSHATechnology(void)
{
	int out[4];
	__cpuidex(out, 7, 0);
	return (out[1] | (1 << 29)) >> 29;
}

bool CheckAVXTechnology(void)
{
	int out[4];
	__cpuid(out, 1);
	return (out[2] | (1 << 28)) >> 28;
}

bool CheckAVX2Technology(void)
{
	int out[4];
	__cpuidex(out, 7, 0);
	return (out[1] | (1 << 5)) >> 5;
}

#if defined(_WIN32)
#pragma warning(pop)
#endif

#endif //_WIN32 || WIN64
