//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "VMainMenu.h"
#include "EngineInterface.h"
#include "VFooterPanel.h"
#include "VHybridButton.h"
#include "VFlyoutMenu.h"
#include "vGenericConfirmation.h"
#include "VAddons.h"
//#include "VQuickJoin.h"
//#include "basemodpanel.h"
//#include "UIGameData.h"
//#include "VGameSettings.h"
//#include "VSteamCloudConfirmation.h"
#include "vaddonassociation.h"

//#include "VSignInDialog.h"
#include "VGuiSystemModuleLoader.h"
//#include "VAttractScreen.h"
#include "gamemodes.h"
#include "FileSystem.h"

#include "vgui/ILocalize.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/Tooltip.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/Image.h"

#include "steam/isteamremotestorage.h"
#include "materialsystem/materialsystem_config.h"

#include "ienginevgui.h"
#include "basepanel.h"
#include "vgui/ISurface.h"
#include "tier0/icommandline.h"
#include "fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;

//=============================================================================
static ConVar connect_lobby( "connect_lobby", "", FCVAR_HIDDEN, "Sets the lobby ID to connect to on start." );
static ConVar ui_old_options_menu( "ui_old_options_menu", "0", FCVAR_NONE, "Brings up the old tabbed options dialog from Keyboard/Mouse when set to 1." );
static ConVar ui_play_online_browser( "ui_play_online_browser",
#if defined( _DEMO )
									 "0",
									 FCVAR_NONE,
#else
									 "1",
									 FCVAR_NONE,
#endif
									 "Whether play online displays a browser or plain search dialog." );

void Demo_DisableButton( Button *pButton );
//void OpenGammaDialog( VPANEL parent );


void GetPrimaryModDirectoryForMenu( char *pcModPath, int nSize )
{
	g_pFullFileSystem->GetSearchPath( "MOD", false, pcModPath, nSize );

	// It's possible that we have multiple MOD directories if there is DLC installed. If that's the case get the last one
	// in the semi-colon delimited list
	char *pSemi = V_strrchr( pcModPath, ';');
	if ( pSemi )
	{
		V_strncpy( pcModPath, ++pSemi, MAX_PATH );
	}
}

//=============================================================================
MainMenu::MainMenu( Panel *parent, const char *panelName ):
	BaseClass( parent, panelName, true, true, false, false )
{
	SetProportional( true );
	SetTitle( "", false );
	SetMoveable( false );
	SetSizeable( false );

	SetLowerGarnishEnabled( true );

	// ?
	AddFrameListener( this );

	m_iQuickJoinHelpText = MMQJHT_NONE;

	SetDeleteSelfOnClose( true );

	SetCloseButtonVisible( false );

	// what is this
	SetConsoleStylePanel( true );

	//vgui::Button SetUseCaptureMouse( true );
}

//=============================================================================
MainMenu::~MainMenu()
{
	RemoveFrameListener( this );
}

const char *GetRandomQuitString()
{
	const char *string;

	int randomizer = RandomInt(0, 32);

	switch (randomizer)
	{
	default:  string = "#QuitMessage0"; break;
	case 0: string = "#QuitMessage0"; break;
	case 1: string = "#QuitMessage1"; break;
	case 2: string = "#QuitMessage2"; break;
	case 3: string = "#QuitMessage3"; break;
	case 4: string = "#QuitMessage4"; break;
	case 5: string = "#QuitMessage5"; break;
	case 6: string = "#QuitMessage6"; break;
	case 7: string = "#QuitMessage7"; break;
	case 8: string = "#QuitMessage8"; break;
	case 9: string = "#QuitMessage9"; break;
	case 10: string = "#QuitMessage10"; break;
	case 11: string = "#QuitMessage11"; break;
	case 12: string = "#QuitMessage12"; break;
	case 13: string = "#QuitMessage13"; break;
	case 14: string = "#QuitMessage14"; break;
	case 15: string = "#QuitMessage15"; break;
	case 16: string = "#QuitMessage16"; break;
	case 17: string = "#QuitMessage17"; break;
	case 18: string = "#QuitMessage18"; break;
	case 19: string = "#QuitMessage19"; break;
	case 20: string = "#QuitMessage20"; break;
	case 21: string = "#QuitMessage21"; break;
	case 22: string = "#QuitMessage22"; break;
	case 23: string = "#QuitMessage23"; break;
	case 24: string = "#QuitMessage24"; break;
	case 25: string = "#QuitMessage25"; break;
	case 26: string = "#QuitMessage26"; break;
	case 27: string = "#QuitMessage27"; break;
	case 28: string = "#QuitMessage28"; break;
	case 29: string = "#QuitMessage29"; break;
	case 30: string = "#QuitMessage30"; break;
	case 31: string = "#QuitMessage31"; break;
	case 32: string = "#QuitMessage32"; break;
	}

	return string;
}

