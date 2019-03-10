#include "cbase.h"
#include "nb_header_footer.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/ImagePanel.h"
#include <vgui/ISurface.h>
#include "vgui_video_player.h"
#include "VGUIMatSurface/IMatSystemSurface.h"
#include "engineinterface.h"
#include "BaseModPanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace VideoPlaybackFlags;
using namespace VideoSystem;

CASW_Background_Movie *g_pBackgroundMovie = NULL;

CASW_Background_Movie* ASWBackgroundMovie()
{
	if(!g_pBackgroundMovie)
		g_pBackgroundMovie = new CASW_Background_Movie();
	return g_pBackgroundMovie;
}

CASW_Background_Movie::CASW_Background_Movie()
{
	m_pVideoMaterial = NULL;
	m_nBIKMaterial = BIKMATERIAL_INVALID;
	m_nTextureID = -1;
	m_szCurrentMovie[0] = 0;
	m_nLastGameState = -1;
}

CASW_Background_Movie::~CASW_Background_Movie()
{

}

void CASW_Background_Movie::SetCurrentMovie(const char *szFilename)
{
	ClearCurrentMovie();

	m_pVideoMaterial = g_pVideo->CreateVideoMaterial("BackgroundMaterial", szFilename, "GAME", NO_AUDIO | LOOP_VIDEO, DETERMINE_FROM_FILE_EXTENSION, true);
	if ( !m_pVideoMaterial )
		return;
	int nWidth, nHeight;
	m_pVideoMaterial->GetVideoImageSize(&nWidth, &nHeight);
	m_pVideoMaterial->GetVideoTexCoordRange(&m_flU, &m_flV);

	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	float flFrameRatio = ( (float) wide / (float) tall );
	float flVideoRatio = ( (float) nWidth / (float) nHeight );

	if ( flVideoRatio > flFrameRatio )
	{
		m_nPlaybackWidth = wide;
		m_nPlaybackHeight = ( wide / flVideoRatio );
	}
	else if ( flVideoRatio < flFrameRatio )
	{
		m_nPlaybackWidth = ( tall * flVideoRatio );
		m_nPlaybackHeight = tall;
	}
	else
	{
		m_nPlaybackWidth = wide;
		m_nPlaybackHeight = tall;
	}

	Q_snprintf(m_szCurrentMovie, sizeof(m_szCurrentMovie), "%s", szFilename);
}

void CASW_Background_Movie::ClearCurrentMovie()
{
	if(m_pVideoMaterial != NULL) {
		// FIXME: Make sure the m_pMaterial is actually destroyed at this point!
		m_pVideoMaterial->StopVideo();
		g_pVideo->DestroyVideoMaterial(m_pVideoMaterial);
		m_pVideoMaterial = NULL;
		g_pMatSystemSurface->DeleteTextureByID(m_nTextureID);
		g_pMatSystemSurface->DestroyTextureID(m_nTextureID);
		m_nTextureID = -1;
	}
}

int CASW_Background_Movie::SetTextureMaterial()
{
	if(m_pVideoMaterial == NULL)
		return -1;

	if(m_nTextureID == -1)
		m_nTextureID = g_pMatSystemSurface->CreateNewTextureID(true);

	g_pMatSystemSurface->DrawSetTextureMaterial(m_nTextureID, m_pVideoMaterial->GetMaterial());
	return m_nTextureID;
}

void CASW_Background_Movie::Update()
{
	//	if ( engine->IsConnected() && ASWGameRules() )
	int nGameState = 0;

	// if you can, move this to a resource file or something
	if(nGameState != m_nLastGameState)
	{
		SetCurrentMovie( "media/bg_01.bik" );
	}
	m_nLastGameState = nGameState;

	if (m_pVideoMaterial == NULL)
	{
		m_nLastGameState = 1;
		return;
	}

	if(!m_pVideoMaterial->IsVideoPlaying()) {
		if(!m_pVideoMaterial->StartVideo())
			ClearCurrentMovie();
	}

	if(m_pVideoMaterial->Update() == false)
		ClearCurrentMovie();
}

