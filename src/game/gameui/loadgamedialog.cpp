//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "loadgamedialog.h"
#include "engineinterface.h"

#include "vgui/isystem.h"
#include "vgui/isurface.h"
#include "vgui/ivgui.h"
#include "keyvalues.h"
#include "filesystem.h"

#include "vgui_controls/button.h"
#include "vgui_controls/panellistpanel.h"
#include "vgui_controls/querybox.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose:Constructor
//-----------------------------------------------------------------------------
CLoadGameDialog::CLoadGameDialog(vgui::Panel *parent) : BaseClass(parent, "LoadGameDialog")
{
	SetScheme(vgui::scheme()->LoadSchemeFromFileEx(0, "resource/sourcescheme.res", "SwarmScheme"));

	SetDeleteSelfOnClose(true);
	SetBounds(0, 0, 512, 384);
	SetMinimumSize( 256, 300 );
	SetSizeable( true );

	SetTitle("#GameUI_LoadGame", true);

	vgui::Button *cancel = new vgui::Button( this, "Cancel", "#GameUI_Cancel" );
	cancel->SetCommand( "Close" );

	LoadControlSettings("resource/LoadGameDialog.res");

	SetControlEnabled( "delete", false );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CLoadGameDialog::~CLoadGameDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadGameDialog::OnCommand( const char *command )
{
	if ( !stricmp( command, "loadsave" ) )
	{
		int saveIndex = GetSelectedItemSaveIndex();
		if ( m_SaveGames.IsValidIndex(saveIndex) )
		{
			const char *shortName = m_SaveGames[saveIndex].szShortName;
			if ( shortName && shortName[ 0 ] )
			{
				// Load the game, return to top and switch to engine
				char sz[ 256 ];
				Q_snprintf(sz, sizeof( sz ), "progress_enable\nload %s\n", shortName );
				
				engine->ClientCmd_Unrestricted( sz );
				
				// Close this dialog
				OnClose();
			}
		}
	}
	else if ( !stricmp( command, "Delete" ) )
	{
		int saveIndex = GetSelectedItemSaveIndex();
		if ( m_SaveGames.IsValidIndex(saveIndex) )
		{
			// confirm the deletion
			QueryBox *box = new QueryBox( "#GameUI_ConfirmDeleteSaveGame_Title", "#GameUI_ConfirmDeleteSaveGame_Info" );
			box->AddActionSignalTarget(this);
			box->SetOKButtonText("#GameUI_ConfirmDeleteSaveGame_OK");
			box->SetOKCommand(new KeyValues("Command", "command", "DeleteConfirmed"));
			box->DoModal();
		}
	}
	else if ( !stricmp( command, "DeleteConfirmed" ) )
	{
		int saveIndex = GetSelectedItemSaveIndex();
		if ( m_SaveGames.IsValidIndex(saveIndex) )
		{
			DeleteSaveGame( m_SaveGames[saveIndex].szFileName );

			// reset the list
			ScanSavedGames();
			m_pGameList->MoveScrollBarToTop();
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

