//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef SCENECACHE_H
#define SCENECACHE_H
#ifdef _WIN32
#pragma once
#endif

#include "utlcachedfiledata.h"
#include "soundemittersystem/isoundemittersystembase.h"

class CChoreoEvent;

#define SCENECACHE_VERSION		7

/* winemaker: #pragma pack(1) */
#include <pshpack1.h>
class CSceneCache : public IBaseCacheInfo
{
public:
	unsigned int		msecs;
	CUtlVector< unsigned short > sounds;

	CSceneCache();
	CSceneCache( const CSceneCache& src );

	int	GetSoundCount() const;
	char const *GetSoundName( int index );

	virtual void Save( CUtlBuffer& buf  );
	virtual void Restore( CUtlBuffer& buf  );
	virtual void Rebuild( char const *filename );

	static unsigned int ComputeSoundScriptFileTimestampChecksum();
	static void PrecacheSceneEvent( CChoreoEvent *event, CUtlVector< unsigned short >& soundlist );
};
/* winemaker: #pragma pack() */
/* winemaker:warning: Using 4 as the default alignment */
#include <pshpack4.h>

#endif // SCENECACHE_H
