//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef _BASEMODFACTORYBASEPANEL_H__
#define _BASEMODFACTORYBASEPANEL_H__

#include "vgui_controls/Panel.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/PHandle.h"
#include "vgui_controls/MenuItem.h"
#include "vgui_controls/messagedialog.h"
#include "tier1/utllinkedlist.h"
#include "shared\settings_old\OptionsDialog.h"
#include "shared\settings_old\OptionsSubKeyboard.h"
#include "shared\settings_old\OptionsSubMouse.h"
#include "shared\settings_old\optionsmousedialog.h"

//#include "avi/ibik.h"
#include "ixboxsystem.h"

class COptionsDialog;
class COptionsMouseDialog;
class IMaterial;
class CMatchmakingBasePanel;
class CBackgroundMenuButton;
class CGameMenu;

enum
{
	DIALOG_STACK_IDX_STANDARD,
	DIALOG_STACK_IDX_WARNING,
	DIALOG_STACK_IDX_ERROR,
};

// X360TBD: Move into a separate module when finished
/*class CMessageDialogHandler
{
public:
	CMessageDialogHandler();
	void ShowMessageDialog(int nType, vgui::Panel *pOwner);
	void CloseMessageDialog(const uint nType = 0);
	void CloseAllMessageDialogs();
	void CreateMessageDialog(const uint nType, const char *pTitle, const char *pMsg, const char *pCmdA, const char *pCmdB, vgui::Panel *pCreator, bool bShowActivity = false);
	void ActivateMessageDialog(int nStackIdx);
	void PositionDialogs(int wide, int tall);
	void PositionDialog(vgui::PHandle dlg, int wide, int tall);

private:
	static const int MAX_MESSAGE_DIALOGS = 3;
	vgui::DHANDLE< CMessageDialog > m_hMessageDialogs[MAX_MESSAGE_DIALOGS];
	int							m_iDialogStackTop;
};*/

// must supply some non-trivial time to let the movie startup smoothly
// the attract screen also uses this so it doesn't pop in either
#define TRANSITION_TO_MOVIE_DELAY_TIME	0.5f	// how long to wait before starting the fade
#define TRANSITION_TO_MOVIE_FADE_TIME	1.2f	// how fast to fade

class IVTFTexture;

namespace BaseModUI 
{
	enum WINDOW_TYPE 
	{
		WT_NONE = 0,
		WT_ACHIEVEMENTS,
		WT_AUDIO,
		WT_AUDIOVIDEO,
		WT_CLOUD,
		WT_CONTROLLER,
		WT_CONTROLLER_STICKS,
		WT_CONTROLLER_BUTTONS,
		WT_DOWNLOADS,
		WT_GAMELOBBY,
		WT_GAMEOPTIONS,
		WT_GAMESETTINGS,
		WT_GENERICCONFIRMATION,
		WT_INGAMEDIFFICULTYSELECT,
		WT_INGAMEMAINMENU,
		WT_INGAMECHAPTERSELECT,
		WT_INGAMEKICKPLAYERLIST,
		WT_VOTEOPTIONS,
		WT_KEYBOARDMOUSE,
		WT_LOADINGPROGRESSBKGND,
		WT_LOADINGPROGRESS,
		WT_MAINMENU,
		//WT_MULTIPLAYER,
		WT_OPTIONS,
		WT_SEARCHINGFORLIVEGAMES,
		WT_SIGNINDIALOG,
		WT_SINGLEPLAYER,
		WT_GENERICWAITSCREEN,
		WT_ATTRACTSCREEN,
		WT_ALLGAMESEARCHRESULTS,
		WT_FOUNDPUBLICGAMES,
		WT_TRANSITIONSCREEN,
		WT_PASSWORDENTRY,
		WT_VIDEO,
		WT_STEAMCLOUDCONFIRM,
		WT_STEAMGROUPSERVERS,
		WT_CUSTOMCAMPAIGNS,
		WT_ADDONS,
		WT_DOWNLOADCAMPAIGN,
		WT_LEADERBOARD,
		WT_ADDONASSOCIATION,
		WT_GETLEGACYDATA,
		WT_JUKEBOX,
		WT_MYUGC,
		WT_MYUGCPOPUP,
		WT_WINDOW_COUNT // WT_WINDOW_COUNT must be last in the list!
	};

