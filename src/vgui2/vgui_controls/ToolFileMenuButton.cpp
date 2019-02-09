//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Standard file menu
//
//=============================================================================

#include "toolutils/toolfilemenubutton.h"
#include "toolutils/toolmenubutton.h"
#include "tier1/keyvalues.h"
#include "tier1/utlstring.h"
#include "vgui_controls/menu.h"
#include "vgui_controls/frame.h"
#include "vgui_controls/button.h"
#include "vgui_controls/listpanel.h"
#include "toolutils/enginetools_int.h"
#include "p4lib/ip4.h"
#include "vgui_controls/perforcefilelistframe.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Global function to create the file menu
//-----------------------------------------------------------------------------
CToolMenuButton* CreateToolFileMenuButton( vgui::Panel *parent, const char *panelName, 
	const char *text, vgui::Panel *pActionTarget, IFileMenuCallbacks *pCallbacks )
{
	return new CToolFileMenuButton( parent, panelName, text, pActionTarget, pCallbacks );
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CToolFileMenuButton::CToolFileMenuButton( vgui::Panel *pParent, const char *panelName, const char *text, vgui::Panel *pActionSignalTarget, IFileMenuCallbacks *pFileMenuCallback ) :
	BaseClass( pParent, panelName, text, pActionSignalTarget ), m_pFileMenuCallback( pFileMenuCallback )
{
	Assert( pFileMenuCallback );

	AddMenuItem( "new", "#ToolFileNew", new KeyValues( "OnNew" ), pActionSignalTarget, NULL, "file_new" );
	AddMenuItem( "open", "#ToolFileOpen", new KeyValues( "OnOpen" ), pActionSignalTarget, NULL, "file_open"  );
	AddMenuItem( "save", "#ToolFileSave", new KeyValues( "OnSave" ), pActionSignalTarget, NULL, "file_save"  );
	AddMenuItem( "saveas", "#ToolFileSaveAs", new KeyValues( "OnSaveAs" ), pActionSignalTarget  );
	AddMenuItem( "close", "#ToolFileClose", new KeyValues( "OnClose" ), pActionSignalTarget  );
 	AddSeparator();

	m_pRecentFiles = new vgui::Menu( this, "RecentFiles" );
	m_nRecentFiles = m_pMenu->AddCascadingMenuItem( "#ToolFileRecent", pActionSignalTarget, m_pRecentFiles );

	AddMenuItem( "clearrecent", "#ToolFileClearRecent", new KeyValues ( "OnClearRecent" ), pActionSignalTarget );
	AddSeparator();
	AddMenuItem( "exit", "#ToolFileExit", new KeyValues ( "OnExit" ), pActionSignalTarget );

	SetMenu( m_pMenu );
}


//-----------------------------------------------------------------------------
// Gets called when the menu is shown
//-----------------------------------------------------------------------------
void CToolFileMenuButton::OnShowMenu( vgui::Menu *menu )
{
	BaseClass::OnShowMenu( menu );

	// Update the menu
	int nEnableMask = m_pFileMenuCallback->GetFileMenuItemsEnabled();

	int id = m_Items.Find( "new" );
	SetItemEnabled( id, (nEnableMask & IFileMenuCallbacks::FILE_NEW) != 0 );
	id = m_Items.Find( "open" );
	SetItemEnabled( id, (nEnableMask & IFileMenuCallbacks::FILE_OPEN) != 0 );
	id = m_Items.Find( "save" );
	SetItemEnabled( id, (nEnableMask & IFileMenuCallbacks::FILE_SAVE) != 0 );
	id = m_Items.Find( "saveas" );
	SetItemEnabled( id, (nEnableMask & IFileMenuCallbacks::FILE_SAVEAS) != 0 );
	id = m_Items.Find( "close" );
	SetItemEnabled( id, (nEnableMask & IFileMenuCallbacks::FILE_CLOSE) != 0 );
	id = m_Items.Find( "clearrecent" );
	SetItemEnabled( id, (nEnableMask & IFileMenuCallbacks::FILE_CLEAR_RECENT) != 0 );

	m_pRecentFiles->DeleteAllItems();

	if ( (nEnableMask & IFileMenuCallbacks::FILE_RECENT) == 0 )
	{
		m_pMenu->SetItemEnabled( m_nRecentFiles, false );
	}
	else
	{
		m_pMenu->SetItemEnabled( m_nRecentFiles, true );
		m_pFileMenuCallback->AddRecentFilesToMenu( m_pRecentFiles );
	}
}