//=============================================================================
static void LeaveGameOkCallback()
{
	COM_TimestampedLog( "Exit Game" );

	MainMenu* self = 
		static_cast< MainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_MAINMENU ) );

	if ( self )
	{
		self->Close();
	}

	engine->ExecuteClientCmd( "gameui_hide" );

	// On PC people can be playing via console bypassing matchmaking
	// and required session settings, so to leave game duplicate
	// session closure with an extra "disconnect" command.
	engine->ExecuteClientCmd( "disconnect" );

	//Turn off commentary.
	engine->ExecuteClientCmd( "commentary 0" );

	engine->ExecuteClientCmd( "gameui_activate" );

	CBaseModPanel::GetSingleton().CloseAllWindows();
	CBaseModPanel::GetSingleton().OpenFrontScreen();
}

static void LoadGameCallback()
{
	MainMenu* self = 
		static_cast< MainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_MAINMENU ) );

	if ( self )
	{
		self->Close();
	}

	engine->ExecuteClientCmd( "reload" );
	engine->ExecuteClientCmd( "gameui_hide" );

	CBaseModPanel::GetSingleton().CloseAllWindows();
	CBaseModPanel::GetSingleton().OnGameUIHidden();
}

static void SavedGameCallback()
{
	MainMenu* self = 
		static_cast< MainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_MAINMENU ) );

	if ( self )
	{
		self->Close();
	}

	engine->ClientCmd( "autosave\n" );
	engine->ExecuteClientCmd( "gameui_hide" );

	CBaseModPanel::GetSingleton().CloseAllWindows();
	CBaseModPanel::GetSingleton().OnGameUIHidden();
}

