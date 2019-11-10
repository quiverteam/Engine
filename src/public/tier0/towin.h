/*

Named similar to ToGL

Defines some structures and WIN32 API functions for Linux and other (mostly) Posix-compliant operating systems

*/
#pragma once

#ifdef POSIX 

#if 0
typedef unsigned long DWORD;
typedef unsigned char UCHAR;
typedef unsigned int UINT;
typedef unsigned long long ULONGLONG;
typedef int INT;
typedef long long LONGLONG;

#ifdef UNICODE
	typedef WCHAR TCHAR;
#else
	typedef char TCHAR;
#endif

#define MAX_PATH 260


typedef struct _FILETIME 
{
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

#if 0
/* WIN32 file search data, pulled from MS docs */
typedef struct _WIN32_FIND_DATA
{
  DWORD    dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD    nFileSizeHigh;
  DWORD    nFileSizeLow;
  DWORD    dwReserved0;
  DWORD    dwReserved1;
  TCHAR    cFileName[MAX_PATH];
  TCHAR    cAlternateFileName[14];
} WIN32_FIND_DATA, *PWIN32_FIND_DATA, *LPWIN32_FIND_DATA;
#endif
#endif
#endif //_POSIX