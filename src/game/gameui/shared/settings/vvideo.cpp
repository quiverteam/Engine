//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "VVideo.h"
#include "VFooterPanel.h"
#include "VDropDownMenu.h"
#include "VSliderControl.h"
#include "VHybridButton.h"
#include "EngineInterface.h"
#include "IGameUIFuncs.h"
#include "gameui_util.h"
#include "vgui/ISurface.h"
#include "modes.h"
#include "VGenericConfirmation.h"
#include "nb_header_footer.h"
#include "materialsystem/materialsystem_config.h"
#include "cdll_util.h"

#ifdef _X360
#include "xbox/xbox_launch.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;


#define VIDEO_ANTIALIAS_COMMAND_PREFIX "_antialias"
#define VIDEO_RESLUTION_COMMAND_PREFIX "_res"

extern ConVar ui_gameui_modal;

int GetScreenAspectMode( int width, int height );
void GetResolutionName( vmode_t *mode, char *sz, int sizeofsz );


void SetFlyoutButtonText( const char *pchCommand, FlyoutMenu *pFlyout, const char *pchNewText )
{
	Button *pButton = pFlyout->FindChildButtonByCommand( pchCommand );
	if ( pButton )
	{
		pButton->SetText( pchNewText );
		pButton->SetVisible( true );
	}
}

/*void AcceptPagedPoolMemWarningCallback()
{
	Video *self = static_cast<Video*>( CBaseModPanel::GetSingleton().GetWindow( WT_VIDEO ) );
	if( self )
	{
		self->OpenPagedPoolMem();
	}
}

void PagedPoolMemOpenned( DropDownMenu *pDropDownMenu, FlyoutMenu *pFlyoutMenu )
{
	GenericConfirmation* confirmation = 
		static_cast< GenericConfirmation* >( CBaseModPanel::GetSingleton().OpenWindow( WT_GENERICCONFIRMATION, CBaseModPanel::GetSingleton().GetWindow( WT_VIDEO ), false ) );

	GenericConfirmation::Data_t data;

	data.pWindowTitle = "#L4D360UI_VideoOptions_Paged_Pool_Mem";
	data.pMessageText = "#L4D360UI_VideoOptions_Paged_Pool_Mem_Info";

	data.bOkButtonEnabled = true;
	data.pfnOkCallback = AcceptPagedPoolMemWarningCallback;

	confirmation->SetUsageData(data);
}*/


//=============================================================================
Video::Video(Panel *parent, const char *panelName):
BaseClass(parent, panelName)
{
	SetDeleteSelfOnClose(true);

	SetProportional( true );
	SetSizeable( true );

	//SetUpperGarnishEnabled(true);
	//SetLowerGarnishEnabled(true);

	m_drpAspectRatio = NULL;
	m_drpResolution = NULL;
	m_drpDisplayMode = NULL;
	//m_drpLockMouse = NULL;
	//m_sldFilmGrain = NULL;

	//m_btnAdvanced = NULL;

	m_drpModelDetail = NULL;
	m_drpLOD = NULL;
	m_drpTextureDetail = NULL;
	m_drpWaterDetail = NULL;
	m_drpAntialias = NULL;
	m_drpFiltering = NULL;
	m_drpVSync = NULL;
	m_drpQueuedMode = NULL;
	m_drpShaderDetail = NULL;
	m_drpShadowDetail = NULL;
	m_drpDXLevel = NULL;

	m_drpCurrent = NULL;
	m_drpHDRLevel = NULL;


	m_btnUseRecommended = NULL;
	m_btnCancel = NULL;
	m_btnDone = NULL;

	//m_btn3rdPartyCredits = NULL;

	/*m_pHeaderFooter = new CNB_Header_Footer( this, "HeaderFooter" );
	m_pHeaderFooter->SetTitle( "" );
	m_pHeaderFooter->SetHeaderEnabled( false );
	m_pHeaderFooter->SetFooterEnabled( true );
	m_pHeaderFooter->SetGradientBarEnabled( true );
	m_pHeaderFooter->SetGradientBarPos( 100, 310 );*/

	//CGameUIConVarRef mat_grain_scale_override( "mat_grain_scale_override" );
	//m_flFilmGrainInitialValue = mat_grain_scale_override.GetFloat();

	m_bDirtyValues = false;

	// can use esc key when in menu
	GameUI().AllowEngineHideGameUI();
}

//=============================================================================
Video::~Video()
{
	GameUI().AllowEngineHideGameUI();
}

void Video::PerformLayout()
{
	BaseClass::PerformLayout();

	//SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );
}

/*void Video::OpenPagedPoolMem( void )
{
	if ( m_drpPagedPoolMem )
	{
		m_drpPagedPoolMem->SetOpenCallback( NULL );
		m_drpPagedPoolMem->OnCommand( "FlmPagedPoolMem" );
	}
}*/

//=============================================================================
void Video::SetupActivateData( void )
{
	const MaterialSystem_Config_t &config = materials->GetCurrentConfigForVideoCard();
	m_iResolutionWidth = config.m_VideoMode.m_Width;
	m_iResolutionHeight = config.m_VideoMode.m_Height;
	m_iAspectRatio = GetScreenAspectMode( m_iResolutionWidth, m_iResolutionHeight );
	m_bWindowed = config.Windowed();
	//m_bNoBorder = false;// config.NoWindowBorder();

	/*CGameUIConVarRef gpu_mem_level( "gpu_mem_level" );
	m_iModelTextureDetail = clamp( gpu_mem_level.GetInt(), 0, 2);

	CGameUIConVarRef mem_level( "mem_level" );
	m_iPagedPoolMem = clamp( mem_level.GetInt(), 0, 2);*/

	CGameUIConVarRef r_rootlod( "r_rootlod" );
	m_iModelDetail =  clamp( r_rootlod.GetInt(), 0, 2);
	CGameUIConVarRef r_lod( "r_lod" );
	m_iLOD = clamp( r_lod.GetInt(), -1, 0);

	CGameUIConVarRef mat_picmip( "mat_picmip" );
	m_iTextureDetail = clamp( mat_picmip.GetInt(), -1, 4);

	CGameUIConVarRef r_waterforceexpensive( "r_waterforceexpensive" );
	CGameUIConVarRef r_waterforcereflectentities( "r_waterforcereflectentities" );

	if ( r_waterforcereflectentities.GetBool() && r_waterforcereflectentities.GetBool() )
	{
		m_iWaterDetail = 2;
	}
	else if ( r_waterforceexpensive.GetBool() )
	{
		m_iWaterDetail = 1;
	}
	else
	{
		m_iWaterDetail = 0;
	}
	
	CGameUIConVarRef r_shadowrendertotexture( "r_shadowrendertotexture" );
	CGameUIConVarRef r_flashlightdepthtexture( "r_flashlightdepthtexture" );

	if ( r_shadowrendertotexture.GetBool() && r_flashlightdepthtexture.GetBool() )
	{
		m_iShadowDetail = 2;
	}
	else if ( r_shadowrendertotexture.GetBool() )
	{
		m_iShadowDetail = 1;
	}
	else
	{
		m_iShadowDetail = 0;
	}
	
	CGameUIConVarRef mat_antialias( "mat_antialias" );
	CGameUIConVarRef mat_aaquality( "mat_aaquality" );
	m_nAASamples = mat_antialias.GetInt();
	m_nAAQuality = mat_aaquality.GetInt();

	CGameUIConVarRef mat_forceaniso( "mat_forceaniso" );
	m_iFiltering = mat_forceaniso.GetInt();

	CGameUIConVarRef mat_vsync( "mat_vsync" );
	m_bVSync = mat_vsync.GetBool();

	//CGameUIConVarRef mat_triplebuffered( "mat_triplebuffered" );
	//m_bTripleBuffered = mat_triplebuffered.GetBool();

	CGameUIConVarRef mat_queue_mode( "mat_queue_mode" );
	m_iQueuedMode = mat_queue_mode.GetInt();

	CGameUIConVarRef mat_dxlevel( "mat_dxlevel" );
	m_iDXLevel = mat_dxlevel.GetInt();

	CGameUIConVarRef mat_hdr_level( "mat_hdr_level" );
	mat_hdr_level.SetValue( mat_hdr_level.GetInt() );
	//m_iHDRLevel = mat_hdr_level.GetInt();
}

