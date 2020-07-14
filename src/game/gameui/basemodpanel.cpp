#include "cbase.h"
#include "basemodpanel.h"
#include "./gameui/igameui.h"
#include "ienginevgui.h"
#include "engine/ienginesound.h"
#include "engineinterface.h"
#include "tier0/dbg.h"
#include "ixboxsystem.h"
#include "gameui_interface.h"
#include "game/client/igameclientexports.h"
#include "gameui/igameconsole.h"
#include "inputsystem/iinputsystem.h"
#include "filesystem.h"
#include "tier2/renderutils.h"
// #include "vgui_video_player.h"

// BaseModUI High-level windows
#include "vtransitionscreen.h"
//#include "VAchievements.h"
#include "vaddonassociation.h"
#include "vaddons.h"
#include "vattractscreen.h"
#include "shared/settings/vaudio.h"
#include "shared/settings/vaudiovideo.h"
//#include "VCloud.h"
//#include "VControllerOptions.h"
//#include "VControllerOptionsButtons.h"
//#include "VControllerOptionsSticks.h"
//#include "VDownloads.h"
//#include "VFoundGames.h"
#include "vflyoutmenu.h"
//#include "VFoundGroupGames.h"
//#include "vfoundpublicgames.h"
//#include "VGameLobby.h"
//#include "VGameOptions.h"
//#include "VGameSettings.h"
#include "vgenericconfirmation.h"
//#include "VGenericWaitScreen.h"
//#include "vgetlegacydata.h"
//#include "VInGameDifficultySelect.h"
#include "vingamemainmenu.h"
//#include "VInGameChapterSelect.h"
//#include "VInGameKickPlayerList.h"
#include "shared/settings/vkeyboardmouse.h"
#include "shared/settings/vkeyboard.h"
//#include "VVoteOptions.h"
#include "vloadingprogress.h"
#include "vmainmenu.h"
//#include "VMultiplayer.h"
#include "shared/settings/voptions.h"
//#include "VSignInDialog.h"
#include "vfooterpanel.h"
//#include "VPasswordEntry.h"
#include "shared/settings/vvideo.h"
//#include "VSteamCloudConfirmation.h"
#include "vcustomcampaigns.h"
//#include "vdownloadcampaign.h"
//#include "vjukebox.h"
//#include "vleaderboard.h"
#include "gameconsole.h"

#include "vgui/isystem.h"
#include "vgui/isurface.h"
#include "vgui/ilocalize.h"
#include "vgui_controls/animationcontroller.h"
#include "gameui_util.h"
#include "vguimatsurface/imatsystemsurface.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "tier0/icommandline.h"
#include "fmtstr.h"
#include "smartptr.h"
//#include "nb_header_footer.h"
#include "vgui_controls/controllermap.h"
#include "modinfo.h"
#include "vgui_controls/animationcontroller.h"
#include "vgui_controls/imagepanel.h"
#include "vgui_controls/label.h"
#include "vgui_controls/menu.h"
#include "vgui_controls/menuitem.h"
#include "vgui_controls/phandle.h"
#include "vgui_controls/messagebox.h"
#include "vgui_controls/querybox.h"
#include "vgui_controls/controllermap.h"
#include "vgui_controls/keyrepeat.h"
#include "vgui/iinput.h"
#include "vgui/ivgui.h"
#include "newgamedialog.h"
#include "bonusmapsdialog.h"
#include "loadgamedialog.h"
#include "savegamedialog.h"
#include "shared/settings_old/optionsdialog.h"
//#include "settings/voptionsmenu.h"

// UI defines. Include if you want to implement some of them [str]
// gotta have a better way to select this
// idk why, but preprocessor defs arnt being added so I can't select this
// im probably an idiot
#include "maplab/ui_defines.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace BaseModUI;
using namespace vgui;

//setup in GameUI_Interface.cpp
extern class IMatchSystem *matchsystem;
extern const char *COM_GetModDirectory( void );
extern class IGameConsole *IGameConsole();
static CBaseModPanel	*g_pBasePanel = NULL;

extern ISoundEmitterSystemBase *soundemitterbase;

bool g_bIsCreatingNewGameMenuForPreFetching = false;

//=============================================================================
CBaseModPanel* CBaseModPanel::m_CFactoryBasePanel = 0;

//-----------------------------------------------------------------------------
// Purpose: singleton accessor
//-----------------------------------------------------------------------------
CBaseModPanel *BaseModUI::BasePanel()
{
	return g_pBasePanel;
}

#ifndef _CERT

ConVar ui_gameui_debug( "ui_gameui_debug", "0" );

int UI_IsDebug()
{
	return (*(int *)(&ui_gameui_debug)) ? ui_gameui_debug.GetInt() : 0;
}
#endif


// Use for show demos to force the correct campaign poster
ConVar demo_campaign_name( "demo_campaign_name", "L4D2C5", FCVAR_DEVELOPMENTONLY, "Short name of campaign (i.e. L4D2C5), used to show correct poster in demo mode." );

ConVar ui_lobby_noresults_create_msg_time( "ui_lobby_noresults_create_msg_time", "2.5", FCVAR_DEVELOPMENTONLY );



CGameMenuItem::CGameMenuItem(vgui::Menu *parent, const char *name)  : BaseClass(parent, name, "GameMenuItem") 
{
	m_bRightAligned = false;
}

void CGameMenuItem::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// make fully transparent
	SetFgColor(GetSchemeColor("MainMenu.TextColor", pScheme));
	SetBgColor(Color(0, 0, 0, 0));
	SetDefaultColor(GetSchemeColor("MainMenu.TextColor", pScheme), Color(0, 0, 0, 0));
	SetArmedColor(GetSchemeColor("MainMenu.ArmedTextColor", pScheme), Color(0, 0, 0, 0));
	SetDepressedColor(GetSchemeColor("MainMenu.DepressedTextColor", pScheme), Color(0, 0, 0, 0));
	SetContentAlignment(Label::a_west);
	SetBorder(NULL);
	SetDefaultBorder(NULL);
	SetDepressedBorder(NULL);
	SetKeyFocusBorder(NULL);

	vgui::HFont hMainMenuFont = pScheme->GetFont( "MainMenuFont", IsProportional() );

	if ( hMainMenuFont )
	{
		SetFont( hMainMenuFont );
	}
	else
	{
		SetFont( pScheme->GetFont( "MenuLarge", IsProportional() ) );
	}
	SetTextInset(0, 0);
	SetArmedSound("UI/buttonrollover.wav");
	SetDepressedSound("UI/buttonclick.wav");
	SetReleasedSound("UI/buttonclickrelease.wav");
	SetButtonActivationType(Button::ACTIVATE_ONPRESSED);

	if (m_bRightAligned)
	{
		SetContentAlignment(Label::a_east);
	}
}

void CGameMenuItem::PaintBackground()
{
	if ( !GameUI().IsConsoleUI() )
	{
		BaseClass::PaintBackground();
	}
	else
	{
		if ( !IsArmed() || !IsVisible() || GetParent()->GetAlpha() < 32 )
			return;

		int wide, tall;
		GetSize( wide, tall );

		DrawBoxFade( 0, 0, wide, tall, GetButtonBgColor(), 1.0f, 255, 0, true );
		DrawBoxFade( 2, 2, wide - 4, tall - 4, Color( 0, 0, 0, 96 ), 1.0f, 255, 0, true );
	}
}

void CGameMenuItem::SetRightAlignedText(bool state)
{
	m_bRightAligned = state;
}

//-----------------------------------------------------------------------------
// Purpose: General purpose 1 of N menu
//-----------------------------------------------------------------------------
class CGameMenu : public vgui::Menu
{
	DECLARE_CLASS_SIMPLE( CGameMenu, vgui::Menu );

public:
	CGameMenu(vgui::Panel *parent, const char *name) : BaseClass(parent, name) 
	{
		if ( GameUI().IsConsoleUI() )
		{
			// shows graphic button hints
			m_pConsoleFooter = new CFooterPanel( parent, "MainMenuFooter" );

			int iFixedWidth = 245;

			SetFixedWidth( iFixedWidth );
		}
		else
		{
			m_pConsoleFooter = NULL;
		}
	}

	virtual void ApplySchemeSettings(IScheme *pScheme)
	{
		BaseClass::ApplySchemeSettings(pScheme);

		// make fully transparent
		SetMenuItemHeight(atoi(pScheme->GetResourceString("MainMenu.MenuItemHeight")));
		SetBgColor(Color(0, 0, 0, 0));
		SetBorder(NULL);
	}

	virtual void LayoutMenuBorder()
	{
	}

	virtual void SetVisible(bool state)
	{
		// force to be always visible
		BaseClass::SetVisible(true);
		// move us to the back instead of going invisible
		if (!state)
		{
			ipanel()->MoveToBack(GetVPanel());
		}
	}

	virtual int AddMenuItem(const char *itemName, const char *itemText, const char *command, Panel *target, KeyValues *userData = NULL)
	{
		MenuItem *item = new CGameMenuItem(this, itemName);
		item->AddActionSignalTarget(target);
		item->SetCommand(command);
		item->SetText(itemText);
		item->SetUserData(userData);
		return BaseClass::AddMenuItem(item);
	}

	virtual int AddMenuItem(const char *itemName, const char *itemText, KeyValues *command, Panel *target, KeyValues *userData = NULL)
	{
		CGameMenuItem *item = new CGameMenuItem(this, itemName);
		item->AddActionSignalTarget(target);
		item->SetCommand(command);
		item->SetText(itemText);
		item->SetRightAlignedText(true);
		item->SetUserData(userData);
		return BaseClass::AddMenuItem(item);
	}

	virtual void SetMenuItemBlinkingState( const char *itemName, bool state )
	{
		for (int i = 0; i < GetChildCount(); i++)
		{
			Panel *child = GetChild(i);
			MenuItem *menuItem = dynamic_cast<MenuItem *>(child);
			if (menuItem)
			{
				if ( Q_strcmp( menuItem->GetCommand()->GetString("command", ""), itemName ) == 0 )
				{
					menuItem->SetBlink( state );
				}
			}
		}
		InvalidateLayout();
	}

	virtual void OnCommand(const char *command)
	{
		m_KeyRepeat.Reset();

		if (!stricmp(command, "Open"))
		{
			MoveToFront();
			RequestFocus();
		}
		else
		{
			BaseClass::OnCommand(command);
		}
	}

	virtual void OnKeyCodePressed( KeyCode code )
	{
		if ( IsX360() )
		{
			if ( GetAlpha() != 255 )
			{
				SetEnabled( false );
				// inhibit key activity during transitions
				return;
			}

			SetEnabled( true );

			if ( code == KEY_XBUTTON_B || code == KEY_XBUTTON_START )
			{
				if ( GameUI().IsInLevel() )
				{
					GetParent()->OnCommand( "ResumeGame" );
				}
				return;
			}
		}

		m_KeyRepeat.KeyDown( code );

		BaseClass::OnKeyCodePressed( code );

		// HACK: Allow F key bindings to operate even here
		if ( IsPC() && code >= KEY_F1 && code <= KEY_F12 )
		{
			// See if there is a binding for the FKey
			const char *binding = gameuifuncs->GetBindingForButtonCode( code );
			if ( binding && binding[0] )
			{
				// submit the entry as a console commmand
				char szCommand[256];
				Q_strncpy( szCommand, binding, sizeof( szCommand ) );
				engine->ClientCmd_Unrestricted( szCommand );
			}
		}
	}

	void OnKeyCodeReleased( vgui::KeyCode code )
	{
		m_KeyRepeat.KeyUp( code );

		BaseClass::OnKeyCodeReleased( code );
	}

	void OnThink()
	{
		vgui::KeyCode code = m_KeyRepeat.KeyRepeated();
		if ( code )
		{
			OnKeyCodeTyped( code );
		}

		BaseClass::OnThink();
	}

	virtual void OnKillFocus()
	{
		BaseClass::OnKillFocus();

		// force us to the rear when we lose focus (so it looks like the menu is always on the background)
		surface()->MovePopupToBack(GetVPanel());

		m_KeyRepeat.Reset();
	}

	void ShowFooter( bool bShow )
	{
		if ( m_pConsoleFooter )
		{
			m_pConsoleFooter->SetVisible( bShow );
		}
	}

	void UpdateMenuItemState( bool isInGame, bool isMultiplayer )
	{
		bool isSteam = IsPC() && ( CommandLine()->FindParm("-steam") != 0 );
		bool bIsConsoleUI = GameUI().IsConsoleUI();

		// disabled save button if we're not in a game
		for (int i = 0; i < GetChildCount(); i++)
		{
			Panel *child = GetChild(i);
			MenuItem *menuItem = dynamic_cast<MenuItem *>(child);
			if (menuItem)
			{
				bool shouldBeVisible = true;
				// filter the visibility
				KeyValues *kv = menuItem->GetUserData();
				if (!kv)
					continue;

				if (!isInGame && kv->GetInt("OnlyInGame") )
				{
					shouldBeVisible = false;
				}
				else if (isMultiplayer && kv->GetInt("notmulti"))
				{
					shouldBeVisible = false;
				}
				else if (isInGame && !isMultiplayer && kv->GetInt("notsingle"))
				{
					shouldBeVisible = false;
				}
				else if (isSteam && kv->GetInt("notsteam"))
				{
					shouldBeVisible = false;
				}
				else if ( !bIsConsoleUI && kv->GetInt( "ConsoleOnly" ) )
				{
					shouldBeVisible = false;
				}

				menuItem->SetVisible( shouldBeVisible );
			}
		}

		if ( !isInGame )
		{
			// Sort them into their original order
			for ( int j = 0; j < GetChildCount() - 2; j++ )
			{
				MoveMenuItem( j, j + 1 );
			}
		}
		else
		{
			// Sort them into their in game order
			for ( int i = 0; i < GetChildCount(); i++ )
			{
				for ( int j = i; j < GetChildCount() - 2; j++ )
				{
					int iID1 = GetMenuID( j );
					int iID2 = GetMenuID( j + 1 );

					MenuItem *menuItem1 = GetMenuItem( iID1 );
					MenuItem *menuItem2 = GetMenuItem( iID2 );

					KeyValues *kv1 = menuItem1->GetUserData();
					KeyValues *kv2 = menuItem2->GetUserData();

					if ( kv1->GetInt("InGameOrder") > kv2->GetInt("InGameOrder") )
						MoveMenuItem( iID2, iID1 );
				}
			}
		}

		InvalidateLayout();

		if ( m_pConsoleFooter )
		{
			// update the console footer
			const char *pHelpName;
			if ( !isInGame )
				pHelpName = "MainMenu";
			else
				pHelpName = "GameMenu";

			if ( !m_pConsoleFooter->GetHelpName() || V_stricmp( pHelpName, m_pConsoleFooter->GetHelpName() ) )
			{
				// game menu must re-establish its own help once it becomes re-active
				m_pConsoleFooter->SetHelpNameAndReset( pHelpName );
				m_pConsoleFooter->AddNewButtonLabel( "#GameUI_Action", "#GameUI_Icons_A_BUTTON" );
				if ( isInGame )
				{
					m_pConsoleFooter->AddNewButtonLabel( "#GameUI_Close", "#GameUI_Icons_B_BUTTON" );
				}
			}
		}
	}

private:
	CFooterPanel *m_pConsoleFooter;
	vgui::CKeyRepeatHandler	m_KeyRepeat;
};