	enum WINDOW_PRIORITY 
	{
		WPRI_NONE,
		WPRI_BKGNDSCREEN,
		WPRI_NORMAL,
		WPRI_WAITSCREEN,
		WPRI_MESSAGE,
		WPRI_LOADINGPLAQUE,
		WPRI_TOPMOST,			// must be highest priority, no other windows can obscure
		WPRI_COUNT				// WPRI_COUNT must be last in the list!
	};

	enum UISound_t
	{
		UISOUND_BACK,
		UISOUND_ACCEPT,
		UISOUND_INVALID,
		UISOUND_COUNTDOWN,
		UISOUND_FOCUS,
		UISOUND_CLICK,
		UISOUND_DENY,
	};

	class CBaseModFrame;
	class CBaseModFooterPanel;

	//=============================================================================
	//
	//=============================================================================
	class CBaseModPanel : public vgui::EditablePanel
	{
		DECLARE_CLASS_SIMPLE( CBaseModPanel, vgui::EditablePanel );

	public:
		CBaseModPanel();
		~CBaseModPanel();

		// IMatchEventSink implementation
	public:
		virtual void OnEvent( KeyValues *pEvent );

	public:
		static CBaseModPanel& GetSingleton();
		static CBaseModPanel* GetSingletonPtr();

		void ReloadScheme();
		//void ReloadScheme( bool layoutNow, bool reloadScheme );

		CBaseModFrame* OpenWindow( const WINDOW_TYPE& wt, CBaseModFrame * caller, bool hidePrevious = true, KeyValues *pParameters = NULL );
		CBaseModFrame* GetWindow( const WINDOW_TYPE& wt );

		void OnFrameClosed( WINDOW_PRIORITY pri, WINDOW_TYPE wt );
		void DbgShowCurrentUIState();
		bool IsLevelLoading();

		WINDOW_TYPE GetActiveWindowType();
		WINDOW_PRIORITY GetActiveWindowPriority();
		void SetActiveWindow( CBaseModFrame * frame );

		enum CloseWindowsPolicy_t
		{
			CLOSE_POLICY_DEFAULT = 0,			// will keep msg boxes alive
			CLOSE_POLICY_EVEN_MSGS = 1,			// will kill even msg boxes
			CLOSE_POLICY_EVEN_LOADING = 2,		// will kill even loading screen
			CLOSE_POLICY_KEEP_BKGND = 0x100,	// will keep bkgnd screen
		};
		void CloseAllWindows( int ePolicyFlags = CLOSE_POLICY_DEFAULT );

		void OnGameUIActivated();
		void OnGameUIHidden();
		void OpenFrontScreen();
		void RunFrame();
		void OnLevelLoadingStarted( char const *levelName, bool bShowProgressDialog );
		void OnLevelLoadingFinished( KeyValues *kvEvent );
		bool UpdateProgressBar(float progress, const char *statusText);
		void OnCreditsFinished(void);

		void SetHelpText( const char* helpText );
		void SetOkButtonEnabled( bool enabled );
		void SetCancelButtonEnabled( bool enabled );

		bool IsReadyToWriteConfig( void );
		const char *GetUISoundName(  UISound_t uiSound );
		void PlayUISound( UISound_t uiSound );
		void StartExitingProcess( bool bWarmRestart );

		CBaseModFooterPanel* GetFooterPanel();
		void SetLastActiveUserId( int userId );
		int GetLastActiveUserId();
		void OpenOptionsDialog( EditablePanel *parent );
		void OpenOptionsMouseDialog( EditablePanel *parent );
		void OpenKeyBindingsDialog( EditablePanel *parent );

		MESSAGE_FUNC_CHARPTR( OnNavigateTo, "OnNavigateTo", panelName );

		bool IsMenuBackgroundMovieValid( void );

		bool IsBackgroundMusicPlaying();
		bool StartBackgroundMusic( float fVol );
		void UpdateBackgroundMusicVolume( float fVol );
		void ReleaseBackgroundMusic();