// ======================================

CNB_Header_Footer::CNB_Header_Footer( vgui::Panel *parent, const char *name ) : BaseClass( parent, name )
{
	// == MANAGED_MEMBER_CREATION_START: Do not edit by hand ==
	m_pBackground = new vgui::Panel( this, "Background" );
	m_pBackgroundImage = new vgui::ImagePanel( this, "BackgroundImage" );	
	m_pTitle = new vgui::Label( this, "Title", "" );
	m_pBottomBar = new vgui::Panel( this, "BottomBar" );
	m_pBottomBarLine = new vgui::Panel( this, "BottomBarLine" );
	m_pTopBar = new vgui::Panel( this, "TopBar" );
	m_pTopBarLine = new vgui::Panel( this, "TopBarLine" );
	// == MANAGED_MEMBER_CREATION_END ==
	m_pGradientBar = new CNB_Gradient_Bar( this, "GradientBar" );
	m_pGradientBar->SetZPos( 2 );

	m_bHeaderEnabled = true;
	m_bFooterEnabled = true;
	m_bMovieEnabled = true;
	m_bGradientBarEnabled = 0;
	m_nTitleStyle = NB_TITLE_MEDIUM;
	m_nBackgroundStyle = NB_BACKGROUND_NONE;
	m_nGradientBarY = 0;
	m_nGradientBarHeight = 480;
}

CNB_Header_Footer::~CNB_Header_Footer()
{

}

extern ConVar asw_force_background_movie;
ConVar asw_background_color( "sdk_background_color", "16 32 46 128", FCVAR_NONE, "Color of background tinting in briefing screens" );

void CNB_Header_Footer::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	
	LoadControlSettings( "resource/ui/nb_header_footer.res" );

	// TODO: Different image in widescreen to avoid stretching
	// this image is no longer used
	//m_pBackgroundImage->SetImage( "../console/background01_widescreen" );

	switch( m_nTitleStyle )
	{
		case NB_TITLE_BRIGHT: m_pTitle->SetFgColor( Color( 255, 255, 255, 255 ) ); break;
		case NB_TITLE_MEDIUM: m_pTitle->SetFgColor( Color( 47, 79, 111, 255 ) ); break;
	}

	switch( m_nBackgroundStyle )
	{
		case NB_BACKGROUND_DARK:
			{
				m_pBackground->SetVisible( true );
				m_pBackgroundImage->SetVisible( false );
				m_pBackground->SetBgColor( Color( 0, 0, 0, 230 ) );
				break;
			}
		case NB_BACKGROUND_TRANSPARENT_BLUE:
			{
				m_pBackground->SetVisible( true );
				m_pBackgroundImage->SetVisible( false );

				color32 clr32;
				UTIL_StringToColor32(&clr32, asw_background_color.GetString());
				
				Color clr;
				clr.SetColor(clr32.r, clr32.g, clr32.b, clr32.a);

				m_pBackground->SetBgColor(clr);
				break;
			}
		case NB_BACKGROUND_TRANSPARENT_RED:
			{
				m_pBackground->SetVisible( true );
				m_pBackgroundImage->SetVisible( false );
				m_pBackground->SetBgColor( Color( 128, 0, 0, 128 ) );
				break;
			}
		case NB_BACKGROUND_BLUE:
			{
				m_pBackground->SetVisible( true );
				m_pBackgroundImage->SetVisible( false );
				m_pBackground->SetBgColor( Color( 16, 32, 46, 230 ) );
				break;
			}
		case NB_BACKGROUND_IMAGE:
			{
				m_pBackground->SetVisible( false );
				m_pBackgroundImage->SetVisible( true );
				break;
			}

		case NB_BACKGROUND_NONE:
			{
				m_pBackground->SetVisible( false );
				m_pBackgroundImage->SetVisible( false );
			}
	}

	m_pTopBar->SetVisible( m_bHeaderEnabled );
	m_pTopBarLine->SetVisible( m_bHeaderEnabled );
	m_pBottomBar->SetVisible( m_bFooterEnabled );
	m_pBottomBarLine->SetVisible( m_bFooterEnabled );
	m_pGradientBar->SetVisible( m_bGradientBarEnabled );
}

