//========= Copyright Valve Corporation, All rights reserved. ============//
#if !defined(__GNUC__)
#ifndef min
	#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
	#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif
#else
#pragma once

template<class a>
a max(const a& _a, const a& _b)
{
	return (((_a) > (_b)) ? (_a) : (_b));
}
template<class a>
a min(const a& _a, const a& _b)
{
	return (((_a) < (_b)) ? (_a) : (_b));
}

#endif