/*

Things like declspec defs so things will work on different platforms

Everything is prefixed with V as to not cause conflicts

*/
#pragma once

#if defined(WIN32) || defined(WIN64)

#define VFORCEINLINE	__forceinline
#define VPACKED_CLASS

#ifndef WIN64 // invalid in 64bit
#define VNAKED			__declspec(naked)
#else
#define VNAKED
#endif

#define VNOTHROW		__declspec(nothrow)
#define VALIGN(x)		__declspec(align(x))
#define VSEGMENT(x)		__declspec(code_seg(x))
#define VNOINLINE		__declspec(noinline)
#define VNORETURN		__declspec(noreturn)
#define VDEPRECIATED	__declspec(depreciated)

#elif defined(POSIX) || defined(LINUX)

#define VFORCEINLINE	__attribute__((always_inline))
#define VPACKED_CLASS	__attribute__((__packed__))
#define VNAKED			__attribute__((naked))
#define VNOTHROW		__attribute__((nothrow))
#define VALIGN(x)		__attribute__((aligned(x)))
#define VSEGMENT(x)		__attribute__((section(x)))
#define VNOINLINE		__attribute__((noinline))
#define VNORETURN		__attribute__((noreturn))
#define VDEPRECIATED	__attribute__((depreciated))

#else
	
#define VFORCEINLINE
#define VPACKED_CLASS
#define VNAKED
#define VNOTHROW
#define VALIGN(x)
#define VSEGMENT(x)
#define VNOINLINE
#define VNORETURN
#define VDEPRECIATED

#endif