void CNB_Header_Footer::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pGradientBar->SetBounds( 0, YRES( m_nGradientBarY ), ScreenWidth(), YRES( m_nGradientBarHeight ) );
}

void CNB_Header_Footer::OnThink()
{
	BaseClass::OnThink();
}

void CNB_Header_Footer::SetTitle( const char *pszTitle )
{
	m_pTitle->SetText( pszTitle );
}

void CNB_Header_Footer::SetTitle( wchar_t *pwszTitle )
{
	m_pTitle->SetText( pwszTitle );
}

void CNB_Header_Footer::SetHeaderEnabled( bool bEnabled )
{
	m_pTopBar->SetVisible( bEnabled );
	m_pTopBarLine->SetVisible( bEnabled );
	m_bHeaderEnabled = bEnabled;
}

void CNB_Header_Footer::SetFooterEnabled( bool bEnabled )
{
	m_pBottomBar->SetVisible( bEnabled );
	m_pBottomBarLine->SetVisible( bEnabled );
	m_bFooterEnabled = bEnabled;
}

void CNB_Header_Footer::SetGradientBarEnabled( bool bEnabled )
{
	m_pGradientBar->SetVisible( bEnabled );
	m_bGradientBarEnabled = bEnabled;
}

void CNB_Header_Footer::SetGradientBarPos( int nY, int nHeight )
{
	m_nGradientBarY = nY;
	m_nGradientBarHeight = nHeight;
	m_pGradientBar->SetBounds( 0, YRES( m_nGradientBarY ), ScreenWidth(), YRES( m_nGradientBarHeight ) );
}

void CNB_Header_Footer::SetTitleStyle( NB_Title_Style nTitleStyle )
{
	m_nTitleStyle = nTitleStyle;
	InvalidateLayout( false, true );
}

void CNB_Header_Footer::SetBackgroundStyle( NB_Background_Style nBackgroundStyle )
{
	m_nBackgroundStyle = nBackgroundStyle;
	InvalidateLayout( false, true );
}

void CNB_Header_Footer::SetMovieEnabled( bool bMovieEnabled )
{
	m_bMovieEnabled = bMovieEnabled;
	InvalidateLayout( false, true );
}

