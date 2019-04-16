//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// Defines the entry point for the application.
//
//===========================================================================//
#pragma once

#include "dbg.h"

SpewRetval_t LauncherDefaultSpewFunc( SpewType_t spewType, char const *pMsg );
bool GetExecutableName( char *out, int outSize );