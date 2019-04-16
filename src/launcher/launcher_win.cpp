//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// Defines the entry point for the application.
//
//===========================================================================//
#if defined( _WIN32 ) && !defined( _X360 )
#include <windows.h>
#include "shlwapi.h" // registry stuff
#endif

#include <stdio.h>
#include "tier0/icommandline.h"
#include "engine_launcher_api.h"
#include "tier0/vcrmode.h"
#include "ifilesystem.h"
#include "tier1/interface.h"
#include "tier0/dbg.h"
#include "iregistry.h"
#include "appframework/IAppSystem.h"
#include "appframework/AppFramework.h"
#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include "tier0/platform.h"
#include "tier0/memalloc.h"
#include "filesystem.h"
#include "tier1/utlrbtree.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "materialsystem/imaterialsystem.h"
#include "istudiorender.h"
#include "vgui/IVGui.h"
#include "IHammer.h"
#include "datacache/idatacache.h"
#include "datacache/imdlcache.h"
#include "vphysics_interface.h"
#include "filesystem_init.h"
#include "vstdlib/iprocessutils.h"
#include "video/iavi.h"
#include "tier1/tier1.h"
#include "tier2/tier2.h"
#include "tier3/tier3.h"
#include "inputsystem/iinputsystem.h"
#include "filesystem/IQueuedLoader.h"
#include "reslistgenerator.h"
#include "tier1/fmtstr.h"

SpewRetval_t LauncherDefaultSpewFunc( SpewType_t spewType, char const *pMsg )
{
#ifndef _CERT
	OutputDebugStringA( pMsg );
	switch( spewType )
	{
	case SPEW_MESSAGE:
	case SPEW_LOG:
		return SPEW_CONTINUE;

	case SPEW_WARNING:
		if ( !stricmp( GetSpewOutputGroup(), "init" ) )
		{
			::MessageBox( NULL, pMsg, "Warning!", MB_OK | MB_SYSTEMMODAL | MB_ICONERROR );
		}
		return SPEW_CONTINUE;

	case SPEW_ASSERT:
		if ( !ShouldUseNewAssertDialog() )
			::MessageBox( NULL, pMsg, "Assert!", MB_OK | MB_SYSTEMMODAL | MB_ICONERROR );
		return SPEW_DEBUGGER;
	
	case SPEW_ERROR:
	default:
		::MessageBox( NULL, pMsg, "Error!", MB_OK | MB_SYSTEMMODAL | MB_ICONERROR );
		_exit( 1 );
	}
#else
	if ( spewType != SPEW_ERROR)
		return SPEW_CONTINUE;
	_exit( 1 );
#endif
}

bool GetExecutableName( char *out, int outSize )
{
	if ( !::GetModuleFileName( ( HINSTANCE )GetModuleHandle( NULL ), out, outSize ) )
	{
		return false;
	}
	return true;
}