CBaseModPanel::CBaseModPanel(): BaseClass(0, "CBaseModPanel"),
	m_bClosingAllWindows( false ),
	m_lastActiveUserId( 0 )
{
	MakePopup( false );

	Assert(m_CFactoryBasePanel == 0);
	m_CFactoryBasePanel = this;

	g_pVGuiLocalize->AddFile( "resource/lang/%language%_l4d360ui.txt");
	g_pVGuiLocalize->AddFile( "resource/lang/%language%_gameui.txt"); // replace l4d360ui with this eventually?
	g_pVGuiLocalize->AddFile( "resource/lang/%language%_ep2.txt");
	g_pVGuiLocalize->AddFile( "resource/lang/%language%_maplab.txt");

	m_LevelLoading = false;
	
	for ( int k = 0; k < WPRI_COUNT; ++ k )
	{
		m_ActiveWindow[k] = WT_NONE;
	}

	// delay 3 frames before doing activation on initialization
	// needed to allow engine to exec startup commands (background map signal is 1 frame behind) 
	m_DelayActivation = 3;

	// maybe in the future add a menu for selecting scheme files, for themes and stuff maybe
	//m_UIScheme = vgui::scheme()->LoadSchemeFromFileEx( 0, "resource/scheme/gameuischeme.res", "GameUIScheme" );
	m_UIScheme = vgui::scheme()->LoadSchemeFromFileEx( 0, "resource/gameuischeme.res", "SwarmScheme" );
	//m_UIScheme = vgui::scheme()->LoadSchemeFromFileEx( 0, "resource/sourcescheme.res", "SwarmScheme" );

	SetScheme( m_UIScheme );

	// Only one user on the PC, so set it now
	SetLastActiveUserId( IsPC() ? 0 : -1 );

	// Precache critical font characters for the 360, dampens severity of these runtime i/o hitches
	IScheme *pScheme = vgui::scheme()->GetIScheme( m_UIScheme );
	m_hDefaultFont = pScheme->GetFont( "Default", true );
	vgui::surface()->PrecacheFontCharacters( m_hDefaultFont, NULL );
	vgui::surface()->PrecacheFontCharacters( pScheme->GetFont( "DefaultBold", true ), NULL );
	vgui::surface()->PrecacheFontCharacters( pScheme->GetFont( "DefaultLarge", true ), NULL );
	vgui::surface()->PrecacheFontCharacters( pScheme->GetFont( "FrameTitle", true ), NULL );

	m_FooterPanel = new CBaseModFooterPanel( this, "FooterPanel" );
	m_hOptionsDialog = NULL;

	m_bWarmRestartMode = false;
	m_ExitingFrameCount = 0;

	m_flBlurScale = 0;
	m_flLastBlurTime = 0;

	m_iBackgroundImageID = -1;
	//m_iProductImageID = -1;

	m_backgroundMusic = "MenuMusicSong";
	m_nBackgroundMusicGUID = 0;

	/*m_nProductImageWide = 0;
	m_nProductImageTall = 0;*/
	m_flMovieFadeInTime = 0.0f;
	m_pBackgroundMaterial = NULL;
	m_pBackgroundTexture = NULL;

	//Make it pausable When we reload, the game stops pausing on +esc
	ConVar *sv_pausable = cvar->FindVar( "sv_pausable" );
	sv_pausable->SetValue(1);
	
	m_pConsoleAnimationController = NULL;
	m_pConsoleControlSettings = NULL;
}

//=============================================================================
CBaseModPanel::~CBaseModPanel()
{
	// Game crashes on this
	ReleaseStartupGraphic();

	if ( m_FooterPanel )
	{
//		delete m_FooterPanel;
		m_FooterPanel->MarkForDeletion();
		m_FooterPanel = NULL;
	}	

	Assert(m_CFactoryBasePanel == this);
	m_CFactoryBasePanel = 0;

	surface()->DestroyTextureID( m_iBackgroundImageID );
	//surface()->DestroyTextureID( m_iProductImageID );

	// Shutdown UI game data
	CUIGameData::Shutdown();
}

//=============================================================================
CBaseModPanel& CBaseModPanel::GetSingleton()
{
	Assert(m_CFactoryBasePanel != 0);
	return *m_CFactoryBasePanel;
}

//=============================================================================
CBaseModPanel* CBaseModPanel::GetSingletonPtr()
{
	return m_CFactoryBasePanel;
}

//=============================================================================
void CBaseModPanel::ReloadScheme()
{

}

//=============================================================================
CBaseModFrame* CBaseModPanel::OpenWindow(const WINDOW_TYPE & wt, CBaseModFrame * caller, bool hidePrevious, KeyValues *pParameters)
{
	CBaseModFrame *newNav = m_Frames[ wt ].Get();
	bool setActiveWindow = true;

	// Window priority is used to track which windows are visible at all times
	// it is used to resolve the situations when a game requests an error box to popup
	// while a loading progress bar is active.
	// Windows with a higher priority force all other windows to get hidden.
	// After the high-priority window goes away it falls back to restore the low priority windows.
	WINDOW_PRIORITY nWindowPriority = WPRI_NORMAL;

	switch ( wt )
	{
	case WT_PASSWORDENTRY:
		setActiveWindow = false;
		break;
	}

	switch ( wt )
	{
	case WT_GENERICWAITSCREEN:
		nWindowPriority = WPRI_WAITSCREEN;
		break;
	case WT_GENERICCONFIRMATION:
		nWindowPriority = WPRI_MESSAGE;
		break;
	case WT_LOADINGPROGRESSBKGND:
		nWindowPriority = WPRI_BKGNDSCREEN;
		break;
	case WT_LOADINGPROGRESS:
		nWindowPriority = WPRI_LOADINGPLAQUE;
		break;
	case WT_PASSWORDENTRY:
		nWindowPriority = WPRI_TOPMOST;
		break;
	case WT_TRANSITIONSCREEN:
		nWindowPriority = WPRI_TOPMOST;
		break;
	}

	if ( !newNav )
	{
		switch ( wt )
		{
		case WT_ADDONS:
			m_Frames[wt] = new Addons( this, "Addons" );
			break;

		case WT_ADDONASSOCIATION:
			m_Frames[wt] = new AddonAssociation( this, "AddonAssociation" );
			break;

		case WT_ATTRACTSCREEN:
			m_Frames[ wt ] = new CAttractScreen( this, "AttractScreen" );
			break;

		case WT_AUDIO:
			m_Frames[wt] = new Audio(this, "Audio");
			break;

		case WT_AUDIOVIDEO:
			m_Frames[wt] = new AudioVideo(this, "AudioVideo");
			break;

		case WT_CUSTOMCAMPAIGNS:
			m_Frames[ wt ] = new CustomCampaigns( this, "CustomCampaigns" );
			break;

/*		case WT_GAMEOPTIONS:
			m_Frames[wt] = new GameOptions(this, "GameOptions");
			break;

		case WT_GAMESETTINGS:
			m_Frames[wt] = new GameSettings(this, "GameSettings");
			break;
*/
		case WT_GENERICCONFIRMATION:
			m_Frames[wt] = new GenericConfirmation(this, "GenericConfirmation");
			break;

		case WT_LOADINGPROGRESSBKGND:
			m_Frames[wt] = new LoadingProgress( this, "LoadingProgress", LoadingProgress::LWT_BKGNDSCREEN );
			break;

		case WT_LOADINGPROGRESS:
			m_Frames[wt] = new LoadingProgress( this, "LoadingProgress", LoadingProgress::LWT_LOADINGPLAQUE );
			break;

		case WT_INGAMEMAINMENU:
			m_Frames[wt] = new InGameMainMenu(this, "InGameMainMenu");
			//m_Frames[wt] = new MainMenu(this, "MainMenu");
			break;

		case WT_KEYBOARDMOUSE:
			m_Frames[wt] = new VKeyboard(this, "VKeyboard");
			break;

		case WT_MAINMENU:
			m_Frames[wt] = new MainMenu(this, "MainMenu");
			break;

		/*case WT_OPTIONS:
			m_Frames[wt] = new Options(this, "Options");
			break;*/
			
		case WT_TRANSITIONSCREEN:
			m_Frames[wt] = new CTransitionScreen( this, "TransitionScreen" );
			break;

		case WT_VIDEO:
			m_Frames[wt] = new Video(this, "Video");
			break;

		default:
			Assert( false );	// unknown window type
			break;
		}

		//
		// Finish setting up the window
		//

		newNav = m_Frames[wt].Get();
		if ( !newNav )
			return NULL;

		newNav->SetWindowPriority( nWindowPriority );
		newNav->SetWindowType(wt);
		newNav->SetVisible( false );
	}

	newNav->SetDataSettings( pParameters );

	if (setActiveWindow)
	{
		m_ActiveWindow[ nWindowPriority ] = wt;
		newNav->AddActionSignalTarget(this);
		newNav->SetCanBeActiveWindowType(true);
	}
	else if ( nWindowPriority == WPRI_MESSAGE )
	{
		m_ActiveWindow[ nWindowPriority ] = wt;
	}

	//
	// Now the window has been created, set it up
	//

	if ( UI_IsDebug() && (wt != WT_LOADINGPROGRESS) )
	{
		Msg( "[GAMEUI] OnOpen( `%s`, caller = `%s`, hidePrev = %d, setActive = %d, wt=%d, wpri=%d )\n",
			newNav->GetName(), caller ? caller->GetName() : "<NULL>", int(hidePrevious),
			int( setActiveWindow ), wt, nWindowPriority );
		KeyValuesDumpAsDevMsg( pParameters, 1 );
	}

	newNav->SetNavBack(caller);

	if (hidePrevious && caller != 0)
	{
		caller->SetVisible( false );
	}
	else if (caller != 0)
	{
		caller->FindAndSetActiveControl();
		//caller->SetAlpha(128);
	}

	// Check if a higher priority window is open
	if ( GetActiveWindowPriority() > newNav->GetWindowPriority() )
	{
		if ( UI_IsDebug() )
		{
			CBaseModFrame *pOther = m_Frames[ GetActiveWindowType() ].Get();
			Warning( "[GAMEUI] OpenWindow: Another window %p`%s` is having priority %d, deferring `%s`!\n",
				pOther, pOther ? pOther->GetName() : "<<null>>",
				GetActiveWindowPriority(), newNav->GetName() );
		}

		// There's a higher priority window that was open at the moment,
		// hide our window for now, it will get restored later.
		// newNav->SetVisible( false );
	}
	else
	{
		newNav->InvalidateLayout(false, false);
		newNav->OnOpen();
	}

	if ( UI_IsDebug() && (wt != WT_LOADINGPROGRESS) )
	{
		DbgShowCurrentUIState();
	}

	return newNav;
}

///=============================================================================
CBaseModFrame * CBaseModPanel::GetWindow( const WINDOW_TYPE& wt )
{
	return m_Frames[wt].Get();
}

//=============================================================================
WINDOW_TYPE CBaseModPanel::GetActiveWindowType()
{
	for ( int k = WPRI_COUNT; k -- > 0; )
	{
		if ( m_ActiveWindow[ k ] != WT_NONE )
		{
			CBaseModFrame *pFrame = m_Frames[ m_ActiveWindow[k] ].Get();
			if ( !pFrame || !pFrame->IsVisible() )
				continue;
			
			return m_ActiveWindow[ k ];
		}
	}
	return WT_NONE;
}

//=============================================================================
WINDOW_PRIORITY CBaseModPanel::GetActiveWindowPriority()
{
	for ( int k = WPRI_COUNT; k -- > 0; )
	{
		if ( m_ActiveWindow[ k ] != WT_NONE )
		{
			CBaseModFrame *pFrame = m_Frames[ m_ActiveWindow[k] ].Get();
			if ( !pFrame || !pFrame->IsVisible() )
				continue;

			return WINDOW_PRIORITY(k);
		}
	}
	return WPRI_NONE;
}

//=============================================================================
void CBaseModPanel::SetActiveWindow( CBaseModFrame * frame )
{
	if( !frame )
		return;
	
	m_ActiveWindow[ frame->GetWindowPriority() ] = frame->GetWindowType();

	if ( GetActiveWindowPriority() > frame->GetWindowPriority() )
	{
		if ( UI_IsDebug() )
		{
			CBaseModFrame *pOther = m_Frames[ GetActiveWindowType() ].Get();
			Warning( "[GAMEUI] SetActiveWindow: Another window %p`%s` is having priority %d, deferring `%s`!\n",
				pOther, pOther ? pOther->GetName() : "<<null>>",
				GetActiveWindowPriority(), frame->GetName() );
		}

		// frame->SetVisible( false );
	}
	else
	{
		frame->OnOpen();
	}
}

//=============================================================================
void CBaseModPanel::OnFrameClosed( WINDOW_PRIORITY pri, WINDOW_TYPE wt )
{
	if ( UI_IsDebug() )
	{
		Msg( "[GAMEUI] CBaseModPanel::OnFrameClosed( %d, %d )\n", pri, wt );
		DbgShowCurrentUIState();
	}

	// Mark the frame that just closed as NULL so that nobody could find it
	m_Frames[wt] = NULL;

	if ( m_bClosingAllWindows )
	{
		if ( UI_IsDebug() )
		{
			Msg( "[GAMEUI] Closing all windows\n" );
		}
		return;
	}

	if( pri <= WPRI_NORMAL )
		return;

	for ( int k = 0; k < WPRI_COUNT; ++ k )
	{
		if ( m_ActiveWindow[k] == wt )
			m_ActiveWindow[k] = WT_NONE;
	}

	//
	// We only care to resurrect windows of lower priority when
	// higher priority windows close
	//

	for ( int k = WPRI_COUNT; k -- > 0; )
	{
		if ( m_ActiveWindow[ k ] == WT_NONE )
			continue;

		CBaseModFrame *pFrame = m_Frames[ m_ActiveWindow[k] ].Get();
		if ( !pFrame )
			continue;

		// pFrame->AddActionSignalTarget(this);

		pFrame->InvalidateLayout(false, false);
		pFrame->OnOpen();
		pFrame->SetVisible( true );
		pFrame->Activate();

		if ( UI_IsDebug() )
		{
			Msg( "[GAMEUI] CBaseModPanel::OnFrameClosed( %d, %d ) -> Activated `%s`, pri=%d\n",
				pri, wt, pFrame->GetName(), pFrame->GetWindowPriority() );
			DbgShowCurrentUIState();
		}

		return;
	}
}