//=============================================================================
bool Video::SetupRecommendedActivateData( void )
{
	KeyValues *pConfigKeys = new KeyValues( "VideoConfig" );
	if ( !pConfigKeys )
		return false;

	/*
	if ( !ReadCurrentVideoConfig( pConfigKeys, true ) )
	{
		pConfigKeys->deleteThis();
		return false;
	}
	*/

	//m_iResolutionWidth = pConfigKeys->GetInt( "setting.defaultres", 640 );
	//m_iResolutionHeight = pConfigKeys->GetInt( "setting.defaultresheight", 480 );
	m_iResolutionWidth = pConfigKeys->GetInt( "setting.defaultres", 1280 );
	m_iResolutionHeight = pConfigKeys->GetInt( "setting.defaultresheight", 720 );
	m_iAspectRatio = GetScreenAspectMode( m_iResolutionWidth, m_iResolutionHeight );
	m_bWindowed = !pConfigKeys->GetBool( "setting.fullscreen", true );
	//m_bNoBorder = pConfigKeys->GetBool( "setting.nowindowborder", false );
	//m_iModelTextureDetail = clamp( pConfigKeys->GetInt( "setting.gpu_mem_level", 0 ), 0, 2 );
	//m_iPagedPoolMem = clamp( pConfigKeys->GetInt( "setting.mem_level", 0 ), 0, 2 );

	// can go from 0 - 5 actually, might add support in this menu for this later
	m_iModelDetail = clamp( pConfigKeys->GetInt( "setting.r_rootlod", 0 ), 0, 2 );
	m_iLOD = clamp( pConfigKeys->GetInt( "setting.m_iLOD", -1 ), -1, 0 );
	m_iTextureDetail = clamp( pConfigKeys->GetInt( "setting.mat_picmip", 0 ), -1, 4 );

	if ( pConfigKeys->GetBool( "setting.r_waterforcereflectentities", 1 ) )
	{
		if ( pConfigKeys->GetBool( "setting.r_waterforcereflectentities", 1 ) )
		{
			m_iWaterDetail = 2;
		}
		m_iWaterDetail = 1;
	}

	if ( pConfigKeys->GetBool( "setting.r_flashlightdepthtexture", 1 ) )
	{
		if ( pConfigKeys->GetBool( "setting.r_shadowrendertotexture", 1 ) )
		{
			m_iShadowDetail = 2;
		}
		m_iShadowDetail = 1;
	}

	m_nAASamples = pConfigKeys->GetInt( "setting.mat_antialias", 0 );
	m_nAAQuality = pConfigKeys->GetInt( "setting.mat_aaquality", 0 );
	m_iFiltering = pConfigKeys->GetInt( "setting.mat_forceaniso", 1 );
	m_bVSync = pConfigKeys->GetBool( "setting.mat_vsync", true );
	//m_bTripleBuffered = pConfigKeys->GetBool( "setting.mat_triplebuffered", false );
	//m_iGPUDetail = pConfigKeys->GetInt( "setting.gpu_level", 0 );
	//m_iCPUDetail = pConfigKeys->GetInt( "setting.cpu_level", 0 );
	//m_flFilmGrain = pConfigKeys->GetFloat( "setting.mat_grain_scale_override", 1.0f );
	m_iQueuedMode = pConfigKeys->GetInt( "setting.mat_queue_mode", -1 );
	m_iDXLevel = pConfigKeys->GetInt( "setting.mat_dxlevel", 95 );
	//m_iHDRLevel = pConfigKeys->GetInt( "setting.mat_hdr_level", 2 );

	pConfigKeys->deleteThis();

	return true;
}

void Video::OpenThirdPartyVideoCreditsDialog()
{
//	if (!m_OptionsSubVideoThirdPartyCreditsDlg.Get())
//	{
//		m_OptionsSubVideoThirdPartyCreditsDlg = new COptionsSubVideoThirdPartyCreditsDlg(GetVParent());
//	}
//	m_OptionsSubVideoThirdPartyCreditsDlg->Activate();
}

