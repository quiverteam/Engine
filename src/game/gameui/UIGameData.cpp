#include "cbase.h"
#include "UIGameData.h"
#include "VGenericConfirmation.h"

//setup in GameUI_Interface.cpp
extern const char *COM_GetModDirectory( void );

ConVar x360_audio_english("x360_audio_english", "0", 0, "Keeps track of whether we're forcing english in a localized language." );

ConVar demo_ui_enable( "demo_ui_enable", "", FCVAR_DEVELOPMENTONLY, "Suffix for the demo UI" );
ConVar demo_connect_string( "demo_connect_string", "", FCVAR_DEVELOPMENTONLY, "Connect string for demo UI" );

///Asyncronous Operations

ConVar mm_ping_max_green( "ping_max_green", "70" );
ConVar mm_ping_max_yellow( "ping_max_yellow", "140" );
ConVar mm_ping_max_red( "ping_max_red", "250" );

namespace BaseModUI
{
	CUIGameData* CUIGameData::m_Singleton = 0;
	bool CUIGameData::m_bModuleShutDown = false;

	CUIGameData* CUIGameData::Get()
	{
		if (!m_Singleton && !m_bModuleShutDown)
			m_Singleton = new CUIGameData();

		return m_Singleton;
	}

	void CUIGameData::Shutdown()
	{
		delete m_Singleton;
		m_Singleton = NULL;
		m_bModuleShutDown = true;
	}

	void CUIGameData::OnGameUIPostInit()
	{
	}

	void CUIGameData::NeedConnectionProblemWaitScreen()
	{
	}

	void CUIGameData::ShowPasswordUI(char const*pchCurrentPW)
	{
	}

	void CUIGameData::RunFrame()
	{
	}

	void CUIGameData::RunFrame_Storage()
	{
	}

char const * CUIGameData::GetPlayerName( XUID playerID, char const *szPlayerNameSpeculative )
{
	static CGameUIConVarRef cl_names_debug( "cl_names_debug" );
	if ( cl_names_debug.GetInt() )
		return "WWWWWWWWWWWWWWW";

#if !defined( _X360 ) && !defined( NO_STEAM )
	if ( steamapicontext && steamapicontext->SteamUtils() &&
		steamapicontext->SteamFriends() && steamapicontext->SteamUser() )
	{
		int iIndex = m_mapUserXuidToName.Find( playerID );
		if ( iIndex == m_mapUserXuidToName.InvalidIndex() )
		{
			char const *szName = steamapicontext->SteamFriends()->GetFriendPersonaName( playerID );
			if ( szName && *szName )
			{
				iIndex = m_mapUserXuidToName.Insert( playerID, szName );
			}
		}

		if ( iIndex != m_mapUserXuidToName.InvalidIndex() )
			return m_mapUserXuidToName.Element( iIndex ).Get();
	}
#endif

	return szPlayerNameSpeculative;
}

	void CUIGameData::RunFrame_Invite()
	{
	}
	void CUIGameData::OnEvent( KeyValues *pEvent )
	{
		char const *szEvent = pEvent->GetName();

		if ( !Q_stricmp( "OnSysXUIEvent", szEvent ) )
		{
			m_bXUIOpen = !Q_stricmp( "opening", pEvent->GetString( "action", "" ) );
		}
		else if ( !Q_stricmp( "OnProfileUnavailable", szEvent ) )
		{
	#if defined( _DEMO ) && defined( _X360 )
			return;
	#endif
			// Activate game ui to see the dialog
			if ( !CBaseModPanel::GetSingleton().IsVisible() )
			{
				engine->ExecuteClientCmd( "gameui_activate" );
			}
			

		}
	}

}