void CBaseModPanel::DbgShowCurrentUIState()
{
	if ( UI_IsDebug() < 2 )
		return;

	Msg( "[GAMEUI] Priorities WT: " );
	for ( int i = 0; i < WPRI_COUNT; ++ i )
	{
		Msg( " %d ", m_ActiveWindow[i] );
	}
	Msg( "\n" );
	for ( int i = 0; i < WT_WINDOW_COUNT; ++ i )
	{
		CBaseModFrame *pFrame = m_Frames[i].Get();
		if ( pFrame )
		{
			Msg( "        %2d. `%s` pri%d vis%d\n",
				i, pFrame->GetName(), pFrame->GetWindowPriority(), pFrame->IsVisible() );
		}
		else
		{
			Msg( "        %2d. NULL\n", i );
		}
	}
}

bool CBaseModPanel::IsLevelLoading()
{
	return m_LevelLoading;
}

//=============================================================================
void CBaseModPanel::CloseAllWindows( int ePolicyFlags )
{
	CAutoPushPop< bool > auto_m_bClosingAllWindows( m_bClosingAllWindows, true );

	if ( UI_IsDebug() )
	{
		Msg( "[GAMEUI] CBaseModPanel::CloseAllWindows( 0x%x )\n", ePolicyFlags );
	}

	// make sure we also close any active flyout menus that might be hanging out.
	FlyoutMenu::CloseActiveMenu();

	for (int i = 0; i < WT_WINDOW_COUNT; ++i)
	{
		CBaseModFrame *pFrame = m_Frames[i].Get();
		if ( !pFrame )
			continue;

		int nPriority = pFrame->GetWindowPriority();

		switch ( nPriority )
		{
		case WPRI_LOADINGPLAQUE:
			if ( !(ePolicyFlags & CLOSE_POLICY_EVEN_LOADING) )
			{
				if ( UI_IsDebug() )
				{
					Msg( "[GAMEUI] CBaseModPanel::CloseAllWindows() - Keeping loading type %d of priority %d.\n", i, nPriority );
				}

				continue;
				m_ActiveWindow[ WPRI_LOADINGPLAQUE ] = WT_NONE;
			}
			break;

		case WPRI_MESSAGE:
			if ( !(ePolicyFlags & CLOSE_POLICY_EVEN_MSGS) )
			{
				if ( UI_IsDebug() )
				{
					Msg( "[GAMEUI] CBaseModPanel::CloseAllWindows() - Keeping msgbox type %d of priority %d.\n", i, nPriority );
				}

				continue;
				m_ActiveWindow[ WPRI_MESSAGE ] = WT_NONE;
			}
			break;

		case WPRI_BKGNDSCREEN:
			if ( ePolicyFlags & CLOSE_POLICY_KEEP_BKGND )
			{
				if ( UI_IsDebug() )
				{
					Msg( "[GAMEUI] CBaseModPanel::CloseAllWindows() - Keeping bkgnd type %d of priority %d.\n", i, nPriority );
				}

				continue;
				m_ActiveWindow[ WPRI_BKGNDSCREEN ] = WT_NONE;
			}
			break;
		}

		// Close the window
		pFrame->Close();
		m_Frames[i] = NULL;
	}

	if ( UI_IsDebug() )
	{
		Msg( "[GAMEUI] After close all windows:\n" );
		DbgShowCurrentUIState();
	}

	m_ActiveWindow[ WPRI_NORMAL ] = WT_NONE;
}

#if defined( _X360 ) && defined( _DEMO )
void CBaseModPanel::OnDemoTimeout()
{
	if ( !engine->IsInGame() && !engine->IsConnected() && !engine->IsDrawingLoadingImage() )
	{
		// exit is terminal and unstoppable
		StartExitingProcess( false );
	}
	else
	{
		engine->ExecuteClientCmd( "disconnect" );
	}
}
#endif

bool CBaseModPanel::ActivateBackgroundEffects()
{
	// PC needs to keep start music, can't loop MP3's
	if ( IsPC() && !IsBackgroundMusicPlaying() )
	{
		StartBackgroundMusic( 1.0f );
	}

	return true;
}

#if defined( _X360 ) && defined( _DEMO )
void CBaseModPanel::OnDemoTimeout()
{
	if ( !engine->IsInGame() && !engine->IsConnected() && !engine->IsDrawingLoadingImage() )
	{
		// exit is terminal and unstoppable
		StartExitingProcess( false );
	}
	else
	{
		engine->ExecuteClientCmd( "disconnect" );
	}
}
#endif

//=============================================================================
void CBaseModPanel::OnGameUIActivated()
{
	if ( UI_IsDebug() )
	{
		Msg( "[GAMEUI] CBaseModPanel::OnGameUIActivated( delay = %d )\n", m_DelayActivation );
	}

	if ( m_DelayActivation )
	{
		return;
	}

	COM_TimestampedLog( "CBaseModPanel::OnGameUIActivated()" );

	SetVisible( true );

	// This is terrible, why are we directing the window that we open when we are only trying to activate the UI?
	if ( WT_GAMELOBBY == GetActiveWindowType() )
	{
		return;
	}
	else if ( !IsX360() && WT_LOADINGPROGRESS == GetActiveWindowType() )
	{
		// Ignore UI activations when loading poster is up
		return;
	}
	else if ( ( !m_LevelLoading && !engine->IsConnected() ) || GameUI().IsInBackgroundLevel() )
	{
		bool bForceReturnToFrontScreen = false;
		WINDOW_TYPE wt = GetActiveWindowType();
		switch ( wt )
		{
		default:
			break;
		case WT_NONE:
		case WT_MAINMENU: //case WT_INGAMEMAINMENU:
		case WT_GENERICCONFIRMATION:
			// bForceReturnToFrontScreen = !g_pMatchFramework->GetMatchmaking()->ShouldPreventOpenFrontScreen();
			bForceReturnToFrontScreen = true; // this used to be some magic about mid-disconnecting-states on PC...
			break;
		}
		if ( !IsPC() || bForceReturnToFrontScreen )
		{
			OpenFrontScreen();
		}
	}
	else if ( engine->IsConnected() && !m_LevelLoading )
	{
		//CBaseModFrame *pInGameMainMenu = m_Frames[ WT_INGAMEMAINMENU ].Get();
		CBaseModFrame *pInGameMainMenu = m_Frames[ WT_MAINMENU ].Get();

		if ( !pInGameMainMenu || !pInGameMainMenu->IsAutoDeleteSet() )
		{
			// Prevent in game menu from opening if it already exists!
			// It might be hiding behind a modal window that needs to keep focus
			//OpenWindow( WT_INGAMEMAINMENU, 0 );
			OpenWindow( WT_MAINMENU, 0 );
		}
	}
}

//=============================================================================
void CBaseModPanel::OnGameUIHidden()
{
	if ( UI_IsDebug() )
	{
		Msg( "[GAMEUI] CBaseModPanel::OnGameUIHidden()\n" );
	}

// 	// We want to check here if we have any pending message boxes and
// 	// if so, then we cannot just simply destroy all the UI elements
// 	for ( int k = WPRI_NORMAL + 1; k < WPRI_LOADINGPLAQUE; ++ k )
// 	{
// 		WINDOW_TYPE wt = m_ActiveWindow[k];
// 		if ( wt != WT_NONE )
// 		{
// 			Msg( "[GAMEUI] CBaseModPanel::OnGameUIHidden() - not destroying UI because of wt %d pri %d\n",
// 				wt, k );
// 			return;
// 		}
// 	}

	SetVisible(false);
	
	// Notify the options dialog that game UI is closing
	if ( m_hOptionsDialog.Get() )
	{
		PostMessage( m_hOptionsDialog.Get(), new KeyValues( "GameUIHidden" ) );
	}

	// Notify the in game menu that game UI is closing
	CBaseModFrame *pInGameMainMenu = GetWindow( WT_INGAMEMAINMENU );
	if ( pInGameMainMenu )
	{
		PostMessage( pInGameMainMenu, new KeyValues( "GameUIHidden" ) );
	}

	// Close achievements
	if ( CBaseModFrame *pFrame = GetWindow( WT_ACHIEVEMENTS ) )
	{
		pFrame->Close();
	}
}

void CBaseModPanel::OpenFrontScreen()
{
	WINDOW_TYPE frontWindow = WT_NONE;
#ifdef _X360
	// make sure we are in the startup menu.
	if ( !GameUI().IsInBackgroundLevel() )
	{
		engine->ClientCmd( "startupmenu" );
	}

	if ( g_pMatchFramework->GetMatchSession() )
	{
		Warning( "CBaseModPanel::OpenFrontScreen during active game ignored!\n" );
		return;
	}

	if( XBX_GetNumGameUsers() > 0 )
	{
		if ( CBaseModFrame *pAttractScreen = GetWindow( WT_ATTRACTSCREEN ) )
		{
			frontWindow = WT_ATTRACTSCREEN;
		}
		else
		{
			frontWindow = WT_MAINMENU;
		}
	}
	else
	{
		frontWindow = WT_ATTRACTSCREEN;
	}
#else
	frontWindow = WT_MAINMENU;
#endif // _X360

	if( frontWindow != WT_NONE )
	{
		if( GetActiveWindowType() != frontWindow )
		{
			CloseAllWindows();
			OpenWindow( frontWindow, NULL );
		}
	}
}

//=============================================================================
void CBaseModPanel::RunFrame()
{
	if ( s_NavLock > 0 )
	{
		--s_NavLock;
	}

	GetAnimationController()->UpdateAnimations( Plat_FloatTime() );

	CBaseModFrame::RunFrameOnListeners();

	CUIGameData::Get()->RunFrame();

	if ( m_DelayActivation )
	{
		m_DelayActivation--;
		if ( !m_LevelLoading && !m_DelayActivation )
		{
			if ( UI_IsDebug() )
			{
				Msg( "[GAMEUI] Executing delayed UI activation\n");
			}
			OnGameUIActivated();
		}
	}

	bool bDoBlur = true;
	WINDOW_TYPE wt = GetActiveWindowType();
	switch ( wt )
	{
	case WT_NONE:
	case WT_MAINMENU:
	case WT_LOADINGPROGRESSBKGND:
	case WT_LOADINGPROGRESS:
	case WT_AUDIOVIDEO:
		bDoBlur = false;
		break;
	}
	if ( GetWindow( WT_ATTRACTSCREEN ) || ( enginevguifuncs && !enginevguifuncs->IsGameUIVisible() ) )
	{
		// attract screen might be open, but not topmost due to notification dialogs
		bDoBlur = false;
	}

	if ( !bDoBlur )
	{
		bDoBlur = false;// GameClientExports()->ClientWantsBlurEffect();
	}

	float nowTime = Plat_FloatTime();
	float deltaTime = nowTime - m_flLastBlurTime;
	if ( deltaTime > 0 )
	{
		m_flLastBlurTime = nowTime;
		m_flBlurScale += deltaTime * bDoBlur ? 0.05f : -0.05f;
		m_flBlurScale = clamp( m_flBlurScale, 0, 0.85f );
		//engine->SetBlurFade( m_flBlurScale );
	}

	if ( IsX360() && m_ExitingFrameCount )
	{
		CTransitionScreen *pTransitionScreen = static_cast< CTransitionScreen* >( GetWindow( WT_TRANSITIONSCREEN ) );
		if ( pTransitionScreen && pTransitionScreen->IsTransitionComplete() )
		{
			if ( m_ExitingFrameCount > 1 )
			{
				m_ExitingFrameCount--;
				if ( m_ExitingFrameCount == 1 )
				{
					// enough frames have transpired, send the single shot quit command
					if ( m_bWarmRestartMode )
					{
						// restarts self, skips any intros
						engine->ClientCmd_Unrestricted( "quit_x360 restart\n" );
					}
					else
					{
						// cold restart, quits to any startup app
						engine->ClientCmd_Unrestricted( "quit_x360\n" );
					}
				}
			}
		}
	}
}

//=============================================================================
void CBaseModPanel::OnLevelLoadingStarted( char const *levelName, bool bShowProgressDialog )
{
	Assert( !m_LevelLoading );

	CloseAllWindows();

	if ( UI_IsDebug() )
	{
		Msg( "[GAMEUI] OnLevelLoadingStarted - opening loading progress (%s)...\n",
			levelName ? levelName : "<< no level specified >>" );
	}

	LoadingProgress *pLoadingProgress = static_cast<LoadingProgress*>( OpenWindow( WT_LOADINGPROGRESS, 0 ) );

	KeyValues *pMissionInfo = NULL;
	KeyValues *pChapterInfo = NULL;
	
	bool bShowPoster = false;
	char chGameMode[64] = {0};

	//
	// If we are just loading into some unknown map, then fake chapter information
	// (static lifetime of fake keyvalues so that we didn't worry about ownership)
	//
	if ( !pMissionInfo )
	{
		static KeyValues *s_pFakeMissionInfo = new KeyValues( "" );
		pMissionInfo = s_pFakeMissionInfo;
		pMissionInfo->SetString( "displaytitle", "#L4D360UI_Lobby_Unknown_Campaign" );
	}
	if ( !pChapterInfo )
	{
		static KeyValues *s_pFakeChapterInfo = new KeyValues( "1" );
		pChapterInfo = s_pFakeChapterInfo;
		pChapterInfo->SetString( "displayname", levelName ? levelName : "#L4D360UI_Lobby_Unknown_Campaign" );
		pChapterInfo->SetString( "map", levelName ? levelName : "" );
	}
	
	//
	// If we are transitioning maps from a real level then we don't want poster.
	// We always want the poster when loading the first chapter of a campaign (vote for restart)
	//
	bShowPoster = true; //( !GameUI().IsInLevel() ||
					//GameModeIsSingleChapter( chGameMode ) ||
					//( pChapterInfo && pChapterInfo->GetInt( "chapter" ) == 1 ) ) &&
		//pLoadingProgress->ShouldShowPosterForLevel( pMissionInfo, pChapterInfo );

	LoadingProgress::LoadingType type;
	if ( bShowPoster )
	{
		type = LoadingProgress::LT_POSTER;

		// These names match the order of the enum Avatar_t in imatchmaking.h

		const char *pPlayerNames[NUM_LOADING_CHARACTERS] = { NULL, NULL, NULL, NULL };
		//const char *pAvatarNames[NUM_LOADING_CHARACTERS] = { "", "", "", "" };

		unsigned char botFlags = 0xFF;

		pLoadingProgress->SetPosterData( pMissionInfo, pChapterInfo, pPlayerNames, botFlags, chGameMode );
	}
	else if ( GameUI().IsInLevel() && !GameUI().IsInBackgroundLevel() )
	{
		// Transitions between levels 
		type = LoadingProgress::LT_TRANSITION;
	}
	else
	{
		// Loading the menu the first time
		type = LoadingProgress::LT_MAINMENU;
	}

	pLoadingProgress->SetLoadingType( type );
	pLoadingProgress->SetProgress( 0.0f );

	m_LevelLoading = true;
}