//=============================================================================
void MainMenu::OnCommand( const char *command )
{
	if (!steamapicontext)
	{
		GenericConfirmation* confirmation =
			static_cast<GenericConfirmation*>(CBaseModPanel::GetSingleton().OpenWindow(WT_GENERICCONFIRMATION, this, false));

		GenericConfirmation::Data_t data;

		data.pWindowTitle = "Steamworks Error";
		//data.pMessageText = "#HL2CEUI_BuyHL2AndInstallSDK";

		data.bOkButtonEnabled = true;
		data.pfnOkCallback = &AcceptQuitGameCallback;
		data.bCancelButtonEnabled = false;

		confirmation->SetUsageData(data);

		NavigateFrom();
	}


	int iUserSlot = CBaseModPanel::GetSingleton().GetLastActiveUserId();

	if ( UI_IsDebug() )
	{
		Msg("[GAMEUI] Handling main menu command %s from user%d ctrlr%d\n",
			command, iUserSlot, 0 );
	}

	bool bOpeningFlyout = false;

	if (!Q_strcmp(command, "SoloPlay"))
	{

		CBaseModPanel::GetSingleton().OnOpenNewGameDialog();
		/*if(CheckSaveFile())
		{
			GenericConfirmation* confirmation = 
				static_cast< GenericConfirmation* >( CBaseModPanel::GetSingleton().OpenWindow( WT_GENERICCONFIRMATION, this, false ) );

			GenericConfirmation::Data_t data;

			data.pWindowTitle = "#Template_NewGame_Confirm";
			data.pMessageText = "#Template_NewGame_ConfirmMsg";

			data.bOkButtonEnabled = true;
			data.pfnOkCallback = &AcceptNewGameCallback;
			data.bCancelButtonEnabled = true;

			confirmation->SetUsageData(data);

			NavigateFrom();

			engine->ExecuteClientCmd("sv_cheats 0");
			engine->ExecuteClientCmd("commentary 0");
		}
		else
		{
			if ( MainMenu *pMainMenu = static_cast< MainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_MAINMENU ) ) )
			{
				engine->ExecuteClientCmd( "sv_cheats 0" );
				engine->ExecuteClientCmd( "commentary 0" );
				OnCommand( "SoloPlay_NoConfirm" );
			}

		}*/
	}
	/*else if (!Q_strcmp(command, "SoloPlay_NoConfirm"))
	{
			engine->ClientCmd( "exec chapter1.cfg" );
	}*/

	/*
	else if ( !Q_strcmp( command, "StatsAndAchievements" ) )
	{
	}
	
	*/

	else if ( ( !Q_strcmp( command, "ReturnToGame" ) ) && engine->IsConnected() )
	{
		engine->ClientCmd("gameui_hide");
	}
	else if (!Q_strcmp(command, "Options_Old"))
	{
		CBaseModPanel::GetSingleton().OpenOptionsDialog( this );
	}
	// some hacky reload by just re-opening the menu
	else if (!Q_strcmp(command, "menu_reload"))
	{
		CBaseModPanel::GetSingleton().OpenWindow(WT_MAINMENU, this, true );
	}

	// --
	// OPTIONS
	else if (!Q_strcmp(command, "Audio"))
	{
		if ( ui_old_options_menu.GetBool() )
		{
			CBaseModPanel::GetSingleton().OpenOptionsDialog( this );
		}
		else
		{
			// audio options dialog, PC only
			if ( m_ActiveControl )
			{
				m_ActiveControl->NavigateFrom( );
			}
			CBaseModPanel::GetSingleton().OpenWindow(WT_AUDIO, this, false );
		}
	}
	else if (!Q_strcmp(command, "Video"))
	{
		if ( ui_old_options_menu.GetBool() )
		{
			CBaseModPanel::GetSingleton().OpenOptionsDialog( this );
		}
		else
		{
			// video options dialog, PC only
			if ( m_ActiveControl )
			{
				m_ActiveControl->NavigateFrom( );
			}
			CBaseModPanel::GetSingleton().OpenWindow(WT_VIDEO, this, false );
		}
	}
	else if (!Q_strcmp(command, "Brightness"))
	{
		if ( ui_old_options_menu.GetBool() )
		{
			CBaseModPanel::GetSingleton().OpenOptionsDialog( this );
		}
		else
		{
			// brightness options dialog, PC only
			//OpenGammaDialog( GetVParent() );
			Msg("Sorry, use the old Options Dialog.\n");
		}
	}
	else if (!Q_strcmp(command, "KeyboardMouse"))
	{
		if ( ui_old_options_menu.GetBool() )
		{
			CBaseModPanel::GetSingleton().OpenOptionsDialog( this );
		}
		else
		{
			// standalone keyboard/mouse dialog, PC only
			if ( m_ActiveControl )
			{
				m_ActiveControl->NavigateFrom( );
			}
			CBaseModPanel::GetSingleton().OpenWindow(WT_KEYBOARDMOUSE, this, false );
		}
	}
	else if (!Q_strcmp(command, "Mouse"))
	{
		CBaseModPanel::GetSingleton().OpenOptionsMouseDialog( this );
	}
	else if( Q_stricmp( "#L4D360UI_Controller_Edit_Keys_Buttons", command ) == 0 )
	{
		FlyoutMenu::CloseActiveMenu();
		CBaseModPanel::GetSingleton().OpenKeyBindingsDialog( this );
	}
	
	//else if (!Q_strcmp(command, "SaveGame"))
	else if ( ( !Q_strcmp( command, "SaveGame" ) ) && engine->IsConnected() )
	{

		CBaseModPanel::GetSingleton().OnOpenSaveGameDialog();
		/*GenericConfirmation* confirmation = 
			static_cast< GenericConfirmation* >( CBaseModPanel::GetSingleton().OpenWindow( WT_GENERICCONFIRMATION, this, true ) );

		GenericConfirmation::Data_t data;

		data.pWindowTitle = "#GameUI_ConfirmOverwriteSaveGame_Title";
		data.pMessageText = "#GameUI_ConfirmOverwriteSaveGame_Info";
		data.bOkButtonEnabled = true;
		data.pfnOkCallback = &SavedGameCallback;
		data.bCancelButtonEnabled = true;

		confirmation->SetUsageData(data);*/
	}
	else if (!Q_strcmp(command, "LoadLastSave"))
	{
		CBaseModPanel::GetSingleton().OnOpenLoadGameDialog();
		/*if ( IsPC() )
		{
			GenericConfirmation* confirmation = 
				static_cast< GenericConfirmation* >( CBaseModPanel::GetSingleton().OpenWindow( WT_GENERICCONFIRMATION, this, false ) );

			GenericConfirmation::Data_t data;

			data.pWindowTitle = "#Template_LoadLastSave_Confirm";
			data.pMessageText = "#Template_LoadLastSave_ConfirmMsg";

			data.bOkButtonEnabled = true;
			data.pfnOkCallback = &AcceptLoadCallback;
			data.bCancelButtonEnabled = true;

			confirmation->SetUsageData(data);

			NavigateFrom();
		}*/
	}
	
	/*else if ( !Q_stricmp( command, "LoadLastSave_NoConfirm" ) )
	{
		if ( IsPC() )
		{
			engine->ClientCmd( "load autosave\n" );
		}
	}*/
	else if (!Q_strcmp(command, "Credits"))
	{
		
	}

	else if ( ( !Q_strcmp( command, "ExitToMainMenu" ) ) && engine->IsConnected() )
	{
		GenericConfirmation* confirmation = 
			static_cast< GenericConfirmation* >( CBaseModPanel::GetSingleton().OpenWindow( WT_GENERICCONFIRMATION, this, true ) );

		GenericConfirmation::Data_t data;

		data.pWindowTitle = "#L4D360UI_LeaveMultiplayerConf";
		data.pMessageText = "#L4D360UI_LeaveMultiplayerConfMsg";
		data.bOkButtonEnabled = true;
		data.pfnOkCallback = &LeaveGameOkCallback;
		data.bCancelButtonEnabled = true;

		confirmation->SetUsageData(data);
	}

	else if (!Q_strcmp(command, "QuitGame"))
	{
		if ( IsPC() )
		{
			GenericConfirmation* confirmation = 
				static_cast< GenericConfirmation* >( CBaseModPanel::GetSingleton().OpenWindow( WT_GENERICCONFIRMATION, this, false ) );

			GenericConfirmation::Data_t data;

			data.pWindowTitle = "#L4D360UI_MainMenu_Quit_ConfirmMsg";
			data.pMessageText = GetRandomQuitString();

			data.bOkButtonEnabled = true;
			data.pfnOkCallback = &AcceptQuitGameCallback;
			data.bCancelButtonEnabled = true;

			confirmation->SetUsageData(data);

			NavigateFrom();
		}

		if ( IsX360() )
		{
			engine->ExecuteClientCmd( "demo_exit" );
		}
	}
	else if (!Q_strcmp(command, "SteamworksError"))
	{

		GenericConfirmation* confirmation =
			static_cast<GenericConfirmation*>(CBaseModPanel::GetSingleton().OpenWindow(WT_GENERICCONFIRMATION, this, false));

		GenericConfirmation::Data_t data;

		data.pWindowTitle = "Steamworks Error";
		//data.pMessageText = "#HL2CEUI_BuyHL2AndInstallSDK";

		data.bOkButtonEnabled = true;
		data.pfnOkCallback = &AcceptQuitGameCallback;
		data.bCancelButtonEnabled = false;

		confirmation->SetUsageData(data);

		NavigateFrom();

	}

	else if ( !Q_strcmp( command, "DeveloperCommentary" ) )
	{
		// Explain the rules of commentary
		GenericConfirmation* confirmation = 
			static_cast< GenericConfirmation* >( CBaseModPanel::GetSingleton().OpenWindow( WT_GENERICCONFIRMATION, this, false ) );

		GenericConfirmation::Data_t data;

		data.pWindowTitle = "#GAMEUI_CommentaryDialogTitle";
		data.pMessageText = "#L4D360UI_Commentary_Explanation";

		data.bOkButtonEnabled = true;
		data.pfnOkCallback = &AcceptCommentaryRulesCallback;
		data.bCancelButtonEnabled = true;

		confirmation->SetUsageData(data);
		NavigateFrom();
	}
	else if( !Q_strcmp( command, "Addons" ) )
	{
		CBaseModPanel::GetSingleton().OpenWindow( WT_ADDONS, this, false );
	}
	// what is this
	else if( !Q_strcmp( command, "MyUGC" ) )
	{
		CBaseModPanel::GetSingleton().OpenWindow( WT_MYUGC, this, true );
	}
	else if ( !Q_stricmp( command, "QuitGame_NoConfirm" ) )
	{
		if ( IsPC() )
		{
			engine->ClientCmd( "quit" );
		}
	}
	else if ( !Q_strcmp( command, "FlmExtrasFlyoutCheck" ) )
	{
		OnCommand( "FlmExtrasFlyout_Simple" );
		return;
	}
	else 
	{
		const char *pchCommand = command;
		if ( StringHasPrefix( command, "FlmExtrasFlyout_" ) )
		{
			command = "FlmExtrasFlyoutCheck";
		}

		// does this command match a flyout menu?
		BaseModUI::FlyoutMenu *flyout = dynamic_cast< FlyoutMenu* >( FindChildByName( pchCommand ) );
		if ( flyout )
		{
			bOpeningFlyout = true;

			// If so, enumerate the buttons on the menu and find the button that issues this command.
			// (No other way to determine which button got pressed; no notion of "current" button on PC.)
			for ( int iChild = 0; iChild < GetChildCount(); iChild++ )
			{
				bool bFound = false;
				GameModes *pGameModes = dynamic_cast< GameModes *>( GetChild( iChild ) );
				if ( pGameModes )
				{
					for ( int iGameMode = 0; iGameMode < pGameModes->GetNumGameInfos(); iGameMode++ )
					{
						BaseModHybridButton *pHybrid = pGameModes->GetHybridButton( iGameMode );
						if ( pHybrid && pHybrid->GetCommand() && !Q_strcmp( pHybrid->GetCommand()->GetString( "command"), command ) )
						{
							pHybrid->NavigateFrom();
							// open the menu next to the button that got clicked
							flyout->OpenMenu( pHybrid );
							flyout->SetListener( this );
							bFound = true;
							break;
						}
					}
				}

				if ( !bFound )
				{
					BaseModHybridButton *hybrid = dynamic_cast<BaseModHybridButton *>( GetChild( iChild ) );
					if ( hybrid && hybrid->GetCommand() && !Q_strcmp( hybrid->GetCommand()->GetString( "command"), command ) )
					{
						hybrid->NavigateFrom();
						// open the menu next to the button that got clicked
						flyout->OpenMenu( hybrid );
						flyout->SetListener( this );
						break;
					}
				}
			}
		}
		else
		{
			BaseClass::OnCommand( command );
		}
	}

	if( !bOpeningFlyout )
	{
		FlyoutMenu::CloseActiveMenu(); //due to unpredictability of mouse navigation over keyboard, we should just close any flyouts that may still be open anywhere.
	}
}

