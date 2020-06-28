//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Core Movie Maker UI API
//
//=============================================================================
#include "toolutils/basetoolsystem.h"
#include "toolutils/recentfilelist.h"
#include "toolutils/toolmenubar.h"
#include "toolutils/toolswitchmenubutton.h"
#include "toolutils/toolfilemenubutton.h"
#include "toolutils/toolmenubutton.h"
#include "vgui_controls/Menu.h"
#include "tier1/KeyValues.h"
#include "toolutils/enginetools_int.h"
#include "toolframework/ienginetool.h"
#include "vgui/IInput.h"
#include "vgui/KeyCode.h"
#include "vgui_controls/FileOpenDialog.h"
#include "filesystem.h"
#include "vgui/ilocalize.h"
#include "dme_controls/elementpropertiestree.h"
#include "tier0/icommandline.h"
#include "materialsystem/imaterialsystem.h"
#include "vguimatsurface/imatsystemsurface.h"
#include "tier3/tier3.h"
#include "tier2/fileutils.h"
#include "menus.h"

using namespace vgui;

#define LOG_FUNCTION_CALL Msg("%s called\n", __FUNCTION__)

const char *GetVGuiControlsModuleName()
{
	return "MapEdit";
}

//-----------------------------------------------------------------------------
// Connect, disconnect
//-----------------------------------------------------------------------------
bool ConnectTools( CreateInterfaceFn factory )
{
	return (materials != NULL) && (g_pMatSystemSurface != NULL);
}

void DisconnectTools( )
{
}


//-----------------------------------------------------------------------------
// Implementation of the sample tool
//-----------------------------------------------------------------------------
class CMapEditor : public CBaseToolSystem, public IFileMenuCallbacks
{
	DECLARE_CLASS_SIMPLE( CMapEditor, CBaseToolSystem );

public:
	CMapEditor();

	// Inherited from IToolSystem
	virtual const char *GetToolName() { return "Map Editor"; }
	virtual const char *GetBindingsContextFile() { return "cfg/MapEdit.kb"; }
	virtual bool	Init( );
    virtual void	Shutdown();

	// Inherited from IFileMenuCallbacks
	virtual int		GetFileMenuItemsEnabled( );
	virtual void	AddRecentFilesToMenu( vgui::Menu *menu );
	virtual bool	GetPerforceFileName( char *pFileName, int nMaxLen ) { return false; }
	virtual vgui::Panel* GetRootPanel() { return this; }

	// Inherited from CBaseToolSystem
	virtual vgui::HScheme GetToolScheme();
	virtual vgui::Menu *CreateActionMenu( vgui::Panel *pParent );
	virtual void OnCommand( const char *cmd );
	virtual const char *GetRegistryName() { return "MapEdit"; }
	virtual vgui::MenuBar *CreateMenuBar( CBaseToolSystem *pParent );

public:
	MESSAGE_FUNC( OnNew, "OnNew" );
	MESSAGE_FUNC( OnOpen, "OnOpen" );
	MESSAGE_FUNC( OnSave, "OnSave" );
	MESSAGE_FUNC( OnSaveAs, "OnSaveAs" );
	MESSAGE_FUNC( OnClose, "OnClose" );
	MESSAGE_FUNC( OnCloseNoSave, "OnCloseNoSave" );
	MESSAGE_FUNC( OnMarkNotDirty, "OnMarkNotDirty" );
	MESSAGE_FUNC( OnExit, "OnExit" );
	MESSAGE_FUNC(OnUndo, "OnUndo");
	MESSAGE_FUNC(OnRedo, "OnRedo");
	MESSAGE_FUNC(OnEditMapProperties, "OnEditMapProperties");
	MESSAGE_FUNC(OnShowGrid, "OnShowGrid");
	MESSAGE_FUNC(OnSnapToGrid, "OnSnapToGrid");
	MESSAGE_FUNC(OnShowMapInfo, "OnShowMapInfo");

	void		OpenFileFromHistory( int slot );
	virtual void SetupFileOpenDialog( vgui::FileOpenDialog *pDialog, bool bOpenFile, const char *pFileFormat, KeyValues *pContextKeyValues );
	virtual bool OnReadFileFromDisk( const char *pFileName, const char *pFileFormat, KeyValues *pContextKeyValues );
	virtual bool OnWriteFileToDisk( const char *pFileName, const char *pFileFormat, KeyValues *pContextKeyValues );
	virtual void OnFileOperationCompleted( const char *pFileType, bool bWroteFile, vgui::FileOpenStateMachine::CompletionState_t state, KeyValues *pContextKeyValues );

public:
	KeyValues* m_pMapKeyValues;

private:
	// Loads up a new document
	void LoadDocument( const char *pDocName );