void CBaseModPanel::OnEngineLevelLoadingSession( KeyValues *pEvent )
{
	if ( UI_IsDebug() )
	{
		Msg( "[GAMEUI] CBaseModPanel::OnEngineLevelLoadingSession\n");
	}

	// We must keep the default loading poster because it will be replaced by
	// the real campaign loading poster shortly
	float flProgress = 0.0f;
	if ( LoadingProgress *pLoadingProgress = static_cast<LoadingProgress*>( GetWindow( WT_LOADINGPROGRESS ) ) )
	{
		flProgress = pLoadingProgress->GetProgress();
		pLoadingProgress->Close();
		m_Frames[ WT_LOADINGPROGRESS ] = NULL;
	}
	CloseAllWindows( CLOSE_POLICY_DEFAULT );

	// Pop up a fake bkgnd poster
	if ( LoadingProgress *pLoadingProgress = static_cast<LoadingProgress*>( OpenWindow( WT_LOADINGPROGRESSBKGND, NULL ) ) )
	{
		pLoadingProgress->SetLoadingType( LoadingProgress::LT_POSTER );
		pLoadingProgress->SetProgress( flProgress );
	}
}

//=============================================================================
void CBaseModPanel::OnLevelLoadingFinished( KeyValues *kvEvent )
{
	int bError = kvEvent->GetInt( "error" );
	const char *failureReason = kvEvent->GetString( "reason" );
	
	Assert( m_LevelLoading );

	if ( UI_IsDebug() )
	{
		Msg( "[GAMEUI] CBaseModPanel::OnLevelLoadingFinished( %s, %s )\n", bError ? "Had Error" : "No Error", failureReason );
	}

	LoadingProgress *pLoadingProgress = static_cast<LoadingProgress*>( GetWindow( WT_LOADINGPROGRESS ) );
	if ( pLoadingProgress )
	{
		pLoadingProgress->SetProgress( 1.0f );

		// always close loading progress, this frees costly resources
		pLoadingProgress->Close();
	}

	m_LevelLoading = false;

	CBaseModFrame *pFrame = CBaseModPanel::GetSingleton().GetWindow( WT_GENERICCONFIRMATION );
	if ( !pFrame )
	{
		// no confirmation up, hide the UI
		GameUI().HideGameUI();
	}

	// if we are loading into the lobby, then skip the UIActivation code path
	// this can happen if we accepted an invite to player who is in the lobby while we were in-game
	if ( WT_GAMELOBBY != GetActiveWindowType() )
	{
		// if we are loading into the front-end, then activate the main menu (or attract screen, depending on state)
		// or if a message box is pending force open game ui
		if ( GameUI().IsInBackgroundLevel() || pFrame )
		{
			GameUI().OnGameUIActivated();
		}
	}

	if ( bError )
	{
		GenericConfirmation* pMsg = ( GenericConfirmation* ) OpenWindow( WT_GENERICCONFIRMATION, NULL, false );		
		if ( pMsg )
		{
			GenericConfirmation::Data_t data;
			data.pWindowTitle = "#L4D360UI_MsgBx_DisconnectedFromServer";			
			data.bOkButtonEnabled = true;
			data.pMessageText = failureReason;
			pMsg->SetUsageData( data );
		}		
	}
}

void CBaseModPanel::OnEvent( KeyValues *pEvent )
{
	char const *szEvent = pEvent->GetName();

	if ( !Q_stricmp( "OnEngineLevelLoadingSession", szEvent ) )
	{
		OnEngineLevelLoadingSession( pEvent );
	}
	else if ( !Q_stricmp( "OnEngineLevelLoadingFinished", szEvent ) )
	{
		OnLevelLoadingFinished( pEvent );
	}
}

//=============================================================================
bool CBaseModPanel::UpdateProgressBar( float progress, const char *statusText )
{
	if ( !m_LevelLoading )
	{
		// Assert( m_LevelLoading );
		// Warning( "WARN: CBaseModPanel::UpdateProgressBar called outside of level loading, discarded!\n" );
		return false;
	}


	LoadingProgress *loadingProgress = static_cast<LoadingProgress*>( OpenWindow( WT_LOADINGPROGRESS, 0 ) );

	// Even if the progress hasn't advanced, we want to go ahead and refresh if it has been more than 1/10 seconds since last refresh to keep the spinny thing going.
	static float s_LastEngineTime = -1.0f;
	// clock the anim at 10hz
	float time = Plat_FloatTime();
	float deltaTime = time - s_LastEngineTime;

	if ( loadingProgress && ( ( loadingProgress->IsDrawingProgressBar() && ( loadingProgress->GetProgress() < progress ) ) || ( deltaTime > 0.06f ) ) )
	{
		// update progress
		loadingProgress->SetProgress( progress );
		s_LastEngineTime = time;

		if ( UI_IsDebug() )
		{
			Msg( "[GAMEUI] [GAMEUI] CBaseModPanel::UpdateProgressBar(%.2f %s)\n", loadingProgress->GetProgress(), statusText );
		}
		return true;
	}

	// no update required
	return false;
}

void CBaseModPanel::SetHelpText( const char* helpText )
{
	if ( m_FooterPanel )
	{
		m_FooterPanel->SetHelpText( helpText );
	}
}

void CBaseModPanel::SetOkButtonEnabled( bool bEnabled )
{
	if ( m_FooterPanel )
	{
		CBaseModFooterPanel::FooterButtons_t buttons = m_FooterPanel->GetButtons();
		if ( bEnabled )
			buttons |= FB_ABUTTON;
		else
			buttons &= ~FB_ABUTTON;
		m_FooterPanel->SetButtons( buttons, m_FooterPanel->GetFormat(), m_FooterPanel->GetHelpTextEnabled() );
	}
}

void CBaseModPanel::SetCancelButtonEnabled( bool bEnabled )
{
	if ( m_FooterPanel )
	{
		CBaseModFooterPanel::FooterButtons_t buttons = m_FooterPanel->GetButtons();
		if ( bEnabled )
			buttons |= FB_BBUTTON;
		else
			buttons &= ~FB_BBUTTON;
		m_FooterPanel->SetButtons( buttons, m_FooterPanel->GetFormat(), m_FooterPanel->GetHelpTextEnabled() );
	}
}

BaseModUI::CBaseModFooterPanel* CBaseModPanel::GetFooterPanel()
{
	// EVIL HACK
	if ( !this )
	{
		Assert( 0 );
		Warning( "CBaseModPanel::GetFooterPanel() called on NULL CBaseModPanel!!!\n" );
		return NULL;
	}
	return m_FooterPanel;
}

void CBaseModPanel::SetLastActiveUserId( int userId )
{
	if ( m_lastActiveUserId != userId )
	{
		DevWarning( "SetLastActiveUserId: %d -> %d\n", m_lastActiveUserId, userId );
	}

	m_lastActiveUserId = userId;
}

int CBaseModPanel::GetLastActiveUserId( )
{
	return m_lastActiveUserId;
}

//-----------------------------------------------------------------------------
// Purpose: moves the game menu button to the right place on the taskbar
//-----------------------------------------------------------------------------
static void BaseUI_PositionDialog(vgui::PHandle dlg)
{
	if (!dlg.Get())
		return;

	int x, y, ww, wt, wide, tall;
	vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
	dlg->GetSize(wide, tall);

	// Center it, keeping requested size
	// ...why would you do that? just do that in the resource files
	//dlg->SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
}


//=============================================================================
void CBaseModPanel::OpenOptionsDialog( EditablePanel *parent )
{
	if ( IsPC() )
	{			
		if ( !m_hOptionsDialog.Get() )
		{
			m_hOptionsDialog = new COptionsDialog( parent );
			BaseUI_PositionDialog( m_hOptionsDialog );
		}

		m_hOptionsDialog->Activate();
	}
}

//=============================================================================
void CBaseModPanel::OpenOptionsMouseDialog( EditablePanel *parent )
{
	if ( IsPC() )
	{			
		if ( !m_hOptionsMouseDialog.Get() )
		{
			m_hOptionsMouseDialog = new COptionsMouseDialog( parent );
			BaseUI_PositionDialog( m_hOptionsMouseDialog );
		}

		m_hOptionsMouseDialog->Activate();
	}
}

//=============================================================================
void CBaseModPanel::OpenKeyBindingsDialog( EditablePanel *parent )
{
	if ( IsPC() )
	{			
		if ( !m_hOptionsDialog.Get() )
		{
			m_hOptionsDialog = new COptionsDialog( parent, OPTIONS_DIALOG_ONLY_BINDING_TABS );
			BaseUI_PositionDialog( m_hOptionsDialog );
		}

		m_hOptionsDialog->Activate();
	}
}

//=============================================================================
void CBaseModPanel::OnNavigateTo( const char* panelName )
{
	CBaseModFrame* currentFrame = 
		static_cast<CBaseModFrame*>(FindChildByName(panelName, false));

	if (currentFrame && currentFrame->GetCanBeActiveWindowType())
	{
		m_ActiveWindow[ currentFrame->GetWindowPriority() ] = currentFrame->GetWindowType();
	}
}

//=============================================================================
void CBaseModPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	//SetBgColor(pScheme->GetColor("Blank", Color(0, 0, 0, 0)));
	SetBgColor(pScheme->GetColor("ass", Color(64, 64, 96, 255)));

	int screenWide, screenTall;
	surface()->GetScreenSize( screenWide, screenTall );

	char filename[MAX_PATH];
	V_snprintf( filename, sizeof( filename ), "VGUI/loading/BGFX01" ); // TODO: engine->GetStartupImage( filename, sizeof( filename ), screenWide, screenTall );
	m_iBackgroundImageID = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_iBackgroundImageID, filename, true, false );

	/*m_iProductImageID = surface()->CreateNewTextureID();
	//surface()->DrawSetTextureFile( m_iProductImageID, "vgui/logo", true, false );
	surface()->DrawSetTextureFile( m_iProductImageID, "vgui/logo", true, true );

	// need these to be anchored now, can't come into existence during load
	PrecacheLoadingTipIcons();

	int logoW = 384;
	int logoH = 192;*/

	bool bIsWidescreen;

	float aspectRatio = (float)screenWide/(float)screenTall;
	bIsWidescreen = aspectRatio >= 1.5999f;
	/*if ( !bIsWidescreen )
	{
		// smaller in standard res
		logoW = 320;
		logoH = 160;
	}
	
	m_nProductImageX = vgui::scheme()->GetProportionalScaledValue( atoi( pScheme->GetResourceString( "Logo.X" ) ) );
	m_nProductImageY = vgui::scheme()->GetProportionalScaledValue( atoi( pScheme->GetResourceString( "Logo.Y" ) ) );
	m_nProductImageWide = vgui::scheme()->GetProportionalScaledValue( logoW );
	m_nProductImageTall = vgui::scheme()->GetProportionalScaledValue( logoH );*/

	if ( aspectRatio >= 1.6f )
	{
		// use the widescreen version
		Q_snprintf( m_szFadeFilename, sizeof( m_szFadeFilename ), "materials/console/%s_widescreen.vtf", "background01" );
	}
	else
	{
		Q_snprintf( m_szFadeFilename, sizeof( m_szFadeFilename ), "materials/console/%s_widescreen.vtf", "background01" );
	}

	// TODO: GetBackgroundMusic
#if 0

	bool bUseMono = false;

	char backgroundMusic[MAX_PATH];
	engine->GetBackgroundMusic( backgroundMusic, sizeof( backgroundMusic ), bUseMono );

	// the precache will be a memory or stream wave as needed 
	// on 360 the sound system will detect the install state and force it to a memory wave to finalize the the i/o now
	// it will be a stream resource if the installer is dormant
	// On PC it will be a streaming MP3
	if ( enginesound->PrecacheSound( backgroundMusic, true, false ) )
	{
		// successfully precached
		m_backgroundMusic = backgroundMusic;
	}
#endif
}

void CBaseModPanel::DrawColoredText( vgui::HFont hFont, int x, int y, unsigned int color, const char *pAnsiText )
{
	wchar_t szconverted[256];
	int len = g_pVGuiLocalize->ConvertANSIToUnicode( pAnsiText, szconverted, sizeof( szconverted ) );
	if ( len <= 0 )
	{
		return;
	}

	int r = ( color >> 24 ) & 0xFF;
	int g = ( color >> 16 ) & 0xFF;
	int b = ( color >> 8 ) & 0xFF;
	int a = ( color >> 0 ) & 0xFF;

	vgui::surface()->DrawSetTextFont( hFont );
	vgui::surface()->DrawSetTextPos( x, y );
	vgui::surface()->DrawSetTextColor( r, g, b, a );
	vgui::surface()->DrawPrintText( szconverted, len );
}

void CBaseModPanel::DrawCopyStats()
{
}