//=============================================================================
void MainMenu::OpenMainMenuJoinFailed( const char *msg )
{
}

//=============================================================================
void MainMenu::OnNotifyChildFocus( vgui::Panel* child )
{
}

void MainMenu::OnFlyoutMenuClose( vgui::Panel* flyTo )
{
	SetFooterState();
}

void MainMenu::OnFlyoutMenuCancelled()
{
}

//=============================================================================
void MainMenu::OnKeyCodePressed( KeyCode code )
{
	int userId = GetJoystickForCode( code );
	BaseModUI::CBaseModPanel::GetSingleton().SetLastActiveUserId( userId );

	switch( GetBaseButtonCode( code ) )
	{
	case KEY_XBUTTON_B:
		// Capture the B key so it doesn't play the cancel sound effect
		break;

	case KEY_XBUTTON_BACK:
#ifdef _X360
		if ( XBX_GetNumGameUsers() > 1 )
		{
			OnCommand( "DisableSplitscreen" );
		}
#endif
		break;
	/*
	case KEY_XBUTTON_INACTIVE_START:
#ifdef _X360
		if ( !XBX_GetPrimaryUserIsGuest() &&
			 userId != (int) XBX_GetPrimaryUserId() &&
			 userId >= 0 &&
			 CUIGameData::Get()->CanPlayer2Join() )
		{
			// Pass the index of controller which wanted to join splitscreen
			CBaseModPanel::GetSingleton().CloseAllWindows();
			CAttractScreen::SetAttractMode( CAttractScreen::ATTRACT_GOSPLITSCREEN, userId );
			CBaseModPanel::GetSingleton().OpenWindow( WT_ATTRACTSCREEN, NULL, true );
		}
#endif
		break;
	*/

	default:
		BaseClass::OnKeyCodePressed( code );
		break;
	}
}