void CNB_Header_Footer::PaintBackground()
{
	BaseClass::PaintBackground();

	/*if (ASWBackgroundMovie())
	{
		ASWBackgroundMovie()->Update();

		if (ASWBackgroundMovie()->GetVideoMaterial())
		{
			// Draw the polys to draw this out
			CMatRenderContextPtr pRenderContext(materials);

			pRenderContext->MatrixMode(MATERIAL_VIEW);
			pRenderContext->PushMatrix();
			pRenderContext->LoadIdentity();

			pRenderContext->MatrixMode(MATERIAL_PROJECTION);
			pRenderContext->PushMatrix();
			pRenderContext->LoadIdentity();

			pRenderContext->Bind(ASWBackgroundMovie()->GetVideoMaterial()->GetMaterial(), NULL);

			CMeshBuilder meshBuilder;
			IMesh* pMesh = pRenderContext->GetDynamicMesh(true);
			meshBuilder.Begin(pMesh, MATERIAL_QUADS, 1);

			int xpos = 0;
			int ypos = 0;
			vgui::ipanel()->GetAbsPos(GetVPanel(), xpos, ypos);

			float flLeftX = xpos;
			float flRightX = xpos + (ASWBackgroundMovie()->m_nPlaybackWidth - 1);

			float flTopY = ypos;
			float flBottomY = ypos + (ASWBackgroundMovie()->m_nPlaybackHeight - 1);

			// Map our UVs to cut out just the portion of the video we're interested in
			float flLeftU = 0.0f;
			float flTopV = 0.0f;

			// We need to subtract off a pixel to make sure we don't bleed
			float flRightU = ASWBackgroundMovie()->m_flU - (1.0f / (float)ASWBackgroundMovie()->m_nPlaybackWidth);
			float flBottomV = ASWBackgroundMovie()->m_flV - (1.0f / (float)ASWBackgroundMovie()->m_nPlaybackHeight);

			// Get the current viewport size
			int vx, vy, vw, vh;
			pRenderContext->GetViewport(vx, vy, vw, vh);

			// map from screen pixel coords to -1..1
			flRightX = FLerp(-1, 1, 0, vw, flRightX);
			flLeftX = FLerp(-1, 1, 0, vw, flLeftX);
			flTopY = FLerp(1, -1, 0, vh, flTopY);
			flBottomY = FLerp(1, -1, 0, vh, flBottomY);

			float alpha = ((float)GetFgColor()[3] / 255.0f);

			for (int corner = 0; corner<4; corner++)
			{
				bool bLeft = (corner == 0) || (corner == 3);
				meshBuilder.Position3f((bLeft) ? flLeftX : flRightX, (corner & 2) ? flBottomY : flTopY, 0.0f);
				meshBuilder.Normal3f(0.0f, 0.0f, 1.0f);
				meshBuilder.TexCoord2f(0, (bLeft) ? flLeftU : flRightU, (corner & 2) ? flBottomV : flTopV);
				meshBuilder.TangentS3f(0.0f, 1.0f, 0.0f);
				meshBuilder.TangentT3f(1.0f, 0.0f, 0.0f);
				meshBuilder.Color4f(1.0f, 1.0f, 1.0f, alpha);
				meshBuilder.AdvanceVertex();
			}

			meshBuilder.End();
			pMesh->Draw();

			pRenderContext->MatrixMode(MATERIAL_VIEW);
			pRenderContext->PopMatrix();

			pRenderContext->MatrixMode(MATERIAL_PROJECTION);
			pRenderContext->PopMatrix();
		}
	}*/
}

// =================

CNB_Gradient_Bar::CNB_Gradient_Bar( vgui::Panel *parent, const char *name ) : BaseClass( parent, name )
{
}

void CNB_Gradient_Bar::PaintBackground()
{
	/*int wide, tall;
	GetSize( wide, tall );

	int y = 0;
	int iHalfWide = wide * 0.5f;

	float flAlpha = 200.0f / 255.0f;

	// fill bar background
	vgui::surface()->DrawSetColor( Color( 0, 0, 0, 255 * flAlpha ) );
	vgui::surface()->DrawFilledRect( 0, y, wide, y + tall );
	
	vgui::surface()->DrawSetColor( Color( 64, 64, 64, 255 * flAlpha ) );

	int nBarPosY = y + YRES( 4 );
	int nBarHeight = tall - YRES( 8 );
	vgui::surface()->DrawFilledRectFade( iHalfWide, nBarPosY, wide, nBarPosY + nBarHeight, 255, 0, true );
	vgui::surface()->DrawFilledRectFade( 0, nBarPosY, iHalfWide, nBarPosY + nBarHeight, 0, 255, true );
	// draw highlights
	nBarHeight = YRES( 2 );
	nBarPosY = y;
	vgui::surface()->DrawSetColor( Color( 170, 170, 170, 255 * flAlpha ) );
	vgui::surface()->DrawFilledRectFade( iHalfWide, nBarPosY, wide, nBarPosY + nBarHeight, 255, 0, true );
	vgui::surface()->DrawFilledRectFade( 0, nBarPosY, iHalfWide, nBarPosY + nBarHeight, 0, 255, true );

	nBarPosY = y + tall - YRES( 2 );
	vgui::surface()->DrawFilledRectFade( iHalfWide, nBarPosY, wide, nBarPosY + nBarHeight, 255, 0, true );
	vgui::surface()->DrawFilledRectFade( 0, nBarPosY, iHalfWide, nBarPosY + nBarHeight, 0, 255, true );*/
}