//=============================================================================
void CBaseModPanel::PaintBackground()
{
	if ( !m_LevelLoading &&
		!GameUI().IsInLevel() &&
		!GameUI().IsInBackgroundLevel() )
	{
		int wide, tall;
		GetSize( wide, tall );

		if ( false /*engine->IsTransitioningToLoad()*/ )
		{
			// ensure the background is clear
			// the loading progress is about to take over in a few frames
			// this keeps us from flashing a different graphic
			surface()->DrawSetColor( 0, 0, 0, 255 );
			surface()->DrawSetTexture( m_iBackgroundImageID );
			surface()->DrawTexturedRect( 0, 0, wide, tall );
		}
		else
		{
			ActivateBackgroundEffects();

			//if ( ASWBackgroundMovie() )
			//{
			//	ASWBackgroundMovie()->Update();

				/*if (ASWBackgroundMovie()->GetVideoMaterial())
				{
					// Draw the polys to draw this out
					CMatRenderContextPtr pRenderContext( materials );
	
					pRenderContext->MatrixMode( MATERIAL_VIEW );
					pRenderContext->PushMatrix();
					pRenderContext->LoadIdentity();

					pRenderContext->MatrixMode( MATERIAL_PROJECTION );
					pRenderContext->PushMatrix();
					pRenderContext->LoadIdentity();

					pRenderContext->Bind( ASWBackgroundMovie()->GetVideoMaterial()->GetMaterial(), NULL );

					CMeshBuilder meshBuilder;
					IMesh* pMesh = pRenderContext->GetDynamicMesh( true );
					meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

					int xpos = 0;
					int ypos = 0;
					vgui::ipanel()->GetAbsPos(GetVPanel(), xpos, ypos);

					float flLeftX = xpos;
					float flRightX = xpos + ( ASWBackgroundMovie()->m_nPlaybackWidth-1 );

					float flTopY = ypos;
					float flBottomY = ypos + ( ASWBackgroundMovie()->m_nPlaybackHeight-1 );

					// Map our UVs to cut out just the portion of the video we're interested in
					float flLeftU = 0.0f;
					float flTopV = 0.0f;

					// We need to subtract off a pixel to make sure we don't bleed
					float flRightU = ASWBackgroundMovie()->m_flU - ( 1.0f / (float) ASWBackgroundMovie()->m_nPlaybackWidth );
					float flBottomV = ASWBackgroundMovie()->m_flV - ( 1.0f / (float) ASWBackgroundMovie()->m_nPlaybackHeight );

					// Get the current viewport size
					int vx, vy, vw, vh;
					pRenderContext->GetViewport( vx, vy, vw, vh );

					// map from screen pixel coords to -1..1
					flRightX = FLerp( -1, 1, 0, vw, flRightX );
					flLeftX = FLerp( -1, 1, 0, vw, flLeftX );
					flTopY = FLerp( 1, -1, 0, vh ,flTopY );
					flBottomY = FLerp( 1, -1, 0, vh, flBottomY );

					float alpha = ((float)GetFgColor()[3]/255.0f);

					for ( int corner=0; corner<4; corner++ )
					{
						bool bLeft = (corner==0) || (corner==3);
						meshBuilder.Position3f( (bLeft) ? flLeftX : flRightX, (corner & 2) ? flBottomY : flTopY, 0.0f );
						meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
						meshBuilder.TexCoord2f( 0, (bLeft) ? flLeftU : flRightU, (corner & 2) ? flBottomV : flTopV );
						meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
						meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
						meshBuilder.Color4f( 1.0f, 1.0f, 1.0f, alpha );
						meshBuilder.AdvanceVertex();
					}
	
					meshBuilder.End();
					pMesh->Draw();

					pRenderContext->MatrixMode( MATERIAL_VIEW );
					pRenderContext->PopMatrix();

					pRenderContext->MatrixMode( MATERIAL_PROJECTION );
					pRenderContext->PopMatrix();
				}*/
				/*if ( ASWBackgroundMovie()->SetTextureMaterial() != -1 )
				{
					surface()->DrawSetColor( 255, 255, 255, 255 );
					int x, y, w, h;
					GetBounds( x, y, w, h );

					// center, 16:9 aspect ratio
					int width_at_ratio = h * (16.0f / 9.0f);
					x = ( w * 0.5f ) - ( width_at_ratio * 0.5f );

					surface()->DrawTexturedRect( x, y, x + width_at_ratio, y + h );

					if ( !m_flMovieFadeInTime )
					{
						// do the fade a little bit after the movie starts (needs to be stable)
						// the product overlay will fade out
						m_flMovieFadeInTime	= Plat_FloatTime() + TRANSITION_TO_MOVIE_DELAY_TIME;
					}

					float flFadeDelta = RemapValClamped( Plat_FloatTime(), m_flMovieFadeInTime, m_flMovieFadeInTime + TRANSITION_TO_MOVIE_FADE_TIME, 1.0f, 0.0f );
					if ( flFadeDelta > 0.0f )
					{
						if ( !m_pBackgroundMaterial )
						{
							PrepareStartupGraphic();
						}
						DrawStartupGraphic( flFadeDelta );
					}
				}*/
			//}
		}
	}
}

IVTFTexture *LoadVTF( CUtlBuffer &temp, const char *szFileName )
{
	if ( !g_pFullFileSystem->ReadFile( szFileName, NULL, temp ) )
		return NULL;

	IVTFTexture *texture = CreateVTFTexture();
	if ( !texture->Unserialize( temp ) )
	{
		Error( "Invalid or corrupt background texture %s\n", szFileName );
		return NULL;
	}
	texture->ConvertImageFormat( IMAGE_FORMAT_RGBA8888, false );
	return texture;
}

void CBaseModPanel::PrepareStartupGraphic()
{
	CUtlBuffer buf;
	// load in the background vtf
	buf.Clear();
	m_pBackgroundTexture = LoadVTF( buf, m_szFadeFilename );
	if ( !m_pBackgroundTexture )
	{
		Error( "Can't find background image '%s'\n", m_szFadeFilename );
		return;
	}

	// Allocate a white material
	m_pVMTKeyValues = new KeyValues( "UnlitGeneric" );
	m_pVMTKeyValues->SetString( "$basetexture", m_szFadeFilename + 10 );
	m_pVMTKeyValues->SetInt( "$ignorez", 1 );
	m_pVMTKeyValues->SetInt( "$nofog", 1 );
	m_pVMTKeyValues->SetInt( "$no_fullbright", 1 );
	m_pVMTKeyValues->SetInt( "$nocull", 1 );
	m_pVMTKeyValues->SetInt( "$vertexalpha", 1 );
	m_pVMTKeyValues->SetInt( "$vertexcolor", 1 );
	m_pBackgroundMaterial = g_pMaterialSystem->CreateMaterial( "__background", m_pVMTKeyValues );
}

void CBaseModPanel::ReleaseStartupGraphic()
{
	if ( m_pBackgroundMaterial )
	{
		m_pBackgroundMaterial->Release();
	}

	if ( m_pBackgroundTexture )
	{
		DestroyVTFTexture( m_pBackgroundTexture );
		m_pBackgroundTexture = NULL;
	}
}

// we have to draw the startup fade graphic using this function so it perfectly matches the one drawn by the engine during load
void DrawScreenSpaceRectangleAlpha( IMaterial *pMaterial, 
							  int nDestX, int nDestY, int nWidth, int nHeight,	// Rect to draw into in screen space
							  float flSrcTextureX0, float flSrcTextureY0,		// which texel you want to appear at destx/y
							  float flSrcTextureX1, float flSrcTextureY1,		// which texel you want to appear at destx+width-1, desty+height-1
							  int nSrcTextureWidth, int nSrcTextureHeight,		// needed for fixup
							  void *pClientRenderable,							// Used to pass to the bind proxies
							  int nXDice, int nYDice,							// Amount to tessellate the mesh
							  float fDepth, float flAlpha )									// what Z value to put in the verts (def 0.0)
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	if ( ( nWidth <= 0 ) || ( nHeight <= 0 ) )
		return;

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->Bind( pMaterial, pClientRenderable );

	int xSegments = MAX( nXDice, 1);
	int ySegments = MAX( nYDice, 1);

	CMeshBuilder meshBuilder;

	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, xSegments * ySegments );

	int nScreenWidth, nScreenHeight;
	pRenderContext->GetRenderTargetDimensions( nScreenWidth, nScreenHeight );
	float flLeftX = nDestX - 0.5f;
	float flRightX = nDestX + nWidth - 0.5f;

	float flTopY = nDestY - 0.5f;
	float flBottomY = nDestY + nHeight - 0.5f;

	float flSubrectWidth = flSrcTextureX1 - flSrcTextureX0;
	float flSubrectHeight = flSrcTextureY1 - flSrcTextureY0;

	float flTexelsPerPixelX = ( nWidth > 1 ) ? flSubrectWidth / ( nWidth - 1 ) : 0.0f;
	float flTexelsPerPixelY = ( nHeight > 1 ) ? flSubrectHeight / ( nHeight - 1 ) : 0.0f;

	float flLeftU = flSrcTextureX0 + 0.5f - ( 0.5f * flTexelsPerPixelX );
	float flRightU = flSrcTextureX1 + 0.5f + ( 0.5f * flTexelsPerPixelX );
	float flTopV = flSrcTextureY0 + 0.5f - ( 0.5f * flTexelsPerPixelY );
	float flBottomV = flSrcTextureY1 + 0.5f + ( 0.5f * flTexelsPerPixelY );

	float flOOTexWidth = 1.0f / nSrcTextureWidth;
	float flOOTexHeight = 1.0f / nSrcTextureHeight;
	flLeftU *= flOOTexWidth;
	flRightU *= flOOTexWidth;
	flTopV *= flOOTexHeight;
	flBottomV *= flOOTexHeight;

	// Get the current viewport size
	int vx, vy, vw, vh;
	pRenderContext->GetViewport( vx, vy, vw, vh );

	// map from screen pixel coords to -1..1
	flRightX = FLerp( -1, 1, 0, vw, flRightX );
	flLeftX = FLerp( -1, 1, 0, vw, flLeftX );
	flTopY = FLerp( 1, -1, 0, vh ,flTopY );
	flBottomY = FLerp( 1, -1, 0, vh, flBottomY );

	// Dice the quad up...
	if ( xSegments > 1 || ySegments > 1 )
	{
		// Screen height and width of a subrect
		float flWidth  = (flRightX - flLeftX) / (float) xSegments;
		float flHeight = (flTopY - flBottomY) / (float) ySegments;

		// UV height and width of a subrect
		float flUWidth  = (flRightU - flLeftU) / (float) xSegments;
		float flVHeight = (flBottomV - flTopV) / (float) ySegments;

		for ( int x=0; x < xSegments; x++ )
		{
			for ( int y=0; y < ySegments; y++ )
			{
				// Top left
				meshBuilder.Position3f( flLeftX   + (float) x * flWidth, flTopY - (float) y * flHeight, fDepth );
				meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
				meshBuilder.TexCoord2f( 0, flLeftU   + (float) x * flUWidth, flTopV + (float) y * flVHeight);
				meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
				meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
				meshBuilder.Color4ub( 255, 255, 255, 255.0f * flAlpha );
				meshBuilder.AdvanceVertex();

				// Top right (x+1)
				meshBuilder.Position3f( flLeftX   + (float) (x+1) * flWidth, flTopY - (float) y * flHeight, fDepth );
				meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
				meshBuilder.TexCoord2f( 0, flLeftU   + (float) (x+1) * flUWidth, flTopV + (float) y * flVHeight);
				meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
				meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
				meshBuilder.Color4ub( 255, 255, 255, 255.0f * flAlpha );
				meshBuilder.AdvanceVertex();

				// Bottom right (x+1), (y+1)
				meshBuilder.Position3f( flLeftX   + (float) (x+1) * flWidth, flTopY - (float) (y+1) * flHeight, fDepth );
				meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
				meshBuilder.TexCoord2f( 0, flLeftU   + (float) (x+1) * flUWidth, flTopV + (float)(y+1) * flVHeight);
				meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
				meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
				meshBuilder.Color4ub( 255, 255, 255, 255.0f * flAlpha );
				meshBuilder.AdvanceVertex();

				// Bottom left (y+1)
				meshBuilder.Position3f( flLeftX   + (float) x * flWidth, flTopY - (float) (y+1) * flHeight, fDepth );
				meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
				meshBuilder.TexCoord2f( 0, flLeftU   + (float) x * flUWidth, flTopV + (float)(y+1) * flVHeight);
				meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
				meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
				meshBuilder.Color4ub( 255, 255, 255, 255.0f * flAlpha );
				meshBuilder.AdvanceVertex();
			}
		}
	}
	else // just one quad
	{
		for ( int corner=0; corner<4; corner++ )
		{
			bool bLeft = (corner==0) || (corner==3);
			meshBuilder.Position3f( (bLeft) ? flLeftX : flRightX, (corner & 2) ? flBottomY : flTopY, fDepth );
			meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
			meshBuilder.TexCoord2f( 0, (bLeft) ? flLeftU : flRightU, (corner & 2) ? flBottomV : flTopV );
			meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
			meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
			meshBuilder.Color4ub( 255, 255, 255, 255.0f * flAlpha );
			meshBuilder.AdvanceVertex();
		}
	}

	meshBuilder.End();
	pMesh->Draw();

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}


void CBaseModPanel::DrawStartupGraphic( float flNormalizedAlpha )
{
	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	int w = GetWide();
	int h = GetTall();
	int tw = m_pBackgroundTexture->Width();
	int th = m_pBackgroundTexture->Height();

	float depth = 0.5f;
	int width_at_ratio = h * (16.0f / 9.0f);
	int x = ( w * 0.5f ) - ( width_at_ratio * 0.5f );
	DrawScreenSpaceRectangleAlpha( m_pBackgroundMaterial, x, 0, width_at_ratio, h, 8, 8, tw-8, th-8, tw, th, NULL,1,1,depth,flNormalizedAlpha );
}

void CBaseModPanel::OnCommand(const char *command)
{
	if ( !Q_stricmp( command, "QuitRestartNoConfirm" ) )
	{
		if ( IsX360() )
		{
			StartExitingProcess( false );
		}
	}
	/*
	else if ( !Q_stricmp( command, "RestartWithNewLanguage" ) )
	{
		if ( !IsX360() )
		{
			const char *pUpdatedAudioLanguage = Audio::GetUpdatedAudioLanguage();

			if ( pUpdatedAudioLanguage[ 0 ] != '\0' )
			{
				char szSteamURL[50];
				char szAppId[50];

				// hide everything while we quit
				SetVisible( false );
				vgui::surface()->RestrictPaintToSinglePanel( GetVPanel() );
				engine->ClientCmd_Unrestricted( "quit\n" );

				// Construct Steam URL. Pattern is steam://run/<appid>/<language>. (e.g. Ep1 In French ==> steam://run/380/french)
				Q_strcpy(szSteamURL, "steam://run/");
				itoa( engine->GetAppID(), szAppId, 10 );
				Q_strcat( szSteamURL, szAppId, sizeof( szSteamURL ) );
				Q_strcat( szSteamURL, "/", sizeof( szSteamURL ) );
				Q_strcat( szSteamURL, pUpdatedAudioLanguage, sizeof( szSteamURL ) );

				// Set Steam URL for re-launch in registry. Launcher will check this registry key and exec it in order to re-load the game in the proper language
				vgui::system()->SetRegistryString("HKEY_CURRENT_USER\\Software\\Valve\\Source\\Relaunch URL", szSteamURL );
			}
		}
	}
	*/
	else
	{
		BaseClass::OnCommand( command );
	}
}

bool CBaseModPanel::IsReadyToWriteConfig( void )
{
	// For cert we only want to write config files is it has been at least 3 seconds
#ifdef _X360
	static ConVarRef r_host_write_last_time( "host_write_last_time" );
	return ( Plat_FloatTime() > r_host_write_last_time.GetFloat() + 3.05f );
#endif
	return false;
}

const char *CBaseModPanel::GetUISoundName(  UISound_t UISound )
{
	switch ( UISound )
	{
	case UISOUND_BACK:
		return "UI/menu_back.wav";
	case UISOUND_ACCEPT:
		return "UI/menu_accept.wav";
	case UISOUND_INVALID:
		return "UI/menu_invalid.wav";
	case UISOUND_COUNTDOWN:
		return "UI/menu_countdown.wav";
	case UISOUND_FOCUS:
		return "UI/menu_focus.wav";
	case UISOUND_CLICK:
		return "UI/buttonclick.wav";
	case UISOUND_DENY:
		return "UI/menu_invalid.wav";
	}
	return NULL;
}