//=============================================================================
bool MainMenu::CheckSaveFile()
{
	char modPath[MAX_PATH];
	char Filename[MAX_PATH];

	GetPrimaryModDirectoryForMenu( modPath, MAX_PATH );
	V_snprintf( Filename, sizeof( Filename ), "%s%s%c%s", modPath, SAVE_DIRNAME, CORRECT_PATH_SEPARATOR, SAVE_FILENAME );
	
	FileFindHandle_t findHandleVMT;
	bool bHaveFile = ( NULL != g_pFullFileSystem->FindFirst( Filename, &findHandleVMT ) );
	g_pFullFileSystem->FindClose( findHandleVMT );

	if ( bHaveFile )
	{
		return true;
	}

	return false;
	
}
//=============================================================================
void MainMenu::OnThink()
{
	BaseClass::OnThink();

	if ( IsPC() )
	{
		FlyoutMenu *pFlyout = dynamic_cast< FlyoutMenu* >( FindChildByName( "FlmOptionsFlyout" ) );
		if ( pFlyout )
		{
			const MaterialSystem_Config_t &config = materials->GetCurrentConfigForVideoCard();
			pFlyout->SetControlEnabled( "BtnBrightness", !config.Windowed() );
		}

		bool bContinueableGame = CheckSaveFile(); //first_time_play.GetBool();

		SetControlEnabled( "BtnContinueGame", bContinueableGame );

		// --------------------------------------
		// only show these buttons when in game
		SetControlVisible("BtnReturnToGame", engine->IsConnected());
		SetControlVisible("BtnSaveGame", engine->IsConnected());
		SetControlVisible("BtnDisconnect", engine->IsConnected());

		// only show this button in the menu
		SetControlVisible("BtnQuit", !engine->IsConnected());
		// --------------------------------------
	}
}

