//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef GAMEUI_UTIL_H
#define GAMEUI_UTIL_H
#ifdef _WIN32
#pragma once
#endif

char	*VarArgs( const char *format, ... );

#include "tier1/convar.h"

void GameUI_MakeSafeName( const char *oldName, char *newName, int newNameBufSize );

//-----------------------------------------------------------------------------
// Useful for game ui since game ui has a single active "splitscreen" owner and since
//  it can gracefully handle non-FCVAR_SS vars without code changes required.
//-----------------------------------------------------------------------------
class CGameUIConVarRef
{
public:
	CGameUIConVarRef( const char *pName );
	CGameUIConVarRef( const char *pName, bool bIgnoreMissing );
	CGameUIConVarRef( IConVar *pConVar );

	void Init( const char *pName, bool bIgnoreMissing );
	bool IsValid() const;
	bool IsFlagSet( int nFlags ) const;

	// Get/Set value
	float GetFloat() const;
	int GetInt() const;
	bool GetBool() const { return !!GetInt(); }
	const char *GetString() const;

	void SetValue( const char *pValue );
	void SetValue( float flValue );
	void SetValue( int nValue );
	void SetValue( bool bValue );

	const char *GetName() const;

	const char *GetDefault() const;

private:
	IConVar * m_pConVar;
	ConVar *m_pConVarState;
};

// In GAMUI we should never use the regular ConVarRef
#define ConVarRef CGameUIConVarRef

//-----------------------------------------------------------------------------
// Did we find an existing convar of that name?
//-----------------------------------------------------------------------------
FORCEINLINE bool CGameUIConVarRef::IsFlagSet( int nFlags ) const
{
	return ( m_pConVar->IsFlagSet( nFlags ) != 0 );
}

FORCEINLINE const char *CGameUIConVarRef::GetName() const
{
	return m_pConVar->GetName();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float
//-----------------------------------------------------------------------------
FORCEINLINE float CGameUIConVarRef::GetFloat() const
{
	return m_pConVarState->GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an int
//-----------------------------------------------------------------------------
FORCEINLINE int CGameUIConVarRef::GetInt() const 
{
	return m_pConVarState->GetInt();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string, return "" for bogus string pointer, etc.
//-----------------------------------------------------------------------------
FORCEINLINE const char *CGameUIConVarRef::GetString() const 
{
	Assert( !IsFlagSet( FCVAR_NEVER_AS_STRING ) );
	return m_pConVarState->GetString();
}


FORCEINLINE void CGameUIConVarRef::SetValue( const char *pValue )
{
	m_pConVar->SetValue( pValue );
}

FORCEINLINE void CGameUIConVarRef::SetValue( float flValue )
{
	m_pConVar->SetValue( flValue );
}

FORCEINLINE void CGameUIConVarRef::SetValue( int nValue )
{
	m_pConVar->SetValue( nValue );
}

FORCEINLINE void CGameUIConVarRef::SetValue( bool bValue )
{
	m_pConVar->SetValue( bValue ? 1 : 0 );
}

FORCEINLINE const char *CGameUIConVarRef::GetDefault() const
{
	return m_pConVarState->GetDefault();
}

#endif // GAMEUI_UTIL_H
