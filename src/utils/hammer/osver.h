//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
enum eOSVersion
{
    eUninitialized,
    eUnknown,
    eWin9x,
    eWinNT,
};

extern void       initOSVersion();
extern eOSVersion getOSVersion();
