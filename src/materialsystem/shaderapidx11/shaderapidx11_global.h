//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef SHADERAPIDX11_GLOBAL_H
#define SHADERAPIDX11_GLOBAL_H

#ifdef _WIN32
#pragma once
#endif

#include "shaderapidx9/shaderapi_global.h"
#include "tier2/tier2.h"

//-----------------------------------------------------------------------------
// The main hardware config interface
//-----------------------------------------------------------------------------
inline IMaterialSystemHardwareConfig* HardwareConfig()
{
	return g_pMaterialSystemHardwareConfig;
}

#endif // SHADERAPIDX11_GLOBAL_H