//=============================================================================
void Video::Activate( bool bRecommendedSettings )
{
	BaseClass::Activate();

	//if ( !bRecommendedSettings )
	//{
		SetupActivateData();
	//}
	/*else
	{
		if ( !SetupRecommendedActivateData() )
			return;
	}*/

	if ( m_drpAspectRatio )
	{
		switch ( m_iAspectRatio )
		{
		default:
		case 0:
			m_drpAspectRatio->SetCurrentSelection( "#GameUI_AspectNormal" );
			break;
		case 1:
			m_drpAspectRatio->SetCurrentSelection( "#GameUI_AspectWide16x9" );
			break;
		case 2:
			m_drpAspectRatio->SetCurrentSelection( "#GameUI_AspectWide16x10" );
			break;
		}

		FlyoutMenu *pFlyout = m_drpAspectRatio->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}

	if ( m_drpDisplayMode )
	{
		if ( m_bWindowed )
		{
			/*if ( m_bNoBorder )
			{
				m_drpDisplayMode->SetCurrentSelection( "#L4D360UI_VideoOptions_Windowed_NoBorder" );
			}
			else
			{*/
				m_drpDisplayMode->SetCurrentSelection( "#GameUI_Windowed" );
			//}
		}
		else
		{
			m_drpDisplayMode->SetCurrentSelection( "#GameUI_Fullscreen" );
		}

		FlyoutMenu *pFlyout = m_drpDisplayMode->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}

	if ( m_drpResolution )
	{
		FlyoutMenu *pFlyout = m_drpResolution->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}

	/*if ( m_drpLockMouse )
	{
		if ( m_bLockMouse )
		{
			m_drpLockMouse->SetCurrentSelection( "#L4D360UI_Enabled" );
		}
		else
		{
			m_drpLockMouse->SetCurrentSelection( "#L4D360UI_Disabled" );
		}

		FlyoutMenu *pFlyout = m_drpLockMouse->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}

	if ( m_sldFilmGrain )
	{
		if ( !bRecommendedSettings )
		{
			m_sldFilmGrain->Reset();
		}
		else
		{
			m_sldFilmGrain->SetCurrentValue( m_flFilmGrain );
			m_sldFilmGrain->ResetSliderPosAndDefaultMarkers();
		}
	}*/

	if ( m_drpModelDetail )
	{
		//switch ( m_iModelTextureDetail )
		switch ( m_iModelDetail )
		{
		case 0:
			m_drpModelDetail->SetCurrentSelection( "ModelDetailHigh" );
			break;
		case 1:
			m_drpModelDetail->SetCurrentSelection( "ModelDetailMedium" );
			break;
		case 2:
			m_drpModelDetail->SetCurrentSelection( "ModelDetailLow" );
			break;
		}

		FlyoutMenu *pFlyout = m_drpModelDetail->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}

	if ( m_drpLOD )
	{
		switch ( m_iLOD )
		{
		case -1:
			m_drpLOD->SetCurrentSelection( "LODEnabled" );
			break;
		case 0:
			m_drpLOD->SetCurrentSelection( "LODDisabled" );
			break;
		}

		FlyoutMenu *pFlyout = m_drpLOD->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}
	
	if ( m_drpTextureDetail )
	{
		switch ( m_iTextureDetail )
		{
		case -1:
			m_drpTextureDetail->SetCurrentSelection( "TextureDetailVeryHigh" );
			break;
		case 0:
			m_drpTextureDetail->SetCurrentSelection( "TextureDetailHigh" );
			break;
		case 1:
			m_drpTextureDetail->SetCurrentSelection( "TextureDetailMedium" );
			break;
		case 2:
			m_drpTextureDetail->SetCurrentSelection( "TextureDetailLow" );
			break;
		case 3:
			m_drpTextureDetail->SetCurrentSelection( "TextureDetailVeryLow" );
			break;
		case 4:
			m_drpTextureDetail->SetCurrentSelection( "TextureDetailLowest" );
			break;
		}

		FlyoutMenu *pFlyout = m_drpTextureDetail->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}

	if ( m_drpWaterDetail )
	{
		switch ( m_iWaterDetail )
		{
		case 0:
			m_drpWaterDetail->SetCurrentSelection( "WaterDetailLow" );
			break;
		case 1:
			m_drpWaterDetail->SetCurrentSelection( "WaterDetailMedium" );
			break;
		case 2:
			m_drpWaterDetail->SetCurrentSelection( "WaterDetailHigh" );
			break;
		}

		FlyoutMenu *pFlyout = m_drpWaterDetail->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}

	if ( m_drpShadowDetail )
	{
		switch ( m_iShadowDetail )
		{
		case 0:
			m_drpShadowDetail->SetCurrentSelection( "ShadowDetailLow" );
			break;
		case 1:
			m_drpShadowDetail->SetCurrentSelection( "ShadowDetailMedium" );
			break;
		case 2:
			m_drpShadowDetail->SetCurrentSelection( "ShadowDetailHigh" );
			break;
		}

		FlyoutMenu *pFlyout = m_drpShadowDetail->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}

	if ( m_drpAntialias )
	{
		FlyoutMenu *pFlyout = m_drpAntialias->GetCurrentFlyout();
		if ( pFlyout )
		{
			char szCurrentButton[ 32 ];
			Q_strncpy( szCurrentButton, VIDEO_ANTIALIAS_COMMAND_PREFIX, sizeof( szCurrentButton ) );

			int iCommandNumberPosition = Q_strlen( szCurrentButton );
			szCurrentButton[ iCommandNumberPosition + 1 ] = '\0';

			// We start with no entries
			m_nNumAAModes = 0;

			// Always have the possibility of no AA
			Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
			szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
			SetFlyoutButtonText( szCurrentButton, pFlyout, "#GameUI_None" );

			m_nAAModes[m_nNumAAModes].m_nNumSamples = 1;
			m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
			m_nNumAAModes++;

			// Add other supported AA settings
			if ( materials->SupportsMSAAMode(2) )
			{
				Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
				szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
				SetFlyoutButtonText( szCurrentButton, pFlyout, "#GameUI_2X" );

				m_nAAModes[m_nNumAAModes].m_nNumSamples = 2;
				m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
				m_nNumAAModes++;
			}

			if ( materials->SupportsMSAAMode(4) )
			{
				Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
				szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
				SetFlyoutButtonText( szCurrentButton, pFlyout, "#GameUI_4X" );

				m_nAAModes[m_nNumAAModes].m_nNumSamples = 4;
				m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
				m_nNumAAModes++;
			}

			if ( materials->SupportsMSAAMode(6) )
			{
				Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
				szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
				SetFlyoutButtonText( szCurrentButton, pFlyout, "#GameUI_6X" );

				m_nAAModes[m_nNumAAModes].m_nNumSamples = 6;
				m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
				m_nNumAAModes++;
			}

			if ( materials->SupportsCSAAMode(4, 2) )							// nVidia CSAA			"8x"
			{
				Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
				szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
				SetFlyoutButtonText( szCurrentButton, pFlyout, "#GameUI_8X_CSAA" );

				m_nAAModes[m_nNumAAModes].m_nNumSamples = 4;
				m_nAAModes[m_nNumAAModes].m_nQualityLevel = 2;
				m_nNumAAModes++;
			}

			if ( materials->SupportsCSAAMode(4, 4) )							// nVidia CSAA			"16x"
			{
				Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
				szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
				SetFlyoutButtonText( szCurrentButton, pFlyout, "#GameUI_16X_CSAA" );

				m_nAAModes[m_nNumAAModes].m_nNumSamples = 4;
				m_nAAModes[m_nNumAAModes].m_nQualityLevel = 4;
				m_nNumAAModes++;
			}

			if ( materials->SupportsMSAAMode(8) )
			{
				Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
				szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
				SetFlyoutButtonText( szCurrentButton, pFlyout, "#GameUI_8X" );

				m_nAAModes[m_nNumAAModes].m_nNumSamples = 8;
				m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
				m_nNumAAModes++;
			}

			if ( materials->SupportsCSAAMode(8, 2) )							// nVidia CSAA			"16xQ"
			{
				Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
				szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
				SetFlyoutButtonText( szCurrentButton, pFlyout, "#GameUI_16XQ_CSAA" );

				m_nAAModes[m_nNumAAModes].m_nNumSamples = 8;
				m_nAAModes[m_nNumAAModes].m_nQualityLevel = 2;
				m_nNumAAModes++;
			}

			// Change the height to fit the active items
			//pFlyout->SetBGTall( m_nNumAAModes * 20 + 5 );

			// Disable the remaining possible choices
			for ( int i = m_nNumAAModes; i <= 9; ++i )
			{
				szCurrentButton[ iCommandNumberPosition ] = i + '0';

				Button *pButton = pFlyout->FindChildButtonByCommand( szCurrentButton );
				if ( pButton )
				{
					pButton->SetVisible( false );
				}
			}

			// Select the currently set type
			m_iAntiAlias = FindMSAAMode( m_nAASamples, m_nAAQuality );
			szCurrentButton[ iCommandNumberPosition ] = m_iAntiAlias + '0';
			m_drpAntialias->SetCurrentSelection( szCurrentButton );

			pFlyout->SetListener( this );
		}
	}

	if ( m_drpFiltering )
	{
		switch ( m_iFiltering )
		{
		case 0:
			m_drpFiltering->SetCurrentSelection( "#GameUI_Bilinear" );
			break;
		case 1:
			m_drpFiltering->SetCurrentSelection( "#GameUI_Trilinear" );
			break;
		case 2:
			m_drpFiltering->SetCurrentSelection( "#GameUI_Anisotropic2X" );
			break;
		case 4:
			m_drpFiltering->SetCurrentSelection( "#GameUI_Anisotropic4X" );
			break;
		case 8:
			m_drpFiltering->SetCurrentSelection( "#GameUI_Anisotropic8X" );
			break;
		case 16:
			m_drpFiltering->SetCurrentSelection( "#GameUI_Anisotropic16X" );
			break;
		default:
			m_drpFiltering->SetCurrentSelection( "#GameUI_Trilinear" );
			m_iFiltering = 1;
			break;
		}

		FlyoutMenu *pFlyout = m_drpFiltering->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}

	if ( m_drpVSync )
	{
		if ( m_bVSync )
		{
			m_drpVSync->SetCurrentSelection( "VSyncEnabled" );
		}
		else
		{
			m_drpVSync->SetCurrentSelection( "VSyncDisabled" );
		}

		FlyoutMenu *pFlyout = m_drpVSync->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}

	if ( m_drpQueuedMode )
	{
		// Only allow the options on multi-processor machines.
		if ( GetCPUInformation()->m_nPhysicalProcessors >= 2 )
		{
			if ( m_iQueuedMode == 2 )
			{
				m_drpQueuedMode->SetCurrentSelection( "QueuedModeMulti" );
			}
			else if ( m_iQueuedMode == 0 )
			{
				m_drpQueuedMode->SetCurrentSelection( "QueuedModeDefault" );
			}
			else
			{
				m_drpQueuedMode->SetCurrentSelection( "QueuedModeSingle" );
			}

			FlyoutMenu *pFlyout = m_drpQueuedMode->GetCurrentFlyout();
			if ( pFlyout )
			{
				pFlyout->SetListener( this );
			}
		}
		else
		{
			m_drpQueuedMode->SetEnabled( false );
		}
	}

	if ( m_drpShaderDetail )
	{
		switch ( m_iShaderDetail )
		{
		case 0:
			m_drpShaderDetail->SetCurrentSelection( "ShaderDetailLow" );
			break;
		case 1:
			m_drpShaderDetail->SetCurrentSelection( "ShaderDetailMedium" );
			break;
		case 2:
			m_drpShaderDetail->SetCurrentSelection( "ShaderDetailHigh" );
			break;
		case 3:
			m_drpShaderDetail->SetCurrentSelection( "ShaderDetailVeryHigh" );
			break;
		}

		FlyoutMenu *pFlyout = m_drpShaderDetail->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}

	/*if ( m_drpCPUDetail )
	{
		switch ( m_iCPUDetail )
		{
		case 0:
			m_drpCPUDetail->SetCurrentSelection( "CPUDetailLow" );
			break;
		case 1:
			m_drpCPUDetail->SetCurrentSelection( "CPUDetailMedium" );
			break;
		case 2:
			m_drpCPUDetail->SetCurrentSelection( "CPUDetailHigh" );
			break;
		}

		FlyoutMenu *pFlyout = m_drpCPUDetail->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}*/

	if ( m_drpDXLevel )
	{
		switch ( m_iDXLevel )
		{
		case 98: // remove?
			m_drpDXLevel->SetCurrentSelection( "dxlevel98" );
			break;
		case 95:
			m_drpDXLevel->SetCurrentSelection( "dxlevel95" );
			break;
		case 90:
			m_drpDXLevel->SetCurrentSelection( "dxlevel90" );
			break;
		case 81:
			m_drpDXLevel->SetCurrentSelection( "dxlevel81" );
			break;
		case 80:
			m_drpDXLevel->SetCurrentSelection( "dxlevel80" );
			break;
		}

		FlyoutMenu *pFlyout = m_drpDXLevel->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}
	
	// replace this with auto grabbing the fieldName somehow
	if ( m_drpHDRLevel )
	{
		char command[256]; // length of command
		CGameUIConVarRef mat_hdr_level( "mat_hdr_level" );
		V_snprintf( command, 256, "engine mat_hdr_level %i", mat_hdr_level.GetInt() );

		m_drpHDRLevel->SetCurrentSelection( command );

		// somehow grab the command from the OnCommand Function, and use that command in this maybe
		// or find a different way to grab the command

		FlyoutMenu *pFlyout = m_drpHDRLevel->GetCurrentFlyout();
		if ( pFlyout )
		{
			pFlyout->SetListener( this );
		}
	}


	//UpdateFooter();
	
	//if ( !bRecommendedSettings )
	//{
		/*if ( m_drpAspectRatio )
		{
			if ( m_ActiveControl )
				m_ActiveControl->NavigateFrom( );
			m_drpAspectRatio->NavigateTo();
			m_ActiveControl = m_drpAspectRatio;
		}*/
	//}
}