	// Updates the menu bar based on the current file
	void UpdateMenuBar( );

	// Shows element properties
	void ShowElementProperties( );

	virtual const char *GetLogoTextureName();

};


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
CMapEditor	*g_pMapEditor = NULL;

void CreateTools()
{
	g_pMapEditor = new CMapEditor();
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CMapEditor::CMapEditor() :
	m_pMapKeyValues(nullptr)
{
}


//-----------------------------------------------------------------------------
// Init, shutdown
//-----------------------------------------------------------------------------
bool CMapEditor::Init( )
{
	m_RecentFiles.LoadFromRegistry( GetRegistryName() );

	// NOTE: This has to happen before BaseClass::Init
//	g_pVGuiLocalize->AddFile( "resource/toolsample_%language%.txt" );

	if ( !BaseClass::Init( ) )
		return false;

	return true;
}

void CMapEditor::Shutdown()
{
	m_RecentFiles.SaveToRegistry( GetRegistryName() );
	BaseClass::Shutdown();
}


//-----------------------------------------------------------------------------
// Derived classes can implement this to get a new scheme to be applied to this tool
//-----------------------------------------------------------------------------
vgui::HScheme CMapEditor::GetToolScheme() 
{ 
	return vgui::scheme()->LoadSchemeFromFile( "Resource/BoxRocket.res", "MapEditor" );
}


//-----------------------------------------------------------------------------
// Initializes the menu bar
//-----------------------------------------------------------------------------
vgui::MenuBar *CMapEditor::CreateMenuBar( CBaseToolSystem *pParent ) 
{
	CToolMenuBar *pMenuBar = new CToolMenuBar( pParent, "Main Menu Bar" );

	// Sets info in the menu bar
	char title[ 64 ];
	ComputeMenuBarTitle( title, sizeof( title ) );
	pMenuBar->SetInfo( title );
	pMenuBar->SetToolName( GetToolName() );

	// Add menu buttons
	CToolMenuButton *pFileButton = CreateToolFileMenuButton( pMenuBar, "File", "&File", GetActionTarget(), this );
	CToolMenuButton *pSwitchButton = CreateToolSwitchMenuButton( pMenuBar, "Switcher", "&Tools", GetActionTarget() );
	CToolMenuButton* pEditButton = new CEditMenu(pMenuBar, "Edit", "&Edit", GetActionTarget());
	CToolMenuButton* pMapButton = new CMapMenu(pMenuBar, "Map", "&Map", GetActionTarget());

	pMenuBar->AddButton(pFileButton);
	pMenuBar->AddButton(pEditButton);
	pMenuBar->AddButton(pMapButton);
	pMenuBar->AddButton(pSwitchButton);
	
	return pMenuBar;
}


//-----------------------------------------------------------------------------
// Creates the action menu
//-----------------------------------------------------------------------------
vgui::Menu *CMapEditor::CreateActionMenu( vgui::Panel *pParent )
{
	vgui::Menu *pActionMenu = new Menu( pParent, "ActionMenu" );
	pActionMenu->AddMenuItem( "#ToolHide", new KeyValues( "Command", "command", "HideActionMenu" ), GetActionTarget() );
	return pActionMenu;
}

//-----------------------------------------------------------------------------
// Inherited from IFileMenuCallbacks
//-----------------------------------------------------------------------------
int	CMapEditor::GetFileMenuItemsEnabled( )
{
	int nFlags = FILE_ALL;
	if ( m_RecentFiles.IsEmpty() )
	{
		nFlags &= ~(FILE_RECENT | FILE_CLEAR_RECENT);
	}
	return nFlags;
}

void CMapEditor::AddRecentFilesToMenu( vgui::Menu *pMenu )
{
	m_RecentFiles.AddToMenu( pMenu, GetActionTarget(), "OnRecent" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CMapEditor::OnExit()
{
	enginetools->Command( "quit\n" );
}

//-----------------------------------------------------------------------------
// Handle commands from the action menu and other menus
//-----------------------------------------------------------------------------
void CMapEditor::OnCommand( const char *cmd )
{
	if ( !V_stricmp( cmd, "HideActionMenu" ) )
	{
		if ( GetActionMenu() )
		{
			GetActionMenu()->SetVisible( false );
		}
	}
	else if ( const char *pSuffix = StringAfterPrefix( cmd, "OnRecent" ) )
	{
		int idx = Q_atoi( pSuffix );
		OpenFileFromHistory( idx );
	}
	else if( const char *pSuffix = StringAfterPrefix( cmd, "OnTool" ) )
	{
		int idx = Q_atoi( pSuffix );
		enginetools->SwitchToTool( idx );
	}
	else
	{
		BaseClass::OnCommand( cmd );
	}
}


//-----------------------------------------------------------------------------
// Command handlers
//-----------------------------------------------------------------------------

/* Called when File->New is called */
void CMapEditor::OnNew()
{
	// FIXME: Implement
	LOG_FUNCTION_CALL;

}

/* Called when the open-file dialog opens */
void CMapEditor::OnOpen()
{
	OpenFile( "txt" );
	LOG_FUNCTION_CALL;
}

bool CMapEditor::OnReadFileFromDisk( const char *pFileName, const char *pFileFormat, KeyValues *pContextKeyValues )
{
	Msg("Loading map %s...\n", pFileName);

	if (m_pMapKeyValues)
	{
		m_pMapKeyValues->deleteThis();
	}

	m_pMapKeyValues = new KeyValues(pFileName);

	m_pMapKeyValues->LoadFromFile(g_pFileSystem, pFileName);
	
	

	KeyValues* pVersionInfo = m_pMapKeyValues->FindKey("versioninfo");

	if (!pVersionInfo)
	{
		Warning("Failed to load map: versioninfo key not found\n");
		return false;
	}

	Msg("Map version: %u\nVMF version: %u\n", pVersionInfo->GetInt("mapversion"), pVersionInfo->GetInt("formatversion"));

	m_RecentFiles.Add( pFileName, pFileFormat );
	return true;
}

/* Called when File->Save is pressed */
void CMapEditor::OnSave()
{
	// FIXME: Implement
	LOG_FUNCTION_CALL;

}

/* Called when File->SaveAs is pressed */
void CMapEditor::OnSaveAs()
{
	LOG_FUNCTION_CALL;

	SaveFile( NULL, NULL, 0 );
}

bool CMapEditor::OnWriteFileToDisk( const char *pFileName, const char *pFileFormat, KeyValues *pContextKeyValues )
{
	// FIXME: Implement

	m_RecentFiles.Add( pFileName, pFileFormat );
	return true;
}

/* Called when File->Close is pressed */
void CMapEditor::OnClose()
{
	LOG_FUNCTION_CALL;

	// FIXME: Implement
}

/* Called when File->Close is pressed with a dirty function */
void CMapEditor::OnCloseNoSave()
{
	LOG_FUNCTION_CALL;

	// FIXME: Implement
}

void CMapEditor::OnMarkNotDirty()
{
	LOG_FUNCTION_CALL;

	// FIXME: Implement
}

void CMapEditor::OnUndo()
{
	LOG_FUNCTION_CALL;
}

void CMapEditor::OnRedo()
{
	LOG_FUNCTION_CALL;
}

void CMapEditor::OnEditMapProperties()
{
	LOG_FUNCTION_CALL;
}

void CMapEditor::OnSnapToGrid()
{

}

void CMapEditor::OnShowGrid()
{

}

void CMapEditor::OnShowMapInfo()
{

}


//-----------------------------------------------------------------------------
// Show the save document query dialog
//-----------------------------------------------------------------------------
void CMapEditor::OpenFileFromHistory( int slot )
{
	const char *pFileName = m_RecentFiles.GetFile( slot );
	OnReadFileFromDisk( pFileName, NULL, 0 );
}


//-----------------------------------------------------------------------------
// Called when file operations complete
//-----------------------------------------------------------------------------
void CMapEditor::OnFileOperationCompleted( const char *pFileType, bool bWroteFile, vgui::FileOpenStateMachine::CompletionState_t state, KeyValues *pContextKeyValues )
{
	// FIXME: Implement
}


//-----------------------------------------------------------------------------
// Show the File browser dialog
//-----------------------------------------------------------------------------
void CMapEditor::SetupFileOpenDialog( vgui::FileOpenDialog *pDialog, bool bOpenFile, const char *pFileFormat, KeyValues *pContextKeyValues )
{
	char pStartingDir[ MAX_PATH ];
	GetModSubdirectory( NULL, pStartingDir, sizeof(pStartingDir) );

	pDialog->SetTitle( "Choose map to edit", true );
	pDialog->SetStartDirectoryContext( "mapedit_session", pStartingDir );
	pDialog->AddFilter( "*.vmf", "Valve Map File (*.vmf)", true );
}

const char *CMapEditor::GetLogoTextureName()
{
	return "vgui/tools/sampletool/sampletool_logo";
}