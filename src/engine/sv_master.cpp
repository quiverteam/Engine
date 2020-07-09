//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "server_pch.h"
#include "server.h"
#include "master.h"
#include "proto_oob.h"
#include "sv_main.h" // SV_GetFakeClientCount()
#include "tier0/icommandline.h"
#include "FindSteamServers.h"
#include "filesystem_engine.h"
#include "tier0/vcrmode.h"
#include "sv_steamauth.h"
#include "hltvserver.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


bool g_bEnableMasterServerUpdater = true;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Heartbeat_f()
{
#ifndef NO_STEAM
	if( SteamGameServer() )
		SteamGameServer()->ForceHeartbeat();
#endif
}


static ConCommand heartbeat( "heartbeat", Heartbeat_f, "Force heartbeat of master servers", 0 );

