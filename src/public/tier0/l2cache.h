//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#ifndef CL2CACHE_H
#define CL2CACHE_H

#pragma once

#include "tier0/platform.h"

class P4Event_BSQ_cache_reference;

class DLL_CLASS_EXPORT CL2Cache
{
public:

	CL2Cache();
	~CL2Cache();

	void Start( void );
	void End( void );

	//-------------------------------------------------------------------------
	// GetL2CacheMisses
	//-------------------------------------------------------------------------
	int GetL2CacheMisses( void )
	{
		return m_iL2CacheMissCount;
	}

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, tchar *pchName );		// Validate our internal structures
#endif // DBGFLAG_VALIDATE

private:

	int								m_nID;

	P4Event_BSQ_cache_reference		*m_pL2CacheEvent;
	int64							m_i64Start;
	int64							m_i64End;
	int								m_iL2CacheMissCount;
};

#endif   // CL2CACHE_H
