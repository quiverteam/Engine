//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef MODELSOUNDSCACHE_H
#define MODELSOUNDSCACHE_H
#ifdef _WIN32
#pragma once
#endif

#include "utlcachedfiledata.h"
#include "soundemittersystem/isoundemittersystembase.h"

#define MODELSOUNDSCACHE_VERSION		5

class CStudioHdr;

/* winemaker: #pragma pack(1) */
#include <pshpack1.h>
class CModelSoundsCache : public IBaseCacheInfo
{
public:
	CUtlVector< unsigned short > sounds;

	CModelSoundsCache();
	CModelSoundsCache( const CModelSoundsCache& src );

	void PrecacheSoundList();

	virtual void Save( CUtlBuffer& buf  );
	virtual void Restore( CUtlBuffer& buf  );
	virtual void Rebuild( char const *filename );

	static void FindOrAddScriptSound( CUtlVector< unsigned short >& sounds, char const *soundname );
	static void BuildAnimationEventSoundList( CStudioHdr *hdr, CUtlVector< unsigned short >& sounds );
private:
	char const *GetSoundName( int index );
};
/* winemaker: #pragma pack() */
/* winemaker:warning: Using 4 as the default alignment */
#include <pshpack4.h>

#endif // MODELSOUNDSCACHE_H