//=============================================================================
void MainMenu::OnOpen()
{
	LoadControlSettings( "Resource/UI/BaseModUI/MainMenu.res" );

	if ( IsPC() && connect_lobby.GetString()[0] )
	{
		// if we were launched with "+connect_lobby <lobbyid>" on the command line, join that lobby immediately
		uint64 nLobbyID = _atoi64( connect_lobby.GetString() );
		if ( nLobbyID != 0 )
		{
			
		}
		// clear the convar so we don't try to join that lobby every time we return to the main menu
		connect_lobby.SetValue( "" );
	}

	BaseClass::OnOpen();

	SetFooterState();
	/*
#ifndef _X360
	bool bSteamCloudVisible = false;

	{
		static CGameUIConVarRef cl_cloud_settings( "cl_cloud_settings" );
		if ( cl_cloud_settings.GetInt() == -1 )
		{
			CBaseModPanel::GetSingleton().OpenWindow( WT_STEAMCLOUDCONFIRM, this, false );
			bSteamCloudVisible = true;
		}
	}


	if ( !bSteamCloudVisible )
	{
		if ( AddonAssociation::CheckAndSeeIfShouldShow() )
		{
			CBaseModPanel::GetSingleton().OpenWindow( WT_ADDONASSOCIATION, this, false );
		}
	}

#endif
	*/
}