// should be changed just to have any dropdownmenu activate
void Video::OnThink()
{
	BaseClass::OnThink();

	bool needsActivate = false;

	if( !m_drpAspectRatio )
	{
		m_drpAspectRatio = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpAspectRatio" ) );
		needsActivate = true;
	}

	if( !m_drpResolution )
	{
		m_drpResolution = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpResolution" ) );
		needsActivate = true;
	}

	if( !m_drpDisplayMode )
	{
		m_drpDisplayMode = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpDisplayMode" ) );
		needsActivate = true;
	}

	/*if ( !m_drpLockMouse )
	{
		m_drpLockMouse = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpLockMouse" ) );
		needsActivate = true;
	}
	
	if ( m_drpLockMouse )
	{
		m_drpLockMouse->SetVisible( m_bWindowed );
	}

	if( !m_sldFilmGrain )
	{
		m_sldFilmGrain = dynamic_cast< SliderControl* >( FindChildByName( "SldFilmGrain" ) );
		needsActivate = true;
	}

	if( !m_btnAdvanced )
	{
		m_btnAdvanced = dynamic_cast< BaseModHybridButton* >( FindChildByName( "BtnAdvanced" ) );
		needsActivate = true;
	}*/

	if( !m_drpModelDetail )
	{
		m_drpModelDetail = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpModelDetail" ) );
		needsActivate = true;
	}

	if( !m_drpLOD )
	{
		m_drpLOD = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpLOD" ) );
		needsActivate = true;
	}

	if( !m_drpTextureDetail )
	{
		m_drpTextureDetail = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpTextureDetail" ) );
		needsActivate = true;
	}

	if( !m_drpWaterDetail )
	{
		m_drpWaterDetail = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpWaterDetail" ) );
		needsActivate = true;
	}

	if( !m_drpShadowDetail )
	{
		m_drpShadowDetail = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpShadowDetail" ) );
		needsActivate = true;
	}

	/*if( !m_drpPagedPoolMem )
	{
		m_drpPagedPoolMem = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpPagedPoolMem" ) );
		needsActivate = true;
	}*/

	if( !m_drpAntialias )
	{
		m_drpAntialias = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpAntialias" ) );
		needsActivate = true;
	}

	if( !m_drpFiltering )
	{
		m_drpFiltering = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpFiltering" ) );
		needsActivate = true;
	}

	if( !m_drpVSync )
	{
		m_drpVSync = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpVSync" ) );
		needsActivate = true;
	}

	if( !m_drpQueuedMode )
	{
		m_drpQueuedMode = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpQueuedMode" ) );
		needsActivate = true;
	}

	if( !m_drpShaderDetail )
	{
		m_drpShaderDetail = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpShaderDetail" ) );
		needsActivate = true;
	}

	/*if( !m_drpCPUDetail )
	{
		m_drpCPUDetail = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpCPUDetail" ) );
		needsActivate = true;
	}*/

	if( !m_drpDXLevel )
	{
		m_drpDXLevel = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpDXLevel" ) );
		needsActivate = true;
	}
	
	if( !m_drpHDRLevel )
	{
		m_drpHDRLevel = dynamic_cast< DropDownMenu* >( FindChildByName( "DrpHDRLevel" ) );

		needsActivate = true;
	}

	/*if( FindChildByControl( "DropDownMenu" ) )
	{
		for ( int i = 0; i < GetChildCount(); i++ )
		{
			// ALWAYS returns the same value, need to fix that
			//int index = FindChildIndexByControl( "DropDownMenu" );

			Panel *pChild = GetChild(i);
			if (!pChild)
				continue;
			
			const char *selectedPanel = FindFieldName( i );

			// supposed to grab the fieldName and make it into a pointer for DropDownMenu
			// somehow use the panel id and make that into a new DropDownMenu object
			new DropDownMenu* [i];
			const DropDownMenu* test[i] = reinterpret_cast< DropDownMenu* >( i );

			// static_cast is const void, not char

			drpCurrent = dynamic_cast< DropDownMenu* >( FindChildByName( selectedPanel ) );

			if ( !V_stricmp( FindFieldName( i ), selectedPanel ) )
			{
				// get index of drop down panel?
				drpCurrent = dynamic_cast< DropDownMenu* >( FindChildByName( selectedPanel ) );
			}

			// add something here to pause the for loop when i is equal to GetChildCount()
		}*/


			/*if ( V_stricmp( FindFieldName( FindChildIndexByControl( "DropDownMenu" ) ), "DrpHDRLevel" ) )
			{
				const char *selectedPanel = FindFieldName( FindChildIndexByControl( "DropDownMenu" ) );

				// get index of drop down panel?
				m_drpHDRLevel = dynamic_cast< DropDownMenu* >( FindChildByName( selectedPanel ) );
			}*/
			//KeyValues *inResourceData;

			//inResourceData->GetString( "fieldName", "" );

			//const char *fieldName = inResourceData->GetString( "fieldName", "" );
			
			//vgui::Panel *panel = m_drpCurrent->FindChildByName( selectedPanel );
			//m_drpCurrent = FindChildByName( selectedPanel );

			//m_drpHDRLevel = dynamic_cast< DropDownMenu* >( FindChildByName( selectedPanel ) );
		//}
		//needsActivate = true;
	//}

	if( !m_btnUseRecommended )
	{
		m_btnUseRecommended = dynamic_cast< BaseModHybridButton* >( FindChildByName( "BtnUseRecommended" ) );
		needsActivate = true;
	}
// 
// 	if( !m_btnCancel )
// 	{
// 		m_btnCancel = dynamic_cast< BaseModHybridButton* >( FindChildByName( "BtnCancel" ) );
// 		needsActivate = true;
// 	}

// 	if( !m_btnDone )
// 	{
// 		m_btnDone = dynamic_cast< BaseModHybridButton* >( FindChildByName( "BtnDone" ) );
// 		needsActivate = true;
// 	}

	if( !m_btn3rdPartyCredits )
	{
		m_btn3rdPartyCredits = dynamic_cast< BaseModHybridButton* >( FindChildByName( "Btn3rdPartyCredits" ) );
		needsActivate = true;
	}

	if( needsActivate )
	{
		Activate();
	}
}