void CBaseModPanel::PlayUISound( UISound_t UISound )
{
	const char *pSound = GetUISoundName( UISound );
	if ( pSound )
	{
		vgui::surface()->PlaySound( pSound );
	}
}

//=============================================================================
// Start system shutdown. Cannot be stopped.
// A Restart is cold restart, plays the intro movie again.
//=============================================================================
void CBaseModPanel::StartExitingProcess( bool bWarmRestart )
{
	if ( !IsX360() )
	{
		// xbox only
		Assert( 0 );
		return;
	}

	if ( m_ExitingFrameCount )
	{
		// already fired
		return;
	}

	// cold restart or warm
	m_bWarmRestartMode = bWarmRestart;

	// the exiting screen will transition to obscure all the game and UI
	OpenWindow( WT_TRANSITIONSCREEN, 0, false );

	// must let a non trivial number of screen swaps occur to stabilize image
	// ui runs in a constrained state, while shutdown is occurring
	m_ExitingFrameCount = 15;

	// exiting cannot be stopped
	// do not allow any input to occur
	g_pInputSystem->DetachFromWindow();

	// start shutting down systems
	engine->StartXboxExitingProcess();
}

void CBaseModPanel::OnSetFocus()
{
	BaseClass::OnSetFocus();
	if ( IsPC() )
	{
		GameConsole().Hide();
	}
}

void CBaseModPanel::OnMovedPopupToFront()
{
	if ( IsPC() )
	{
		GameConsole().Hide();
	}
}

bool CBaseModPanel::IsBackgroundMusicPlaying()
{
	if ( m_backgroundMusic.IsEmpty() )
		return false;

	if ( m_nBackgroundMusicGUID == 0 )
		return false;
	
	return enginesound->IsSoundStillPlaying( m_nBackgroundMusicGUID );
}

// per Morasky
#define BACKGROUND_MUSIC_DUCK	0.15f

bool CBaseModPanel::StartBackgroundMusic( float fVol )
{
	if ( IsBackgroundMusicPlaying() )
		return true;
	
	if ( m_backgroundMusic.IsEmpty() )
		return false;

	// trying to exit, cannot start it
	if ( m_ExitingFrameCount )
		return false;
	
	CSoundParameters params;
	if ( !soundemitterbase->GetParametersForSound( m_backgroundMusic.Get(), params, GENDER_NONE ) )
		return false;

	enginesound->EmitAmbientSound( params.soundname, params.volume * fVol, params.pitch );
	m_nBackgroundMusicGUID = enginesound->GetGuidForLastSoundEmitted();
		
	return ( m_nBackgroundMusicGUID != 0 );
}
void CBaseModPanel::UpdateBackgroundMusicVolume( float fVol )
{
	if ( !IsBackgroundMusicPlaying() )
		return;

	// mixes too loud against soft ui sounds
	enginesound->SetVolumeByGuid( m_nBackgroundMusicGUID, BACKGROUND_MUSIC_DUCK * fVol );
}

void CBaseModPanel::ReleaseBackgroundMusic()
{
	if ( m_backgroundMusic.IsEmpty() )
		return;

	if ( m_nBackgroundMusicGUID == 0 )
		return;

	// need to stop the sound now, do not queue the stop
	// we must release the 2-5 MB held by this resource
	enginesound->StopSoundByGuid( m_nBackgroundMusicGUID );
#if defined( _X360 )
	// TODO: enginesound->UnloadSound( m_backgroundMusic );
#endif

	m_nBackgroundMusicGUID = 0;
}

void CBaseModPanel::SafeNavigateTo( Panel *pExpectedFrom, Panel *pDesiredTo, bool bAllowStealFocus )
{
	Panel *pOriginalFocus = ipanel()->GetPanel( GetCurrentKeyFocus(), GetModuleName() );
	bool bSomeoneElseHasFocus = pOriginalFocus && (pOriginalFocus != pExpectedFrom);
	bool bActuallyChangingFocus = (pExpectedFrom != pDesiredTo);
	bool bNeedToReturnKeyFocus = !bAllowStealFocus && bSomeoneElseHasFocus && bActuallyChangingFocus;

	pDesiredTo->NavigateTo();

	if ( bNeedToReturnKeyFocus )
	{
		pDesiredTo->NavigateFrom();
		pOriginalFocus->NavigateTo();
	}
}

//-----------------------------------------------------------------------------
// Purpose: runs an animation sequence, then calls a message mapped function
//			when the animation is complete. 
//-----------------------------------------------------------------------------
void CBaseModPanel::RunAnimationWithCallback(vgui::Panel *parent, const char *animName, KeyValues *msgFunc)
{
	return; //more stupid console stuff aaaaaa
}

//-----------------------------------------------------------------------------
// Purpose: runs an animation to close a dialog and cleans up after close
//-----------------------------------------------------------------------------
void CBaseModPanel::RunCloseAnimation(const char *animName)
{
	RunAnimationWithCallback( this, animName, new KeyValues( "FinishDialogClose" ) );
}