		void SafeNavigateTo( Panel *pExpectedFrom, Panel *pDesiredTo, bool bAllowStealFocus );

#if defined( _X360 ) && defined( _DEMO )
		void OnDemoTimeout();
#endif

	protected:
		CBaseModPanel(const CBaseModPanel&);
		CBaseModPanel& operator=(const CBaseModPanel&);

		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PaintBackground();

		void OnCommand(const char *command);
		void OnSetFocus();

		MESSAGE_FUNC( OnMovedPopupToFront, "OnMovedPopupToFront" );

	private:
		void DrawColoredText( vgui::HFont hFont, int x, int y, unsigned int color, const char *pAnsiText );
		void DrawCopyStats();
		void OnEngineLevelLoadingSession( KeyValues *pEvent );
		bool ActivateBackgroundEffects();

		static CBaseModPanel* m_CFactoryBasePanel;

		vgui::DHANDLE< CBaseModFrame > m_Frames[WT_WINDOW_COUNT];
		vgui::DHANDLE< CBaseModFooterPanel > m_FooterPanel;
		WINDOW_TYPE m_ActiveWindow[WPRI_COUNT];
		bool m_LevelLoading;
		vgui::HScheme m_UIScheme;
		vgui::DHANDLE<COptionsDialog> m_hOptionsDialog;	// standalone options dialog - PC only
		vgui::DHANDLE<COptionsMouseDialog> m_hOptionsMouseDialog;	// standalone options dialog - PC only
		int m_lastActiveUserId;

		vgui::HFont m_hDefaultFont;

		int	m_iBackgroundImageID;
		int	m_iFadeToBackgroundImageID;
		float m_flMovieFadeInTime;

		int m_DelayActivation;
		int m_ExitingFrameCount;
		bool m_bWarmRestartMode;
		bool m_bClosingAllWindows;

		float m_flBlurScale;
		float m_flLastBlurTime;

		CUtlString m_backgroundMusic;
		int m_nBackgroundMusicGUID;

		/*int m_iProductImageID;
		int m_nProductImageX;
		int m_nProductImageY;
		int m_nProductImageWide;
		int m_nProductImageTall;*/

		char m_szFadeFilename[ MAX_PATH ];
		IMaterial *m_pBackgroundMaterial;
		KeyValues *m_pVMTKeyValues;

		void PrepareStartupGraphic();
		void ReleaseStartupGraphic();
		void DrawStartupGraphic( float flNormalizedAlpha );
		IVTFTexture			*m_pBackgroundTexture;

	public:
		// 2007 src GameUI port for use of HL2 panels
		vgui::AnimationController *GetOldAnimationController(void) { return m_pConsoleAnimationController; } //was GetAnimationController but that conflicts with existing ASW GameUI functions
		void RunCloseAnimation(const char *animName);
		void RunAnimationWithCallback(vgui::Panel *parent, const char *animName, KeyValues *msgFunc);
		vgui::AnimationController	*m_pConsoleAnimationController;
		void ShowMessageDialog(const uint nType, vgui::Panel *pParent = NULL);
		//CMessageDialogHandler		m_MessageDialogHandler;
		KeyValues					*m_pConsoleControlSettings;
		KeyValues *GetConsoleControlSettings(void);
		void FadeToBlackAndRunEngineCommand(const char *engineCommand);// fades to black then runs an engine command (usually to start a level)
		void SetMenuItemBlinkingState(const char *itemName, bool state); // sets the blinking state of a menu item
		void PositionDialog(vgui::PHandle dlg);

		// game dialogs
		void OnOpenNewGameDialog(const char *chapter = NULL);
		void OnOpenBonusMapsDialog();
		void OnOpenLoadGameDialog();
		void OnOpenSaveGameDialog();