//=============================================================================
void MainMenu::RunFrame()
{
	BaseClass::RunFrame();
}

//=============================================================================
#ifdef _X360
void MainMenu::Activate()
{
	BaseClass::Activate();
	OnFlyoutMenuClose( NULL );
}
#endif

//=============================================================================
void MainMenu::PaintBackground() 
{
}

void MainMenu::SetFooterState()
{
	CBaseModFooterPanel *footer = BaseModUI::CBaseModPanel::GetSingleton().GetFooterPanel();
	if ( footer )
	{
		CBaseModFooterPanel::FooterButtons_t buttons = FB_ABUTTON;
#if defined( _X360 )
		if ( XBX_GetPrimaryUserIsGuest() == 0 )
		{
			buttons |= FB_XBUTTON;
		}
#endif

		footer->SetButtons( buttons, FF_MAINMENU, false );
		footer->SetButtonText( FB_ABUTTON, "#L4D360UI_Select" );
		footer->SetButtonText( FB_XBUTTON, "#L4D360UI_MainMenu_SeeAll" );
	}
}

//=============================================================================
void MainMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	//LoadControlSettings( "Resource/UI/BaseModUI/MainMenu.res" );

	//SetPaintBackgroundEnabled( true );

	//SetFooterState();

	const char *pSettings = "Resource/UI/BaseModUI/MainMenu.res";

	if (!steamapicontext)
		pSettings = "Resource/UI/BaseModUI/mainmenu_steamworkserror.res";

#if !defined( _X360 )
	/*
	if ( !g_pMatchFramework->GetMatchSystem()->GetPlayerManager()->GetLocalPlayer( 0 ) )
	{
		pSettings = "Resource/UI/BaseModUI/MainMenuStub.res";
	}
	*/
#endif
	
	LoadControlSettings( pSettings );

	BaseModHybridButton *button = dynamic_cast< BaseModHybridButton* >( FindChildByName( "BtnPlaySolo" ) );
	if ( button )
	{
	}


	if ( IsPC() )
	{
		FlyoutMenu *pFlyout = dynamic_cast< FlyoutMenu* >( FindChildByName( "FlmOptionsFlyout" ) );
		if ( pFlyout )
		{
			bool bUsesCloud = false;

#ifdef IS_WINDOWS_PC
			ISteamRemoteStorage *pRemoteStorage = SteamClient()?(ISteamRemoteStorage *)SteamClient()->GetISteamGenericInterface(
				SteamAPI_GetHSteamUser(), SteamAPI_GetHSteamPipe(), STEAMREMOTESTORAGE_INTERFACE_VERSION ):NULL;
#else
			ISteamRemoteStorage *pRemoteStorage =  NULL; 
			AssertMsg( false, "This branch run on a PC build without IS_WINDOWS_PC defined." );
#endif

			int32 availableBytes, totalBytes = 0;
			if ( pRemoteStorage && pRemoteStorage->GetQuota( &totalBytes, &availableBytes ) )
			{
				if ( totalBytes > 0 )
				{
					bUsesCloud = true;
				}
			}

			pFlyout->SetControlEnabled( "BtnCloud", bUsesCloud );
		}
	}

	SetFooterState();

	if ( IsX360() )
	{		
		GameModes *pGameModes =  dynamic_cast< GameModes* >( FindChildByName( "BtnGameModes" ) );	
		if ( pGameModes )
		{
			char lastActive[MAX_PATH];
			if ( pGameModes->GetLastActiveNameId( lastActive, sizeof( lastActive ) ) )
			{
				pGameModes->SetActive( lastActive, true );
			}
			else
			{
				pGameModes->SetActive( "BtnPlaySolo", true );
			}
			m_ActiveControl = pGameModes;
		}
	}

	if ( IsPC() )
	{
		vgui::Panel *firstPanel = FindChildByName( "BtnCoOp" );
		if ( firstPanel )
		{
			if ( m_ActiveControl )
			{
				m_ActiveControl->NavigateFrom( );
			}
			firstPanel->NavigateTo();
		}
	}