void Video::OnKeyCodePressed(KeyCode code)
{
	int joystick = GetJoystickForCode( code );
	int userId = CBaseModPanel::GetSingleton().GetLastActiveUserId();
	if ( joystick != userId || joystick < 0 )
	{	
		return;
	}

	switch ( GetBaseButtonCode( code ) )
	{
	case KEY_XBUTTON_B:
		// nav back
		BaseClass::OnKeyCodePressed(code);
		break;

	default:
		BaseClass::OnKeyCodePressed(code);
		break;
	}
}

//=============================================================================
void Video::OnCommand(const char *command)
{
	if ( !Q_strcmp( command, "#GameUI_AspectNormal" ) )
	{
		m_iAspectRatio = 0;
		m_bDirtyValues = true;
		PrepareResolutionList();
	}
	else if ( !Q_strcmp( command, "#GameUI_AspectWide16x9" ) )
	{
		m_iAspectRatio = 1;
		m_bDirtyValues = true;
		PrepareResolutionList();
	}
	else if ( !Q_strcmp( command, "#GameUI_AspectWide16x10" ) )
	{
		m_iAspectRatio = 2;
		m_bDirtyValues = true;
		PrepareResolutionList();
	}
	else if ( StringHasPrefix( command, VIDEO_RESLUTION_COMMAND_PREFIX ) )
	{
		int iCommandNumberPosition = Q_strlen( VIDEO_RESLUTION_COMMAND_PREFIX );
		int iResolution = clamp( command[ iCommandNumberPosition ] - '0', 0, m_nNumResolutionModes - 1 );

		m_iResolutionWidth = m_nResolutionModes[ iResolution ].m_nWidth;
		m_iResolutionHeight = m_nResolutionModes[ iResolution ].m_nHeight;

		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "#GameUI_Windowed" ) )
	{
		m_bWindowed = true;
		//m_bNoBorder = false;
		m_bDirtyValues = true;
		PrepareResolutionList();
	}
	/*else if ( !Q_strcmp( command, "#L4D360UI_VideoOptions_Windowed_NoBorder" ) )
	{
		m_bWindowed = true;
		m_bNoBorder = true;
		m_bDirtyValues = true;
		PrepareResolutionList();
	}*/
	else if ( !Q_strcmp( command, "#GameUI_Fullscreen" ) )
	{
		m_bWindowed = false;
		//m_bNoBorder = false;
		m_bDirtyValues = true;
		PrepareResolutionList();
	}
	/*else if ( !Q_strcmp( command, "LockMouseEnabled" ) )
	{
		m_bLockMouse = true;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "LockMouseDisabled" ) )
	{
		m_bLockMouse = false;
		m_bDirtyValues = true;
	}*/
	/*else if ( !Q_strcmp( command, "ShowAdvanced" ) )
	{
		SetControlVisible( "BtnAdvanced", false );
		SetControlVisible( "DrpModelDetail", true );
		SetControlVisible( "DrpPagedPoolMem", true );
		SetControlVisible( "DrpFiltering", true );
		SetControlVisible( "DrpVSync", true );
		SetControlVisible( "DrpQueuedMode", true );
		SetControlVisible( "DrpShaderDetail", true );
		SetControlVisible( "DrpCPUDetail", true );

		if ( m_drpAntialias )
		{
			m_drpAntialias->SetVisible( true );
			m_drpAntialias->NavigateTo();
		}

		if ( m_btnUseRecommended )
		{
			int iXPos, iYPos;
			m_btnUseRecommended->GetPos( iXPos, iYPos );
			m_btnUseRecommended->SetPos( iXPos, iBtnUseRecommendedYPos );
		}

		if ( m_btnCancel )
		{
			int iXPos, iYPos;
			m_btnCancel->GetPos( iXPos, iYPos );
			m_btnCancel->SetPos( iXPos, iBtnCancelYPos );
		}

		if ( m_btnDone )
		{
			int iXPos, iYPos;
			m_btnDone->GetPos( iXPos, iYPos );
			m_btnDone->SetPos( iXPos, iBtnDoneYPos );
		}

		FlyoutMenu::CloseActiveMenu();
	}*/

	else if ( !Q_strcmp( command, "ModelDetailHigh" ) )
	{
		//m_iModelTextureDetail = 2;
		m_iModelDetail = 0;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "ModelDetailMedium" ) )
	{
		m_iModelDetail = 1;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "ModelDetailLow" ) )
	{
		m_iModelDetail = 2;
		m_bDirtyValues = true;
	}

	else if ( !Q_strcmp( command, "LODEnabled" ) )
	{
		m_iLOD = -1;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "LODDisabled" ) )
	{
		m_iLOD = 0;
		m_bDirtyValues = true;
	}

	else if ( !Q_strcmp( command, "TextureDetailVeryHigh" ) )
	{
		m_iTextureDetail = -1;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "TextureDetailHigh" ) )
	{
		m_iTextureDetail = 0;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "TextureDetailMedium" ) )
	{
		m_iTextureDetail = 1;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "TextureDetailLow" ) )
	{
		m_iTextureDetail = 2;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "TextureDetailVeryLow" ) )
	{
		m_iTextureDetail = 3;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "TextureDetailLowest" ) )
	{
		m_iTextureDetail = 4;
		m_bDirtyValues = true;
	}

	else if ( !Q_strcmp( command, "WaterDetailHigh" ) )
	{
		m_iWaterDetail = 2;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "WaterDetailMedium" ) )
	{
		m_iWaterDetail = 1;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "WaterDetailLow" ) )
	{
		m_iWaterDetail = 0;
		m_bDirtyValues = true;
	}

	else if ( !Q_strcmp( command, "ShadowDetailHigh" ) )
	{
		m_iShadowDetail = 2;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "ShadowDetailMedium" ) )
	{
		m_iShadowDetail = 1;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "ShadowDetailLow" ) )
	{
		m_iShadowDetail = 0;
		m_bDirtyValues = true;
	}

	/*else if ( !Q_strcmp( command, "PagedPoolMemHigh" ) )
	{
		m_iPagedPoolMem = 2;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "PagedPoolMemMedium" ) )
	{
		m_iPagedPoolMem = 1;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "PagedPoolMemLow" ) )
	{
		m_iPagedPoolMem = 0;
		m_bDirtyValues = true;
	}*/
	else if ( StringHasPrefix( command, VIDEO_ANTIALIAS_COMMAND_PREFIX ) )
	{
		int iCommandNumberPosition = Q_strlen( VIDEO_ANTIALIAS_COMMAND_PREFIX );
		m_iAntiAlias = clamp( command[ iCommandNumberPosition ] - '0', 0, m_nNumAAModes - 1 );
		m_bDirtyValues = true;
	}

	else if ( !Q_strcmp( command, "#GameUI_Bilinear" ) )
	{
		m_iFiltering = 0;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "#GameUI_Trilinear" ) )
	{
		m_iFiltering = 1;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "#GameUI_Anisotropic2X" ) )
	{
		m_iFiltering = 2;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "#GameUI_Anisotropic4X" ) )
	{
		m_iFiltering = 4;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "#GameUI_Anisotropic8X" ) )
	{
		m_iFiltering = 8;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "#GameUI_Anisotropic16X" ) )
	{
		m_iFiltering = 16;
		m_bDirtyValues = true;
	}

	else if ( Q_strcmp( "VSyncEnabled", command ) == 0 )
	{
		m_bVSync = true;
		m_bDirtyValues = true;
	}
	else if ( Q_strcmp( "VSyncDisabled", command ) == 0 )
	{
		m_bVSync = false;
		m_bDirtyValues = true;
	}

	else if ( !Q_strcmp( command, "QueuedModeMulti" ) )
	{
		m_iQueuedMode = 2;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "QueuedModeDefault" ) )
	{
		m_iQueuedMode = -1;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "QueuedModeSingle" ) )
	{
		m_iQueuedMode = 0;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "ShaderDetailVeryHigh" ) )
	{
		m_iShaderDetail = 3;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "ShaderDetailHigh" ) )
	{
		m_iShaderDetail = 2;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "ShaderDetailMedium" ) )
	{
		m_iShaderDetail = 1;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "ShaderDetailLow" ) )
	{
		m_iShaderDetail = 0;
		m_bDirtyValues = true;
	}
	/*else if ( !Q_strcmp( command, "CPUDetailHigh" ) )
	{
		m_iCPUDetail = 2;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "CPUDetailMedium" ) )
	{
		m_iCPUDetail = 1;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "CPUDetailLow" ) )
	{
		m_iCPUDetail = 0;
		m_bDirtyValues = true;
	}*/

	else if ( !Q_strcmp( command, "dxlevel98" ) )
	{
		m_iDXLevel = 98;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "dxlevel95" ) )
	{
		m_iDXLevel = 95;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "dxlevel90" ) )
	{
		m_iDXLevel = 90;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "dxlevel81" ) )
	{
		m_iDXLevel = 81;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "dxlevel80" ) )
	{
		m_iDXLevel = 80;
		m_bDirtyValues = true;
	}

	// float hdr,
	/*else if ( !Q_strcmp( command, "HDRLevel3" ) )
	{
		m_iHDRLevel = 3;
		m_bDirtyValues = true;
	}*/
	/*else if ( !Q_strcmp( command, "HDRLevel2" ) )
	{
		m_iHDRLevel = 2;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "HDRLevel1" ) )
	{
		m_iHDRLevel = 2;
		m_bDirtyValues = true;
	}
	else if ( !Q_strcmp( command, "HDRLevel0" ) )
	{
		m_iHDRLevel = 2;
		m_bDirtyValues = true;
	}*/

	else if( Q_stricmp( "UseRecommended", command ) == 0 )
	{
		FlyoutMenu::CloseActiveMenu();
		Activate( true );
	}
	else if( Q_stricmp( "Cancel", command ) == 0 )
	{
		m_bDirtyValues = false;
		//OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
		//FlyoutMenu::CloseActiveMenu();
		Close(); // closes the panel
	}
	else if( Q_stricmp( "Apply", command ) == 0 )
	{
		//OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
		FlyoutMenu::CloseActiveMenu();
		ApplyChanges();
	}
	else if( Q_stricmp( "Back", command ) == 0 )
	{
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
	}
	else if( Q_stricmp( "3rdPartyCredits", command ) == 0 )
	{
		OpenThirdPartyVideoCreditsDialog();
		FlyoutMenu::CloseActiveMenu();
	}
	else if (Q_stricmp(command, "engine "))
	{
		const char *engineCMD = strstr(command, "engine ") + strlen("engine ");
		if (strlen(engineCMD) > 0)
		{
			//engine->ClientCmd_Unrestricted(command);
			engine->ClientCmd_Unrestricted( const_cast<char *> (engineCMD));
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

int Video::FindMSAAMode( int nAASamples, int nAAQuality )
{
	// Run through the AA Modes supported by the device
	for ( int nAAMode = 0; nAAMode < m_nNumAAModes; nAAMode++ )
	{
		// If we found the mode that matches what we're looking for, return the index
		if ( ( m_nAAModes[nAAMode].m_nNumSamples == nAASamples) && ( m_nAAModes[nAAMode].m_nQualityLevel == nAAQuality) )
		{
			return nAAMode;
		}
	}

	return 0;	// Didn't find what we're looking for, so no AA
}

static vmode_t s_pWindowedModes[] = 
{
	// NOTE: These must be sorted by ascending width, then ascending height
	{ 640, 480, 32, 60 },
	{ 852, 480, 32, 60 },
	{ 1280, 720, 32, 60 },
	{ 1920, 1080, 32, 60 },
};

static void GenerateWindowedModes( CUtlVector< vmode_t > &windowedModes, int nCount, vmode_t *pFullscreenModes )
{
	int nFSMode = 0;
	for ( int i = 0; i < ARRAYSIZE( s_pWindowedModes ); ++i )
	{
		while ( true )
		{
			if ( nFSMode >= nCount )
				break;

			if ( pFullscreenModes[nFSMode].width > s_pWindowedModes[i].width )
				break;

			if ( pFullscreenModes[nFSMode].width == s_pWindowedModes[i].width )
			{
				if ( pFullscreenModes[nFSMode].height > s_pWindowedModes[i].height )
					break;

				if ( pFullscreenModes[nFSMode].height == s_pWindowedModes[i].height )
				{
					// Don't add the matching fullscreen mode
					++nFSMode;
					break;
				}
			}

			windowedModes.AddToTail( pFullscreenModes[nFSMode] );
			++nFSMode;
		}

		windowedModes.AddToTail( s_pWindowedModes[i] );
	}

	for ( ; nFSMode < nCount; ++nFSMode )
	{
		windowedModes.AddToTail( pFullscreenModes[nFSMode] );
	}
}

// Check to see if a mode is already in the resolution list. This unfortunately
// makes PrepareResolutionList() quadratic, but it's run infrequently and it's
// a bit late in the project to mess with the source of the bug in
// GenerateWindowedModes(). 
// Fixes bugbait 30693 
static bool HasResolutionMode( const ResolutionMode_t * RESTRICT pModes, const int numModes, const vmode_t * RESTRICT pTestMode )
{
	for ( int i = 0 ; i < numModes ; ++i )
	{
		if ( pModes[ i ].m_nWidth  == pTestMode->width   &&
			 pModes[ i ].m_nHeight == pTestMode->height )
		{
			return true;
		}
	}

	return false;
}

void Video::PrepareResolutionList()
{
	int iOverflowPosition = 1;
	m_nNumResolutionModes = 0;

	if ( !m_drpResolution )
		return;

	FlyoutMenu *pResolutionFlyout = m_drpResolution->GetCurrentFlyout();

	if ( !pResolutionFlyout )
		return;

	// Set up the base string for each button command
	char szCurrentButton[ 32 ];
	Q_strncpy( szCurrentButton, VIDEO_RESLUTION_COMMAND_PREFIX, sizeof( szCurrentButton ) );

	int iCommandNumberPosition = Q_strlen( szCurrentButton );
	szCurrentButton[ iCommandNumberPosition + 1 ] = '\0';

	// get full video mode list
	vmode_t *plist = NULL;
	int count = 0;
	gameuifuncs->GetVideoModes( &plist, &count );

	int desktopWidth, desktopHeight;
	gameuifuncs->GetDesktopResolution( desktopWidth, desktopHeight );

	// Add some extra modes in if we're running windowed
	CUtlVector< vmode_t > windowedModes;
	if ( m_bWindowed )
	{
		GenerateWindowedModes( windowedModes, count, plist );
		count = windowedModes.Count();
		plist = windowedModes.Base();
	}

	// iterate all the video modes adding them to the dropdown
	bool bFoundWidescreen = false;
	for ( int i = 0; i < count; i++, plist++ )
	{
		// don't show modes bigger than the desktop for windowed mode
		if ( m_bWindowed && ( plist->width > desktopWidth || plist->height > desktopHeight ) )
			continue;

		// skip a mode if it is somehow already in the list (bug 30693)
		if ( HasResolutionMode( m_nResolutionModes, m_nNumResolutionModes, plist ) )
			continue;

		char sz[ 256 ];
		GetResolutionName( plist, sz, sizeof( sz ) );

		int iAspectMode = GetScreenAspectMode( plist->width, plist->height );

		if ( iAspectMode > 0 )
		{
			bFoundWidescreen = true;

			if ( m_drpAspectRatio )
			{
				FlyoutMenu *pFlyout = m_drpAspectRatio->GetCurrentFlyout();
				if ( pFlyout )
				{
					Button *pButton = NULL;

					switch ( iAspectMode )
					{
					default:
					case 0:
						pButton = pFlyout->FindChildButtonByCommand( "#GameUI_AspectNormal" );
						break;
					case 1:
						pButton = pFlyout->FindChildButtonByCommand( "#GameUI_AspectWide16x9" );
						break;
					case 2:
						pButton = pFlyout->FindChildButtonByCommand( "#GameUI_AspectWide16x10" );
						break;
					}

					if ( pButton )
					{
						pButton->SetEnabled( true );
					}
				}
			}
		}

		// filter the list for those matching the current aspect
		if ( iAspectMode == m_iAspectRatio )
		{
			if ( m_nNumResolutionModes == MAX_DYNAMIC_VIDEO_MODES )
			{
				// No more will fit in this drop down, and it's too late in the product to make this a scrollable list
				// Instead lets make the list less fine grained by removing middle entries
				if ( iOverflowPosition >= MAX_DYNAMIC_VIDEO_MODES )
				{
					// Wrap the second entry
					iOverflowPosition = 1;
				}

				int iShiftPosition = iOverflowPosition;

				while ( iShiftPosition < MAX_DYNAMIC_VIDEO_MODES - 1 )
				{
					// Copy the entry in front of us over this entry
					szCurrentButton[ iCommandNumberPosition ] = ( iShiftPosition + 1 ) + '0';

					Button *pButton = pResolutionFlyout->FindChildButtonByCommand( szCurrentButton );
					if ( pButton )
					{
						char szResName[ 256 ];
						pButton->GetText( szResName, sizeof(szResName) );

						szCurrentButton[ iCommandNumberPosition ] = iShiftPosition + '0';
						SetFlyoutButtonText( szCurrentButton, pResolutionFlyout, szResName );
					}

					m_nResolutionModes[ iShiftPosition ].m_nWidth = m_nResolutionModes[ iShiftPosition + 1 ].m_nWidth;
					m_nResolutionModes[ iShiftPosition ].m_nHeight = m_nResolutionModes[ iShiftPosition + 1 ].m_nHeight;

					iShiftPosition++;
				}

				iOverflowPosition += 2;
				m_nNumResolutionModes--;
			}

			szCurrentButton[ iCommandNumberPosition ] = m_nNumResolutionModes + '0';
			SetFlyoutButtonText( szCurrentButton, pResolutionFlyout, sz );

			m_nResolutionModes[ m_nNumResolutionModes ].m_nWidth = plist->width;
			m_nResolutionModes[ m_nNumResolutionModes ].m_nHeight = plist->height;

			m_nNumResolutionModes++;
		}
	}

	// Change the height to fit the active items
	//pResolutionFlyout->SetBGTall( m_nNumResolutionModes * 20 + 5 );


	// Disable the remaining possible choices
	for ( int i = m_nNumResolutionModes; i < MAX_DYNAMIC_VIDEO_MODES; ++i )
	{
		szCurrentButton[ iCommandNumberPosition ] = i + '0';

		Button *pButton = pResolutionFlyout->FindChildButtonByCommand( szCurrentButton );
		if ( pButton )
		{
			pButton->SetVisible( false );
		}
	}

	// Find closest selection
	int selectedItemID;

	for ( selectedItemID = 0; selectedItemID < m_nNumResolutionModes; ++selectedItemID )
	{
		if ( m_nResolutionModes[ selectedItemID ].m_nHeight > m_iResolutionHeight )
			break;

		if (( m_nResolutionModes[ selectedItemID ].m_nHeight == m_iResolutionHeight ) &&
			( m_nResolutionModes[ selectedItemID ].m_nWidth > m_iResolutionWidth ))
			break;
	}

	// Go back to the one that matched or was smaller (and prevent -1 when none are smaller)
	selectedItemID = MAX( selectedItemID - 1, 0 );

	// Select the currently set type
	szCurrentButton[ iCommandNumberPosition ] = selectedItemID + '0';
	m_drpResolution->SetCurrentSelection( szCurrentButton );

	m_iResolutionWidth = m_nResolutionModes[ selectedItemID ].m_nWidth;
	m_iResolutionHeight = m_nResolutionModes[ selectedItemID ].m_nHeight;
}

void Video::ApplyChanges()
{
	/*if ( !m_bDirtyValues )
	{
		// Revert slider
		CGameUIConVarRef mat_grain_scale_override( "mat_grain_scale_override" );
		mat_grain_scale_override.SetValue( m_flFilmGrainInitialValue );

		// No need to apply settings
		return;
	}

	CGameUIConVarRef gpu_mem_level( "gpu_mem_level" );
	gpu_mem_level.SetValue( m_iModelTextureDetail );

	CGameUIConVarRef mem_level( "mem_level" );
	mem_level.SetValue( m_iPagedPoolMem );*/

	CGameUIConVarRef r_rootlod( "r_rootlod" );
	r_rootlod.SetValue( m_iModelDetail );
	CGameUIConVarRef r_lod( "r_lod" );
	r_lod.SetValue( m_iLOD );

	CGameUIConVarRef mat_picmip( "mat_picmip" );
	mat_picmip.SetValue( m_iTextureDetail );

	CGameUIConVarRef r_waterforceexpensive( "r_waterforceexpensive" );
	CGameUIConVarRef r_waterforcereflectentities( "r_waterforcereflectentities" );

	if ( m_iWaterDetail == 0 )
	{
		r_waterforcereflectentities.SetValue( 0 );
		r_waterforceexpensive.SetValue( 0 );
	}

	if ( m_iWaterDetail == 1 )
	{
		r_waterforcereflectentities.SetValue( 0 );
		r_waterforceexpensive.SetValue( 1 );
	}

	if ( m_iWaterDetail == 2 )
	{
		r_waterforcereflectentities.SetValue( 1 );
		r_waterforceexpensive.SetValue( 1 );
	}


	CGameUIConVarRef r_shadowrendertotexture( "r_shadowrendertotexture" );
	CGameUIConVarRef r_flashlightdepthtexture( "r_flashlightdepthtexture" );

	if ( m_iShadowDetail == 0 )
	{
		r_shadowrendertotexture.SetValue( 0 );
		r_flashlightdepthtexture.SetValue( 0 );
	}

	if ( m_iShadowDetail == 1 )
	{
		r_shadowrendertotexture.SetValue( 1 );
		r_flashlightdepthtexture.SetValue( 0 );
	}

	if ( m_iShadowDetail == 2 )
	{
		r_shadowrendertotexture.SetValue( 1 );
		r_flashlightdepthtexture.SetValue( 1 );
	}

	CGameUIConVarRef mat_antialias( "mat_antialias" );
	CGameUIConVarRef mat_aaquality( "mat_aaquality" );
	mat_antialias.SetValue( m_nAAModes[ m_iAntiAlias ].m_nNumSamples );
	mat_aaquality.SetValue( m_nAAModes[ m_iAntiAlias ].m_nQualityLevel );

	CGameUIConVarRef mat_forceaniso( "mat_forceaniso" );
	mat_forceaniso.SetValue( m_iFiltering );

	CGameUIConVarRef mat_vsync( "mat_vsync" );
	mat_vsync.SetValue( m_bVSync );

	//CGameUIConVarRef mat_triplebuffered( "mat_triplebuffered" );
	//mat_triplebuffered.SetValue( m_bTripleBuffered );

	CGameUIConVarRef mat_queue_mode( "mat_queue_mode" );
	mat_queue_mode.SetValue( m_iQueuedMode );

	//CGameUIConVarRef cpu_level( "cpu_level" );
	//cpu_level.SetValue( m_iCPUDetail );

	CGameUIConVarRef gpu_level( "gpu_level" );
	gpu_level.SetValue( m_iShaderDetail );

	//CGameUIConVarRef in_lock_mouse_to_window( "in_lock_mouse_to_window" );
	//in_lock_mouse_to_window.SetValue( m_bLockMouse );*/

	// Make sure there is a resolution change
	const MaterialSystem_Config_t &config = materials->GetCurrentConfigForVideoCard();
	if ( config.m_VideoMode.m_Width != m_iResolutionWidth || 
		 config.m_VideoMode.m_Height != m_iResolutionHeight || 
		 config.Windowed() != m_bWindowed/* ||
		 config.NoWindowBorder() != m_bNoBorder*/ )
	{
		// set mode
		char szCmd[ 256 ];
		//Q_snprintf( szCmd, sizeof( szCmd ), "mat_setvideomode %i %i %i %i\n", m_iResolutionWidth, m_iResolutionHeight, m_bWindowed ? 1 : 0, m_bNoBorder ? 1 : 0 );
		Q_snprintf( szCmd, sizeof( szCmd ), "mat_setvideomode %i %i %i \n", m_iResolutionWidth, m_iResolutionHeight, m_bWindowed ? 1 : 0 );
		engine->ClientCmd_Unrestricted( szCmd );
	}

	CGameUIConVarRef mat_dxlevel( "mat_dxlevel" );
	mat_dxlevel.SetValue( m_iDXLevel );

	CGameUIConVarRef mat_hdr_level( "mat_hdr_level" );
	mat_hdr_level.SetValue( mat_hdr_level.GetInt() );

	// apply changes
	engine->ClientCmd_Unrestricted( "mat_savechanges\n" );

	engine->ClientCmd_Unrestricted( "host_writeconfig" );
	m_bDirtyValues = false;

	// Update the current video config file.
#ifdef _X360
	AssertMsg( false, "VideoCFG is not supported on 360." );
#else
	/*
	int nAspectRatioMode = GetScreenAspectMode( config.m_VideoMode.m_Width, config.m_VideoMode.m_Height );
	UpdateCurrentVideoConfig( config.m_VideoMode.m_Width, config.m_VideoMode.m_Height, nAspectRatioMode, !config.Windowed(), config.NoWindowBorder() );
	*/
#endif
}

void Video::OnNotifyChildFocus( vgui::Panel* child )
{
}

void Video::UpdateFooter()
{
	CBaseModFooterPanel *footer = BaseModUI::CBaseModPanel::GetSingleton().GetFooterPanel();
	if  ( footer )
	{
		footer->SetButtons( FB_ABUTTON | FB_BBUTTON, FF_AB_ONLY, IsPC() ? true : false );
		footer->SetButtonText( FB_ABUTTON, "#L4D360UI_Select" );
		footer->SetButtonText( FB_BBUTTON, "#L4D360UI_Controller_Done" );
	}
}

void Video::OnFlyoutMenuClose( vgui::Panel* flyTo )
{
	UpdateFooter();

	/*if ( m_drpPagedPoolMem )
	{
		m_drpPagedPoolMem->SetOpenCallback( PagedPoolMemOpenned );
	}*/
}

void Video::OnFlyoutMenuCancelled()
{
}

//=============================================================================
Panel* Video::NavigateBack()
{
	ApplyChanges();

	return BaseClass::NavigateBack();
}

void Video::PaintBackground()
{
	//bool bIsAdvanced = ( m_btnAdvanced && !m_btnAdvanced->IsVisible() );
	//BaseClass::DrawDialogBackground( bIsAdvanced ? "#GameUI_VideoAdvanced_Title" : "#GameUI_Video", NULL, "#L4D360UI_AudioVideo_Desc", NULL, NULL, true );
}

void Video::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// required for new style
	SetPaintBackgroundEnabled( true );
	SetupAsDialogStyle();

	LoadControlSettings("resource/ui/settings/video.res");

	// Collapse the buttons under the advance options button
	/*int iAdvancedPos = 0;

	if ( m_btnAdvanced )
	{
		int iXPos;
		m_btnAdvanced->GetPos( iXPos, iAdvancedPos );
	}

	int iButtonSpacing = vgui::scheme()->GetProportionalScaledValue( 20 );
	int iNextButtonPos = iAdvancedPos + iButtonSpacing;

	if ( m_btnUseRecommended )
	{
		int iXPos;
		m_btnUseRecommended->GetPos( iXPos, iBtnUseRecommendedYPos );
		m_btnUseRecommended->SetPos( iXPos, iNextButtonPos );

		iNextButtonPos += iButtonSpacing;
	}

	if ( m_btnCancel )
	{
		int iXPos;
		m_btnCancel->GetPos( iXPos, iBtnCancelYPos );
		m_btnCancel->SetPos( iXPos, iNextButtonPos );

		iNextButtonPos += iButtonSpacing;
	}

	if ( m_btnDone )
	{
		int iXPos;
		m_btnDone->GetPos( iXPos, iBtnDoneYPos );
		m_btnDone->SetPos( iXPos, iNextButtonPos );

		iNextButtonPos += iButtonSpacing;
	}*/
}