//-----------------------------------------------------------------------------
// Purpose: starts the game
//-----------------------------------------------------------------------------
void CBaseModPanel::FadeToBlackAndRunEngineCommand(const char *engineCommand)
{
	KeyValues *pKV = new KeyValues("RunEngineCommand", "command", engineCommand);

	// execute immediately, with no delay
	PostMessage(this, pKV, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Add an Xbox 360 message dialog to a dialog stack
//-----------------------------------------------------------------------------
void CBaseModPanel::ShowMessageDialog(const uint nType, vgui::Panel *pOwner)
{
	if (pOwner == NULL)
	{
		pOwner = this;
	}

	//m_MessageDialogHandler.ShowMessageDialog(nType, pOwner);
}

void CBaseModPanel::SetMenuItemBlinkingState(const char *itemName, bool state)
{
	for (int i = 0; i < GetChildCount(); i++)
	{
		Panel *child = GetChild(i);
		CGameMenu *pGameMenu = dynamic_cast<CGameMenu *>(child);
		if ( pGameMenu )
		{
			pGameMenu->SetMenuItemBlinkingState( itemName, state );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Xbox 360 - Get the console UI keyvalues to pass to LoadControlSettings()
//-----------------------------------------------------------------------------
KeyValues *CBaseModPanel::GetConsoleControlSettings(void)
{
	return m_pConsoleControlSettings;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModPanel::OnOpenNewGameDialog(const char *chapter)
{
	if ( !m_hNewGameDialog.Get() )
	{
		m_hNewGameDialog = new CNewGameDialog(this, false);
		PositionDialog( m_hNewGameDialog );
	}

	if ( chapter )
	{
		((CNewGameDialog *)m_hNewGameDialog.Get())->SetSelectedChapter(chapter);
	}

	((CNewGameDialog *)m_hNewGameDialog.Get())->SetCommentaryMode( false );
	m_hNewGameDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModPanel::OnOpenBonusMapsDialog(void)
{
	if ( !m_hBonusMapsDialog.Get() )
	{
		m_hBonusMapsDialog = new CBonusMapsDialog(this);
		PositionDialog( m_hBonusMapsDialog );
	}

	m_hBonusMapsDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModPanel::OnOpenLoadGameDialog()
{
	if ( !m_hLoadGameDialog.Get() )
	{
		m_hLoadGameDialog = new CLoadGameDialog(this);
		PositionDialog( m_hLoadGameDialog );
	}
	m_hLoadGameDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseModPanel::OnOpenSaveGameDialog()
{
	if ( !m_hSaveGameDialog.Get() )
	{
		m_hSaveGameDialog = new CSaveGameDialog(this);
		PositionDialog( m_hSaveGameDialog );
	}
	m_hSaveGameDialog->Activate();
}

//-----------------------------------------------------------------------------
// Purpose: moves the game menu button to the right place on the taskbar
//-----------------------------------------------------------------------------
void CBaseModPanel::PositionDialog(vgui::PHandle dlg)
{
	if (!dlg.Get())
		return;

	int x, y, ww, wt, wide, tall;
	vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
	dlg->GetSize(wide, tall);

	// Center it, keeping requested size
	dlg->SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
}

//-----------------------------------------------------------------------------
// Purpose: xbox UI panel that displays button icons and help text for all menus
//-----------------------------------------------------------------------------
CFooterPanel::CFooterPanel( Panel *parent, const char *panelName ) : BaseClass( parent, panelName ) 
{
	SetVisible( true );
	SetAlpha( 0 );
	m_pHelpName = NULL;

	m_pSizingLabel = new vgui::Label( this, "SizingLabel", "" );
	m_pSizingLabel->SetVisible( false );

	m_nButtonGap = 32;
	m_nButtonGapDefault = 32;
	m_ButtonPinRight = 100;
	m_FooterTall = 80;

	int wide, tall;
	surface()->GetScreenSize(wide, tall);

	if ( tall <= 480 )
	{
		m_FooterTall = 60;
	}

	m_ButtonOffsetFromTop = 0;
	m_ButtonSeparator = 4;
	m_TextAdjust = 0;

	m_bPaintBackground = false;
	m_bCenterHorizontal = false;

	m_szButtonFont[0] = '\0';
	m_szTextFont[0] = '\0';
	m_szFGColor[0] = '\0';
	m_szBGColor[0] = '\0';
}

CFooterPanel::~CFooterPanel()
{
	SetHelpNameAndReset( NULL );

	delete m_pSizingLabel;
}

//-----------------------------------------------------------------------------
// Purpose: apply scheme settings
//-----------------------------------------------------------------------------
void CFooterPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_hButtonFont = pScheme->GetFont( ( m_szButtonFont[0] != '\0' ) ? m_szButtonFont : "GameUIButtons" );
	m_hTextFont = pScheme->GetFont( ( m_szTextFont[0] != '\0' ) ? m_szTextFont : "MenuLarge" );

	SetFgColor( pScheme->GetColor( m_szFGColor, Color( 255, 255, 255, 255 ) ) );
	SetBgColor( pScheme->GetColor( m_szBGColor, Color( 0, 0, 0, 255 ) ) );

	int x, y, w, h;
	GetParent()->GetBounds( x, y, w, h );
	SetBounds( x, h - m_FooterTall, w, m_FooterTall );
}

//-----------------------------------------------------------------------------
// Purpose: apply settings
//-----------------------------------------------------------------------------
void CFooterPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	// gap between hints
	m_nButtonGap = inResourceData->GetInt( "buttongap", 32 );
	m_nButtonGapDefault = m_nButtonGap;
	m_ButtonPinRight = inResourceData->GetInt( "button_pin_right", 100 );
	m_FooterTall = inResourceData->GetInt( "tall", 80 );
	m_ButtonOffsetFromTop = inResourceData->GetInt( "buttonoffsety", 0 );
	m_ButtonSeparator = inResourceData->GetInt( "button_separator", 4 );
	m_TextAdjust = inResourceData->GetInt( "textadjust", 0 );

	m_bCenterHorizontal = ( inResourceData->GetInt( "center", 0 ) == 1 );
	m_bPaintBackground = ( inResourceData->GetInt( "paintbackground", 0 ) == 1 );

	// fonts for text and button
	Q_strncpy( m_szTextFont, inResourceData->GetString( "fonttext", "MenuLarge" ), sizeof( m_szTextFont ) );
	Q_strncpy( m_szButtonFont, inResourceData->GetString( "fontbutton", "GameUIButtons" ), sizeof( m_szButtonFont ) );

	// fg and bg colors
	Q_strncpy( m_szFGColor, inResourceData->GetString( "fgcolor", "White" ), sizeof( m_szFGColor ) );
	Q_strncpy( m_szBGColor, inResourceData->GetString( "bgcolor", "Black" ), sizeof( m_szBGColor ) );

	for ( KeyValues *pButton = inResourceData->GetFirstSubKey(); pButton != NULL; pButton = pButton->GetNextKey() )
	{
		const char *pName = pButton->GetName();

		if ( !Q_stricmp( pName, "button" ) )
		{
			// Add a button to the footer
			const char *pText = pButton->GetString( "text", "NULL" );
			const char *pIcon = pButton->GetString( "icon", "NULL" );
			AddNewButtonLabel( pText, pIcon );
		}
	}

	InvalidateLayout( false, true ); // force ApplySchemeSettings to run
}

//-----------------------------------------------------------------------------
// Purpose: adds button icons and help text to the footer panel when activating a menu
//-----------------------------------------------------------------------------
void CFooterPanel::AddButtonsFromMap( vgui::Frame *pMenu )
{
	SetHelpNameAndReset( pMenu->GetName() );

	CControllerMap *pMap = dynamic_cast<CControllerMap*>( pMenu->FindChildByName( "ControllerMap" ) );
	if ( pMap )
	{
		int buttonCt = pMap->NumButtons();
		for ( int i = 0; i < buttonCt; ++i )
		{
			const char *pText = pMap->GetBindingText( i );
			if ( pText )
			{
				AddNewButtonLabel( pText, pMap->GetBindingIcon( i ) );
			}
		}
	}
}

void CFooterPanel::SetStandardDialogButtons()
{
	SetHelpNameAndReset( "Dialog" );
	AddNewButtonLabel( "#GameUI_Action", "#GameUI_Icons_A_BUTTON" );
	AddNewButtonLabel( "#GameUI_Close", "#GameUI_Icons_B_BUTTON" );
}

//-----------------------------------------------------------------------------
// Purpose: Caller must tag the button layout. May support reserved names
// to provide stock help layouts trivially.
//-----------------------------------------------------------------------------
void CFooterPanel::SetHelpNameAndReset( const char *pName )
{
	if ( m_pHelpName )
	{
		free( m_pHelpName );
		m_pHelpName = NULL;
	}

	if ( pName )
	{
		m_pHelpName = strdup( pName );
	}

	ClearButtons();
}

//-----------------------------------------------------------------------------
// Purpose: Caller must tag the button layout
//-----------------------------------------------------------------------------
const char *CFooterPanel::GetHelpName()
{
	return m_pHelpName;
}

void CFooterPanel::ClearButtons( void )
{
	m_ButtonLabels.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: creates a new button label with icon and text
//-----------------------------------------------------------------------------
void CFooterPanel::AddNewButtonLabel( const char *text, const char *icon )
{
	ButtonLabel_t *button = new ButtonLabel_t;

	Q_strncpy( button->name, text, MAX_PATH );
	button->bVisible = true;

	// Button icons are a single character
	wchar_t *pIcon = g_pVGuiLocalize->Find( icon );
	if ( pIcon )
	{
		button->icon[0] = pIcon[0];
		button->icon[1] = '\0';
	}
	else
	{
		button->icon[0] = '\0';
	}

	// Set the help text
	wchar_t *pText = g_pVGuiLocalize->Find( text );
	if ( pText )
	{
		wcsncpy( button->text, pText, wcslen( pText ) + 1 );
	}
	else
	{
		button->text[0] = '\0';
	}

	m_ButtonLabels.AddToTail( button );
}

//-----------------------------------------------------------------------------
// Purpose: Shows/Hides a button label
//-----------------------------------------------------------------------------
void CFooterPanel::ShowButtonLabel( const char *name, bool show )
{
	for ( int i = 0; i < m_ButtonLabels.Count(); ++i )
	{
		if ( !Q_stricmp( m_ButtonLabels[ i ]->name, name ) )
		{
			m_ButtonLabels[ i ]->bVisible = show;
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Changes a button's text
//-----------------------------------------------------------------------------
void CFooterPanel::SetButtonText( const char *buttonName, const char *text )
{
	for ( int i = 0; i < m_ButtonLabels.Count(); ++i )
	{
		if ( !Q_stricmp( m_ButtonLabels[ i ]->name, buttonName ) )
		{
			wchar_t *wtext = g_pVGuiLocalize->Find( text );
			if ( text )
			{
				wcsncpy( m_ButtonLabels[ i ]->text, wtext, wcslen( wtext ) + 1 );
			}
			else
			{
				m_ButtonLabels[ i ]->text[ 0 ] = '\0';
			}
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Footer panel background rendering
//-----------------------------------------------------------------------------
void CFooterPanel::PaintBackground( void )
{
	if ( !m_bPaintBackground )
		return;

	BaseClass::PaintBackground();
}

//-----------------------------------------------------------------------------
// Purpose: Footer panel rendering
//-----------------------------------------------------------------------------
void CFooterPanel::Paint( void )
{
	// inset from right edge
	int wide = GetWide();
	int right = wide - m_ButtonPinRight;

	// center the text within the button
	int buttonHeight = vgui::surface()->GetFontTall( m_hButtonFont );
	int fontHeight = vgui::surface()->GetFontTall( m_hTextFont );
	int textY = ( buttonHeight - fontHeight )/2 + m_TextAdjust;

	if ( textY < 0 )
	{
		textY = 0;
	}

	int y = m_ButtonOffsetFromTop;

	if ( !m_bCenterHorizontal )
	{
		// draw the buttons, right to left
		int x = right;

		for ( int i = 0; i < m_ButtonLabels.Count(); ++i )
		{
			ButtonLabel_t *pButton = m_ButtonLabels[i];
			if ( !pButton->bVisible )
				continue;

			// Get the string length
			m_pSizingLabel->SetFont( m_hTextFont );
			m_pSizingLabel->SetText( pButton->text );
			m_pSizingLabel->SizeToContents();

			int iTextWidth = m_pSizingLabel->GetWide();

			if ( iTextWidth == 0 )
				x += m_nButtonGap;	// There's no text, so remove the gap between buttons
			else
				x -= iTextWidth;

			// Draw the string
			vgui::surface()->DrawSetTextFont( m_hTextFont );
			vgui::surface()->DrawSetTextColor( GetFgColor() );
			vgui::surface()->DrawSetTextPos( x, y + textY );
			vgui::surface()->DrawPrintText( pButton->text, wcslen( pButton->text ) );

			// Draw the button
			// back up button width and a little extra to leave a gap between button and text
			x -= ( vgui::surface()->GetCharacterWidth( m_hButtonFont, pButton->icon[0] ) + m_ButtonSeparator );
			vgui::surface()->DrawSetTextFont( m_hButtonFont );
			vgui::surface()->DrawSetTextColor( 255, 255, 255, 255 );
			vgui::surface()->DrawSetTextPos( x, y );
			vgui::surface()->DrawPrintText( pButton->icon, 1 );

			// back up to next string
			x -= m_nButtonGap;
		}
	}
	else
	{
		// center the buttons (as a group)
		int x = wide / 2;
		int totalWidth = 0;
		int i = 0;
		int nButtonCount = 0;

		// need to loop through and figure out how wide our buttons and text are (with gaps between) so we can offset from the center
		for ( i = 0; i < m_ButtonLabels.Count(); ++i )
		{
			ButtonLabel_t *pButton = m_ButtonLabels[i];
			if ( !pButton->bVisible )
				continue;

			// Get the string length
			m_pSizingLabel->SetFont( m_hTextFont );
			m_pSizingLabel->SetText( pButton->text );
			m_pSizingLabel->SizeToContents();

			totalWidth += vgui::surface()->GetCharacterWidth( m_hButtonFont, pButton->icon[0] );
			totalWidth += m_ButtonSeparator;
			totalWidth += m_pSizingLabel->GetWide();

			nButtonCount++; // keep track of how many active buttons we'll be drawing
		}

		totalWidth += ( nButtonCount - 1 ) * m_nButtonGap; // add in the gaps between the buttons
		x -= ( totalWidth / 2 );

		for ( i = 0; i < m_ButtonLabels.Count(); ++i )
		{
			ButtonLabel_t *pButton = m_ButtonLabels[i];
			if ( !pButton->bVisible )
				continue;

			// Get the string length
			m_pSizingLabel->SetFont( m_hTextFont );
			m_pSizingLabel->SetText( pButton->text );
			m_pSizingLabel->SizeToContents();

			int iTextWidth = m_pSizingLabel->GetWide();

			// Draw the icon
			vgui::surface()->DrawSetTextFont( m_hButtonFont );
			vgui::surface()->DrawSetTextColor( 255, 255, 255, 255 );
			vgui::surface()->DrawSetTextPos( x, y );
			vgui::surface()->DrawPrintText( pButton->icon, 1 );
			x += vgui::surface()->GetCharacterWidth( m_hButtonFont, pButton->icon[0] ) + m_ButtonSeparator;

			// Draw the string
			vgui::surface()->DrawSetTextFont( m_hTextFont );
			vgui::surface()->DrawSetTextColor( GetFgColor() );
			vgui::surface()->DrawSetTextPos( x, y + textY );
			vgui::surface()->DrawPrintText( pButton->text, wcslen( pButton->text ) );
			
			x += iTextWidth + m_nButtonGap;
		}
	}
}	

DECLARE_BUILD_FACTORY( CFooterPanel );

// X360TBD: Move into a separate module when completed
/*CMessageDialogHandler::CMessageDialogHandler()
{
	m_iDialogStackTop = -1;
}
void CMessageDialogHandler::ShowMessageDialog( int nType, vgui::Panel *pOwner )
{
	int iSimpleFrame = 0;
	if ( ModInfo().IsSinglePlayerOnly() )
	{
		iSimpleFrame = MD_SIMPLEFRAME;
	}

	switch( nType )
	{
	case MD_SEARCHING_FOR_GAMES:
		CreateMessageDialog( MD_CANCEL|MD_RESTRICTPAINT,
							NULL, 
							"#TF_Dlg_SearchingForGames", 
							NULL,
							"CancelOperation",
							pOwner,
							true ); 
		break;

	case MD_CREATING_GAME:
		CreateMessageDialog( MD_RESTRICTPAINT,
							NULL, 
							"#TF_Dlg_CreatingGame", 
							NULL,
							NULL,
							pOwner,
							true ); 
		break;

	case MD_SESSION_SEARCH_FAILED:
		CreateMessageDialog( MD_YESNO|MD_RESTRICTPAINT, 
							NULL, 
							"#TF_Dlg_NoGamesFound", 
							"ShowSessionOptionsDialog",
							"ReturnToMainMenu",
							pOwner ); 
		break;

	case MD_SESSION_CREATE_FAILED:
		CreateMessageDialog( MD_OK, 
							NULL, 
							"#TF_Dlg_CreateFailed", 
							"ReturnToMainMenu", 
							NULL,
							pOwner );
		break;

	case MD_SESSION_CONNECTING:
		CreateMessageDialog( 0, 
							NULL, 
							"#TF_Dlg_Connecting", 
							NULL, 
							NULL,
							pOwner,
							true );
		break;

	case MD_SESSION_CONNECT_NOTAVAILABLE:
		CreateMessageDialog( MD_OK, 
							NULL, 
							"#TF_Dlg_JoinRefused", 
							"ReturnToMainMenu", 
							NULL,
							pOwner );
		break;

	case MD_SESSION_CONNECT_SESSIONFULL:
		CreateMessageDialog( MD_OK, 
							NULL, 
							"#TF_Dlg_GameFull", 
							"ReturnToMainMenu", 
							NULL,
							pOwner );
		break;

	case MD_SESSION_CONNECT_FAILED:
		CreateMessageDialog( MD_OK, 
							NULL, 
							"#TF_Dlg_JoinFailed", 
							"ReturnToMainMenu", 
							NULL,
							pOwner );
		break;

	case MD_LOST_HOST:
		CreateMessageDialog( MD_OK|MD_RESTRICTPAINT, 
							NULL, 
							"#TF_Dlg_LostHost", 
							"ReturnToMainMenu", 
							NULL,
							pOwner );
		break;

	case MD_LOST_SERVER:
		CreateMessageDialog( MD_OK|MD_RESTRICTPAINT, 
							NULL, 
							"#TF_Dlg_LostServer", 
							"ReturnToMainMenu", 
							NULL,
							pOwner );
		break;

	case MD_MODIFYING_SESSION:
		CreateMessageDialog( MD_RESTRICTPAINT, 
							NULL, 
							"#TF_Dlg_ModifyingSession", 
							NULL, 
							NULL,
							pOwner,
							true );
		break;

	case MD_SAVE_BEFORE_QUIT:
		CreateMessageDialog( MD_YESNO|iSimpleFrame|MD_RESTRICTPAINT, 
							"#GameUI_QuitConfirmationTitle", 
							"#GameUI_Console_QuitWarning", 
							"QuitNoConfirm", 
							"CloseQuitDialog_OpenMainMenu",
							pOwner );
		break;

	case MD_QUIT_CONFIRMATION:
		CreateMessageDialog( MD_YESNO|iSimpleFrame|MD_RESTRICTPAINT, 
							 "#GameUI_QuitConfirmationTitle", 
							 "#GameUI_QuitConfirmationText", 
							 "QuitNoConfirm", 
							 "CloseQuitDialog_OpenMainMenu",
							 pOwner );
		break;

	case MD_QUIT_CONFIRMATION_TF:
		CreateMessageDialog( MD_YESNO|MD_RESTRICTPAINT, 
							 "#GameUI_QuitConfirmationTitle", 
							 "#GameUI_QuitConfirmationText", 
							 "QuitNoConfirm", 
							 "CloseQuitDialog_OpenMatchmakingMenu",
							 pOwner );
		break;

	case MD_DISCONNECT_CONFIRMATION:
		CreateMessageDialog( MD_YESNO|MD_RESTRICTPAINT, 
							"", 
							"#GameUI_DisconnectConfirmationText", 
							"DisconnectNoConfirm", 
							"close_dialog",
							pOwner );
		break;

	case MD_DISCONNECT_CONFIRMATION_HOST:
		CreateMessageDialog( MD_YESNO|MD_RESTRICTPAINT, 
							"", 
							"#GameUI_DisconnectHostConfirmationText", 
							"DisconnectNoConfirm", 
							"close_dialog",
							pOwner );
		break;

	case MD_KICK_CONFIRMATION:
		CreateMessageDialog( MD_YESNO, 
							"", 
							"#TF_Dlg_ConfirmKick", 
							"KickPlayer", 
							"close_dialog",
							pOwner );
		break;

	case MD_CLIENT_KICKED:
		CreateMessageDialog( MD_OK|MD_RESTRICTPAINT, 
							"", 
							"#TF_Dlg_ClientKicked", 
							"close_dialog", 
							NULL,
							pOwner );
		break;

	case MD_EXIT_SESSION_CONFIRMATION:
		CreateMessageDialog( MD_YESNO, 
							"", 
							"#TF_Dlg_ExitSessionText", 
							"ReturnToMainMenu", 
							"close_dialog",
							pOwner );
		break;

	case MD_STORAGE_DEVICES_NEEDED:
		CreateMessageDialog( MD_YESNO|MD_WARNING|MD_COMMANDAFTERCLOSE|iSimpleFrame|MD_RESTRICTPAINT, 
							 "#GameUI_Console_StorageRemovedTitle", 
							 "#GameUI_Console_StorageNeededBody", 
							 "ShowDeviceSelector", 
							 "QuitNoConfirm",
							 pOwner );
		break;

	case MD_STORAGE_DEVICES_CHANGED:
		CreateMessageDialog( MD_YESNO|MD_WARNING|MD_COMMANDAFTERCLOSE|iSimpleFrame|MD_RESTRICTPAINT, 
							"#GameUI_Console_StorageRemovedTitle", 
							"#GameUI_Console_StorageRemovedBody", 
							"ShowDeviceSelector", 
							"clear_storage_deviceID",
							pOwner );
		break;

	case MD_STORAGE_DEVICES_TOO_FULL:
		CreateMessageDialog( MD_YESNO|MD_WARNING|MD_COMMANDAFTERCLOSE|iSimpleFrame|MD_RESTRICTPAINT, 
							 "#GameUI_Console_StorageTooFullTitle", 
							 "#GameUI_Console_StorageTooFullBody", 
							 "ShowDeviceSelector", 
							 "StorageDeviceDenied",
							 pOwner );
		break;

	case MD_PROMPT_STORAGE_DEVICE:
		CreateMessageDialog( MD_YESNO|MD_WARNING|MD_COMMANDAFTERCLOSE|iSimpleFrame|MD_RESTRICTPAINT, 
							 "#GameUI_Console_NoStorageDeviceSelectedTitle", 
							 "#GameUI_Console_NoStorageDeviceSelectedBody", 
							 "ShowDeviceSelector", 
							 "StorageDeviceDenied",
							 pOwner );
		break;

	case MD_PROMPT_STORAGE_DEVICE_REQUIRED:
		CreateMessageDialog( MD_YESNO|MD_WARNING|MD_COMMANDAFTERCLOSE|MD_SIMPLEFRAME, 
							"#GameUI_Console_NoStorageDeviceSelectedTitle", 
							"#GameUI_Console_StorageDeviceRequiredBody", 
							"ShowDeviceSelector", 
							"RequiredStorageDenied",
							pOwner );
		break;

	case MD_PROMPT_SIGNIN:
		CreateMessageDialog( MD_YESNO|MD_WARNING|MD_COMMANDAFTERCLOSE|iSimpleFrame, 
							 "#GameUI_Console_NoUserProfileSelectedTitle", 
							 "#GameUI_Console_NoUserProfileSelectedBody", 
							 "ShowSignInUI", 
							 "SignInDenied",
							 pOwner );
		break;

	case MD_PROMPT_SIGNIN_REQUIRED:
		CreateMessageDialog( MD_YESNO|MD_WARNING|MD_COMMANDAFTERCLOSE|iSimpleFrame, 
							"#GameUI_Console_NoUserProfileSelectedTitle", 
							"#GameUI_Console_UserProfileRequiredBody", 
							"ShowSignInUI", 
							"RequiredSignInDenied",
							pOwner );
		break;

	case MD_NOT_ONLINE_ENABLED:
		CreateMessageDialog( MD_YESNO|MD_WARNING, 
							"", 
							"#TF_Dlg_NotOnlineEnabled", 
							"ShowSigninUI", 
							"close_dialog",
							pOwner );
		break;

	case MD_NOT_ONLINE_SIGNEDIN:
		CreateMessageDialog( MD_YESNO|MD_WARNING, 
							"", 
							"#TF_Dlg_NotOnlineSignedIn", 
							"ShowSigninUI", 
							"close_dialog",
							pOwner );
		break;

	case MD_DEFAULT_CONTROLS_CONFIRM:
		CreateMessageDialog( MD_YESNO|MD_WARNING|iSimpleFrame|MD_RESTRICTPAINT, 
							 "#GameUI_RestoreDefaults", 
							 "#GameUI_ControllerSettingsText", 
							 "DefaultControls", 
							 "close_dialog",
							 pOwner );
		break;

	case MD_AUTOSAVE_EXPLANATION:
		CreateMessageDialog( MD_OK|MD_WARNING|iSimpleFrame|MD_RESTRICTPAINT, 
							 "#GameUI_ConfirmNewGame_Title", 
							 "#GameUI_AutoSave_Console_Explanation", 
							 "StartNewGameNoCommentaryExplanation", 
							 NULL,
							 pOwner );
		break;

	case MD_COMMENTARY_EXPLANATION:
		CreateMessageDialog( MD_OK|MD_WARNING|iSimpleFrame|MD_RESTRICTPAINT, 
							 "#GameUI_CommentaryDialogTitle", 
							 "#GAMEUI_Commentary_Console_Explanation", 
							 "StartNewGameNoCommentaryExplanation", 
							 NULL,
							 pOwner );
		break;

	case MD_COMMENTARY_EXPLANATION_MULTI:
		CreateMessageDialog( MD_OK|MD_WARNING, 
							 "#GameUI_CommentaryDialogTitle", 
							 "#GAMEUI_Commentary_Console_Explanation", 
							 "StartNewGameNoCommentaryExplanation", 
							 NULL,
							 pOwner );
		break;

	case MD_COMMENTARY_CHAPTER_UNLOCK_EXPLANATION:
		CreateMessageDialog( MD_OK|MD_WARNING|iSimpleFrame|MD_RESTRICTPAINT, 
							 "#GameUI_CommentaryDialogTitle", 
							 "#GameUI_CommentaryUnlock", 
							 "close_dialog", 
							 NULL,
							 pOwner );
		break;
		
	case MD_SAVE_BEFORE_LANGUAGE_CHANGE:
		CreateMessageDialog( MD_YESNO|MD_WARNING|MD_SIMPLEFRAME|MD_COMMANDAFTERCLOSE|MD_RESTRICTPAINT, 
							 "#GameUI_ChangeLanguageRestart_Title", 
							 "#GameUI_ChangeLanguageRestart_Info", 
							 "AcceptVocalsLanguageChange", 
							 "CancelVocalsLanguageChange",
							 pOwner );

	case MD_SAVE_BEFORE_NEW_GAME:
		CreateMessageDialog( MD_OKCANCEL|MD_WARNING|iSimpleFrame|MD_COMMANDAFTERCLOSE|MD_RESTRICTPAINT, 
							 "#GameUI_ConfirmNewGame_Title", 
							 "#GameUI_NewGameWarning", 
							 "StartNewGame", 
							 "close_dialog",
							 pOwner );
		break;

	case MD_SAVE_BEFORE_LOAD:
		CreateMessageDialog( MD_OKCANCEL|MD_WARNING|iSimpleFrame|MD_COMMANDAFTERCLOSE|MD_RESTRICTPAINT, 
							 "#GameUI_ConfirmLoadGame_Title", 
							 "#GameUI_LoadWarning", 
							 "LoadGame", 
							 "LoadGameCancelled",
							 pOwner );
		break;

	case MD_DELETE_SAVE_CONFIRM:
		CreateMessageDialog( MD_OKCANCEL|MD_WARNING|iSimpleFrame|MD_COMMANDAFTERCLOSE, 
							 "#GameUI_ConfirmDeleteSaveGame_Title", 
							 "#GameUI_ConfirmDeleteSaveGame_Info", 
							 "DeleteGame", 
							 "DeleteGameCancelled",
							 pOwner );
		break;

	case MD_SAVE_OVERWRITE:
		CreateMessageDialog( MD_OKCANCEL|MD_WARNING|iSimpleFrame|MD_COMMANDAFTERCLOSE, 
							 "#GameUI_ConfirmOverwriteSaveGame_Title", 
							 "#GameUI_ConfirmOverwriteSaveGame_Info", 
							 "SaveGame", 
							 "OverwriteGameCancelled",
							 pOwner );
		break;

	case MD_SAVING_WARNING:
		CreateMessageDialog( MD_WARNING|iSimpleFrame|MD_COMMANDONFORCECLOSE, 
							 "",
							 "#GameUI_SavingWarning", 
							 "SaveSuccess", 
							 NULL,
							 pOwner,
							 true);
		break;

	case MD_SAVE_COMPLETE:
		CreateMessageDialog( MD_OK|iSimpleFrame|MD_COMMANDAFTERCLOSE, 
							 "#GameUI_ConfirmOverwriteSaveGame_Title", 
							 "#GameUI_GameSaved", 
							 "CloseAndSelectResume", 
							 NULL,
							 pOwner );
		break;

	case MD_LOAD_FAILED_WARNING:
		CreateMessageDialog( MD_OK |MD_WARNING|iSimpleFrame, 
			"#GameUI_LoadFailed", 
			"#GameUI_LoadFailed_Description", 
			"close_dialog", 
			NULL,
			pOwner );
		break;

	case MD_OPTION_CHANGE_FROM_X360_DASHBOARD:
		CreateMessageDialog( MD_OK|iSimpleFrame|MD_RESTRICTPAINT, 
							 "#GameUI_SettingChangeFromX360Dashboard_Title", 
							 "#GameUI_SettingChangeFromX360Dashboard_Info", 
							 "close_dialog", 
							 NULL,
							 pOwner );
		break;

	case MD_STANDARD_SAMPLE:
		CreateMessageDialog( MD_OK, 
							"Standard Dialog", 
							"This is a standard dialog", 
							"close_dialog", 
							NULL,
							pOwner );
		break;

	case MD_WARNING_SAMPLE:
		CreateMessageDialog( MD_OK | MD_WARNING,
							"#GameUI_Dialog_Warning", 
							"This is a warning dialog", 
							"close_dialog", 
							NULL,
							pOwner );
		break;

	case MD_ERROR_SAMPLE:
		CreateMessageDialog( MD_OK | MD_ERROR, 
							"Error Dialog", 
							"This is an error dialog", 
							"close_dialog", 
							NULL,
							pOwner );
		break;

	case MD_STORAGE_DEVICES_CORRUPT:
		CreateMessageDialog( MD_OK | MD_WARNING | iSimpleFrame | MD_RESTRICTPAINT,
			"", 
			"#GameUI_Console_FileCorrupt", 
			"close_dialog", 
			NULL,
			pOwner );
		break;

	case MD_CHECKING_STORAGE_DEVICE:
		CreateMessageDialog( iSimpleFrame | MD_RESTRICTPAINT,
			NULL, 
			"#GameUI_Dlg_CheckingStorageDevice",
			NULL,
			NULL,
			pOwner,
			true ); 
		break;

	default:
		break;
	}
}

ConVar vgui_message_dialog_modal("vgui_message_dialog_modal", "1", FCVAR_ARCHIVE);

void CMessageDialogHandler::CloseAllMessageDialogs()
{
	for ( int i = 0; i < MAX_MESSAGE_DIALOGS; ++i )
	{
		CMessageDialog *pDlg = m_hMessageDialogs[i];
		if ( pDlg )
		{
			vgui::surface()->RestrictPaintToSinglePanel(NULL);
			if ( vgui_message_dialog_modal.GetBool() )
			{
				vgui::input()->ReleaseAppModalSurface();
			}

			pDlg->Close();
			m_hMessageDialogs[i] = NULL;
		}
	}
}

void CMessageDialogHandler::CloseMessageDialog( const uint nType )
{
	int nStackIdx = 0;
	if ( nType & MD_WARNING )
	{
		nStackIdx = DIALOG_STACK_IDX_WARNING;
	}
	else if ( nType & MD_ERROR )
	{
		nStackIdx = DIALOG_STACK_IDX_ERROR;
	}

	CMessageDialog *pDlg = m_hMessageDialogs[nStackIdx];
	if ( pDlg )
	{
		vgui::surface()->RestrictPaintToSinglePanel(NULL);
		if ( vgui_message_dialog_modal.GetBool() )
		{
			vgui::input()->ReleaseAppModalSurface();
		}

		pDlg->Close();
		m_hMessageDialogs[nStackIdx] = NULL;
	}
}

void CMessageDialogHandler::CreateMessageDialog( const uint nType, const char *pTitle, const char *pMsg, const char *pCmdA, const char *pCmdB, vgui::Panel *pCreator, bool bShowActivity /*= false*/ /*)
{
	int nStackIdx = 0;
	if ( nType & MD_WARNING )
	{
		nStackIdx = DIALOG_STACK_IDX_WARNING;
	}
	else if ( nType & MD_ERROR )
	{
		nStackIdx = DIALOG_STACK_IDX_ERROR;
	}

	// Can only show one dialog of each type at a time
	if ( m_hMessageDialogs[nStackIdx].Get() )
	{
		Warning( "Tried to create two dialogs of type %d\n", nStackIdx );
		return;
	}

	// Show the new dialog
	m_hMessageDialogs[nStackIdx] = new CMessageDialog( BaseModUI::BasePanel(), nType, pTitle, pMsg, pCmdA, pCmdB, pCreator, bShowActivity );

	

	//m_hMessageDialogs[nStackIdx]->SetControlSettingsKeys( BasePanel()->GetConsoleControlSettings()->FindKey( "MessageDialog.res" ) );

	if ( nType & MD_RESTRICTPAINT )
	{
		vgui::surface()->RestrictPaintToSinglePanel( m_hMessageDialogs[nStackIdx]->GetVPanel() );
	}

	ActivateMessageDialog( nStackIdx );	
}

//-----------------------------------------------------------------------------
// Purpose: Activate a new message dialog
//-----------------------------------------------------------------------------
void CMessageDialogHandler::ActivateMessageDialog( int nStackIdx )
{
	int x, y, wide, tall;
	vgui::surface()->GetWorkspaceBounds( x, y, wide, tall );
	PositionDialog( m_hMessageDialogs[nStackIdx], wide, tall );

	uint nType = m_hMessageDialogs[nStackIdx]->GetType();
	if ( nType & MD_WARNING )
	{
		m_hMessageDialogs[nStackIdx]->SetZPos( 75 );
	}
	else if ( nType & MD_ERROR )
	{
		m_hMessageDialogs[nStackIdx]->SetZPos( 100 );
	}

	// Make sure the topmost item on the stack still has focus
	int idx = MAX_MESSAGE_DIALOGS - 1;
	for ( idx; idx >= nStackIdx; --idx )
	{
		CMessageDialog *pDialog = m_hMessageDialogs[idx];
		if ( pDialog )
		{
			pDialog->Activate();
			if ( vgui_message_dialog_modal.GetBool() )
			{
				vgui::input()->SetAppModalSurface( pDialog->GetVPanel() );
			}
			m_iDialogStackTop = idx;
			break;
		}
	}
}

void CMessageDialogHandler::PositionDialogs( int wide, int tall )
{
	for ( int i = 0; i < MAX_MESSAGE_DIALOGS; ++i )
	{
		if ( m_hMessageDialogs[i].Get() )
		{
			PositionDialog( m_hMessageDialogs[i], wide, tall );
		}
	}
}

void CMessageDialogHandler::PositionDialog( vgui::PHandle dlg, int wide, int tall )
{
	int w, t;
	dlg->GetSize(w, t);
	dlg->SetPos( (wide - w) / 2, (tall - t) / 2 );
}*/

static char *g_rgValidCommands[] =
{
	"OpenGameMenu",
	"OpenPlayerListDialog",
	"OpenNewGameDialog",
	"OpenLoadGameDialog",
	"OpenSaveGameDialog",
	"OpenCustomMapsDialog",
	//"OpenOptionsDialog",
	"OpenBenchmarkDialog",
	"OpenServerBrowser",
	"OpenFriendsDialog",
	"OpenLoadDemoDialog",
	"OpenCreateMultiplayerGameDialog",
	"OpenChangeGameDialog",
	"OpenLoadCommentaryDialog",
	"Quit",
	"QuitNoConfirm",
	"ResumeGame",
	"Disconnect",
};

static void CC_GameMenuCommand(const CCommand &args)
{
	int c = args.ArgC();
	if (c < 2)
	{
		Msg("Usage:  gamemenucommand <commandname>\n");
		return;
	}

	if (!g_pBasePanel)
	{
		return;
	}

	vgui::ivgui()->PostMessage(g_pBasePanel->GetVPanel(), new KeyValues("Command", "command", args[1]), NULL);
}
// This is defined in ulstring.h at the bottom in 2013 MP
/*
static bool UtlStringLessFunc(const CUtlString &lhs, const CUtlString &rhs)
{
	return Q_stricmp(lhs.String(), rhs.String()) < 0;
}*/
static int CC_GameMenuCompletionFunc(char const *partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	char const *cmdname = "gamemenucommand";

	char *substring = (char *)partial;
	if (Q_strstr(partial, cmdname))
	{
		substring = (char *)partial + strlen(cmdname) + 1;
	}

	int checklen = Q_strlen(substring);

	CUtlRBTree< CUtlString > symbols(0, 0, UtlStringLessFunc);

	int i;
	int c = ARRAYSIZE(g_rgValidCommands);
	for (i = 0; i < c; ++i)
	{
		if (Q_strnicmp(g_rgValidCommands[i], substring, checklen))
			continue;

		CUtlString str;
		str = g_rgValidCommands[i];

		symbols.Insert(str);

		// Too many
		if (symbols.Count() >= COMMAND_COMPLETION_MAXITEMS)
			break;
	}

	// Now fill in the results
	int slot = 0;
	for (i = symbols.FirstInorder(); i != symbols.InvalidIndex(); i = symbols.NextInorder(i))
	{
		char const *name = symbols[i].String();

		char buf[512];
		Q_strncpy(buf, name, sizeof(buf));
		Q_strlower(buf);

		Q_snprintf(commands[slot++], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s",
			cmdname, buf);
	}

	return slot;
}

static ConCommand gamemenucommand("gamemenucommand", CC_GameMenuCommand, "Issue game menu command.", 0, CC_GameMenuCompletionFunc);