#if defined( _X360 ) && defined( _DEMO )
	SetControlVisible( "BtnExtras", !engine->IsDemoHostedFromShell() );
	SetControlVisible( "BtnQuit", engine->IsDemoHostedFromShell() );
#endif

	// CERT CATCH ALL JUST IN CASE!
#ifdef _X360
	bool bAllUsersCorrectlySignedIn = ( XBX_GetNumGameUsers() > 0 );
	for ( int k = 0; k < ( int ) XBX_GetNumGameUsers(); ++ k )
	{
		if ( !g_pMatchFramework->GetMatchSystem()->GetPlayerManager()->GetLocalPlayer( XBX_GetUserId( k ) ) )
			bAllUsersCorrectlySignedIn = false;
	}
	if ( !bAllUsersCorrectlySignedIn )
	{
		Warning( "======= SIGNIN FAIL SIGNIN FAIL SIGNIN FAIL SIGNIN FAIL ==========\n" );
		Assert( 0 );
		CBaseModPanel::GetSingleton().CloseAllWindows( CBaseModPanel::CLOSE_POLICY_EVEN_MSGS );
		CAttractScreen::SetAttractMode( CAttractScreen::ATTRACT_GAMESTART );
		CBaseModPanel::GetSingleton().OpenWindow( WT_ATTRACTSCREEN, NULL, true );
		Warning( "======= SIGNIN RESET SIGNIN RESET SIGNIN RESET SIGNIN RESET ==========\n" );
	}
#endif
}

const char *pDemoDisabledButtons[] = { "BtnVersus", "BtnSurvival", "BtnStatsAndAchievements", "BtnExtras" };

void MainMenu::Demo_DisableButtons( void )
{
	for ( int i = 0; i < ARRAYSIZE( pDemoDisabledButtons ); i++ )
	{
		BaseModHybridButton *pButton = dynamic_cast< BaseModHybridButton* >( FindChildByName( pDemoDisabledButtons[i] ) );

		if ( pButton )
		{
			Demo_DisableButton( pButton );
		}
	}
}

void MainMenu::AcceptNewGameCallback()
{
	if ( MainMenu *pMainMenu = static_cast< MainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_MAINMENU ) ) )
	{
		pMainMenu->OnCommand( "SoloPlay_NoConfirm" );
	}
}

void MainMenu::AcceptCommentaryRulesCallback() 
{
	if ( MainMenu *pMainMenu = static_cast< MainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_MAINMENU ) ) )
	{
		engine->ExecuteClientCmd( "sv_cheats 1" );
		engine->ExecuteClientCmd( "commentary 1" );
		pMainMenu->OnCommand( "SoloPlay_NoConfirm" );
	}
}

void MainMenu::AcceptSplitscreenDisableCallback()
{
	if ( MainMenu *pMainMenu = static_cast< MainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_MAINMENU ) ) )
	{
		pMainMenu->OnCommand( "DisableSplitscreen_NoConfirm" );
	}
}

void MainMenu::AcceptQuitGameCallback()
{
	if ( MainMenu *pMainMenu = static_cast< MainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_MAINMENU ) ) )
	{
		pMainMenu->OnCommand( "QuitGame_NoConfirm" );
	}
}

void MainMenu::AcceptSaveOverCallback()
{
	if ( MainMenu *pMainMenu = static_cast< MainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_MAINMENU ) ) )
	{
		pMainMenu->OnCommand( "SaveGame" );
	}
}

void MainMenu::AcceptLoadCallback()
{
	if ( MainMenu *pMainMenu = static_cast< MainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_MAINMENU ) ) )
	{
		pMainMenu->OnCommand( "LoadLastSave_NoConfirm" );
	}
}

void MainMenu::AcceptVersusSoftLockCallback()
{
	if ( MainMenu *pMainMenu = static_cast< MainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_MAINMENU ) ) )
	{
		pMainMenu->OnCommand( "FlmVersusFlyout" );
	}
}