		vgui::DHANDLE<vgui::Frame> m_hNewGameDialog;
		vgui::DHANDLE<vgui::Frame> m_hBonusMapsDialog;
		vgui::DHANDLE<vgui::Frame> m_hLoadGameDialog;
		vgui::DHANDLE<vgui::Frame> m_hLoadGameDialog_Xbox;
		vgui::DHANDLE<vgui::Frame> m_hSaveGameDialog;
		vgui::DHANDLE<vgui::Frame> m_hSaveGameDialog_Xbox;
		//vgui::DHANDLE<vgui::PropertyDialog> m_hOptionsDialog;
		vgui::DHANDLE<vgui::Frame> m_hOptionsDialog_Xbox;
		vgui::DHANDLE<vgui::Frame> m_hCreateMultiplayerGameDialog;
		//vgui::DHANDLE<vgui::Frame> m_hDemoPlayerDialog;
		vgui::DHANDLE<vgui::Frame> m_hChangeGameDialog;
		vgui::DHANDLE<vgui::Frame> m_hPlayerListDialog;
		vgui::DHANDLE<vgui::Frame> m_hBenchmarkDialog;
		vgui::DHANDLE<vgui::Frame> m_hLoadCommentaryDialog;
		vgui::DHANDLE<vgui::Frame> m_hAchievementsDialog;
	};

	//-----------------------------------------------------------------------------
	// Purpose: singleton accessor
	//-----------------------------------------------------------------------------
	extern CBaseModPanel *BasePanel();
};

//-----------------------------------------------------------------------------
// Purpose: Transparent menu item designed to sit on the background ingame
//-----------------------------------------------------------------------------
class CGameMenuItem : public vgui::MenuItem
{
	DECLARE_CLASS_SIMPLE( CGameMenuItem, vgui::MenuItem );
public:
	CGameMenuItem(vgui::Menu *parent, const char *name);

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PaintBackground( void );
	void SetRightAlignedText( bool state );

private:
	bool		m_bRightAligned;
};

//-----------------------------------------------------------------------------
// Purpose: Panel that acts as background for button icons and help text in the UI
//-----------------------------------------------------------------------------
class CFooterPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CFooterPanel, vgui::EditablePanel );

public:
	CFooterPanel( Panel *parent, const char *panelName );
	virtual ~CFooterPanel();

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	ApplySettings( KeyValues *pResourceData );
	virtual void	Paint( void );
	virtual void	PaintBackground( void );

	// caller tags the current hint, used to assist in ownership
	void			SetHelpNameAndReset( const char *pName );
	const char		*GetHelpName();

	void			AddButtonsFromMap( vgui::Frame *pMenu );
	void			SetStandardDialogButtons();
	void			AddNewButtonLabel( const char *text, const char *icon );
	void			ShowButtonLabel( const char *name, bool show = true );
	void			SetButtonText( const char *buttonName, const char *text );
	void			ClearButtons();
	void			SetButtonGap( int nButtonGap ){ m_nButtonGap = nButtonGap; }
	void			UseDefaultButtonGap(){ m_nButtonGap = m_nButtonGapDefault; }

private:
	struct ButtonLabel_t
	{
		bool	bVisible;
		char	name[MAX_PATH];
		wchar_t	text[MAX_PATH];
		wchar_t	icon[2];			// icon is a single character
	};

	CUtlVector< ButtonLabel_t* > m_ButtonLabels;

	vgui::Label		*m_pSizingLabel;		// used to measure font sizes

	bool			m_bPaintBackground;		// fill the background?
	bool			m_bCenterHorizontal;	// center buttons horizontally?
	int				m_ButtonPinRight;		// if not centered, this is the distance from the right margin that we use to start drawing buttons (right to left)
	int				m_nButtonGap;			// space between buttons when drawing
	int				m_nButtonGapDefault;		// space between buttons (initial value)
	int				m_FooterTall;			// height of the footer
	int				m_ButtonOffsetFromTop;	// how far below the top the buttons should be drawn
	int				m_ButtonSeparator;		// space between the button icon and text
	int				m_TextAdjust;			// extra adjustment for the text (vertically)...text is centered on the button icon and then this value is applied

	char			m_szTextFont[64];		// font for the button text
	char			m_szButtonFont[64];		// font for the button icon
	char			m_szFGColor[64];		// foreground color (text)
	char			m_szBGColor[64];		// background color (fill color)
	
	vgui::HFont		m_hButtonFont;
	vgui::HFont		m_hTextFont;
	char			*m_pHelpName;
};

#endif
