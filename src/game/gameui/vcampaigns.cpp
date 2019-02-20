#include "cbase.h"
#include "vgui_controls\Button.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "basemodui.h"
#include "nb_header_footer.h"
//#include "..\game\shared\hl2ce\hl2ce_missioninfo.h"
#include "..\game\shared\core\missioninfo.h"

class Campaigns : public vgui::Button
{
public:
	DECLARE_CLASS_SIMPLE( Campaigns, vgui::Button );

	Campaigns( Panel *pParent, const char *pName );
	virtual void ApplySettings( KeyValues *pInResourceData );
	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void Campaigns::OnKeyCodePressed( vgui::KeyCode code );
	virtual void PaintBackground( void );
	virtual void SetActiveCampaign( int index );
private:
	unsigned int m_nActiveCampaign;
	int			m_nLeftArrowId;
	int			m_nRightArrowId;
	int			m_nLeftArrowX;
	int			m_nLeftArrowY;
	int			m_nRightArrowX;
	int			m_nRightArrowY;
	vgui::HFont	m_hTitleFont;
	const wchar_t *m_Title;
	const wchar_t *m_SubTitle;
	int			m_nTitleWide;
	int			m_iGameTitlePos1x;
	int			m_iGameTitlePos1y;
	int			m_iGameTitlePos2x;
	int			m_iGameTitlePos2y;
};

Campaigns::Campaigns( Panel *pParent, const char *pName ) : BaseClass( pParent, pName, "" )
{
	SetProportional( true );
	SetPaintBorderEnabled( false );
	SetPaintBackgroundEnabled( true );
}

using namespace BaseModUI;
using namespace vgui;

DECLARE_BUILD_FACTORY( Campaigns );

void Campaigns::ApplySettings( KeyValues *pInResourceData )
{
	BaseClass::ApplySettings( pInResourceData );

	vgui::HScheme hScheme = vgui::scheme()->GetScheme("SwarmScheme");
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme(hScheme);

	m_hTitleFont = pScheme->GetFont( "CampaignTitle", true );

	m_iGameTitlePos1y = scheme()->GetProportionalScaledValue( 90 );
	m_iGameTitlePos2y = scheme()->GetProportionalScaledValue( 107 );

	m_nLeftArrowId = vgui::surface()->DrawGetTextureId( "vgui/arrow_left" );
	if ( m_nLeftArrowId == -1 )
	{
		m_nLeftArrowId = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_nLeftArrowId, "vgui/arrow_left", true, false );	
	}

	m_nRightArrowId = vgui::surface()->DrawGetTextureId( "vgui/arrow_right" );
	if ( m_nRightArrowId == -1 )
	{
		m_nRightArrowId = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_nRightArrowId, "vgui/arrow_right", true, false );	
	}
	
	int x, y;
	GetPos(x, y);
	m_nLeftArrowX = x - scheme()->GetProportionalScaledValue( 10 );
	m_nLeftArrowY = y;

	SetActiveCampaign(0);
}

void Campaigns::OnMousePressed( vgui::MouseCode code )
{
	BaseClass::OnMousePressed( code );

	if ( code != MOUSE_LEFT )
		return;		

	int iPosX;
	int iPosY;
	input()->GetCursorPos( iPosX, iPosY );
	ScreenToLocal( iPosX, iPosY );

	bool bRightScroll = false;
	bool bLeftScroll = false;

	if ( ( iPosX >= m_nLeftArrowX && iPosX <= m_nLeftArrowX + 21 ) &&
		( iPosY >= m_nLeftArrowY && iPosY <= m_nLeftArrowY + 21 ) )
	{
		bLeftScroll = true;
	}
	else if ( ( iPosX >= m_nRightArrowX && iPosX <= m_nRightArrowX + 21 ) &&
		( iPosY >= m_nRightArrowY && iPosY <= m_nRightArrowY + 21 ) )
	{
		bRightScroll = true;
	}

	if ( bLeftScroll || bRightScroll )
	{
		if ( bLeftScroll )
		{
			SetActiveCampaign( m_nActiveCampaign - 1 );
			CBaseModPanel::GetSingleton().PlayUISound( UISOUND_CLICK );
		}
		else if ( bRightScroll )
		{
			SetActiveCampaign( m_nActiveCampaign + 1 );
			CBaseModPanel::GetSingleton().PlayUISound( UISOUND_CLICK );
		}
	}
}

void Campaigns::OnKeyCodePressed( vgui::KeyCode code )
{
	bool bHandled = false;

	switch( code )
	{
	case KEY_LEFT:
	case KEY_XBUTTON_LEFT:
		{
			SetActiveCampaign( m_nActiveCampaign - 1 );
			CBaseModPanel::GetSingleton().PlayUISound( UISOUND_CLICK );
			bHandled = true;
		}
		break;
	case KEY_RIGHT:
	case KEY_XBUTTON_RIGHT:
		{
			SetActiveCampaign( m_nActiveCampaign + 1 );
			CBaseModPanel::GetSingleton().PlayUISound( UISOUND_CLICK );
			bHandled = true;
		}
		break;
	case KEY_UP:
	case KEY_XBUTTON_UP:
		if ( NavigateUp() )
		{
			bHandled = true;
		}
		break;
	case KEY_DOWN:
	case KEY_XBUTTON_DOWN:
		if ( NavigateDown() )
		{
			bHandled = true;
		}
		break;
	}

	if ( !bHandled )
	{
		BaseClass::OnKeyCodeTyped(code);
	}
}

void Campaigns::PaintBackground()
{
	int iPosX;
	int iPosY;
	input()->GetCursorPos( iPosX, iPosY );
	ScreenToLocal( iPosX, iPosY );

	bool bLeftHighlight = false, bRightHightlight = false;

	if (HasFocus())
	{
		bLeftHighlight = true;
		bRightHightlight = true;
	}
	else if ( ( iPosX >= m_nLeftArrowX && iPosX <= m_nLeftArrowX + 21 ) &&
		( iPosY >= m_nLeftArrowY && iPosY <= m_nLeftArrowY + 21 ) )
	{
		bLeftHighlight = true;
	}
	else if ( ( iPosX >= m_nRightArrowX && iPosX <= m_nRightArrowX + 21 ) &&
	( iPosY >= m_nRightArrowY && iPosY <= m_nRightArrowY + 21 ) )
	{
		bRightHightlight = true;
	}

	Color leftArrowColor;
	leftArrowColor.SetColor( 125, 125, 125, 255 );
	if ( bLeftHighlight )
	{
		leftArrowColor.SetColor( 255, 255, 255, 255 );
	}
	vgui::surface()->DrawSetColor( leftArrowColor );
	vgui::surface()->DrawSetTexture( m_nLeftArrowId );
	vgui::surface()->DrawTexturedRect( m_nLeftArrowX, m_nLeftArrowY, m_nLeftArrowX + 21, m_nLeftArrowY + 21 );

	Color rightArrowColor;
	rightArrowColor.SetColor( 125, 125, 125, 255 );
	if ( bRightHightlight )
	{
		rightArrowColor.SetColor( 255, 255, 255, 255 );
	}
	vgui::surface()->DrawSetColor( rightArrowColor );
	vgui::surface()->DrawSetTexture( m_nRightArrowId );
	vgui::surface()->DrawTexturedRect( m_nRightArrowX, m_nRightArrowY, m_nRightArrowX + 21, m_nRightArrowY + 21 );

	vgui::surface()->DrawSetTextFont(m_hTitleFont);
	vgui::surface()->DrawSetTextPos(m_iGameTitlePos1x, m_iGameTitlePos1y);
	vgui::surface()->DrawSetTextColor(Color(255,255,255,220));
	vgui::surface()->DrawPrintText(m_Title, V_wcslen(m_Title));

	vgui::surface()->DrawSetTextPos(m_iGameTitlePos2x, m_iGameTitlePos2y);
	vgui::surface()->DrawSetTextColor(Color(255,255,255,200));
	vgui::surface()->DrawPrintText(m_SubTitle, V_wcslen(m_SubTitle));
}

void Campaigns::SetActiveCampaign( int rindex )
{
	int index = rindex;
	if ( index > missioninformer->GetCampaignCount() - 1 )
		index = 0;
	else if ( index < 0 )
		index = missioninformer->GetCampaignCount() - 1;

	ASWBackgroundMovie()->SetCurrentMovie(missioninformer->GetCampaignDetails(index)->GetString("Movie"));
	if ( ASWBackgroundMovie()->GetVideoMaterial() )
	{
		float frame = RandomFloat(1.0, (ASWBackgroundMovie()->GetVideoMaterial()->GetVideoDuration() / 12));
		if (ASWBackgroundMovie()->GetVideoMaterial()->SetTime(frame))
			Msg("%f \n", frame);
	}

	m_nActiveCampaign = index;
	m_iGameTitlePos1x = scheme()->GetProportionalScaledValue( missioninformer->GetCampaignDetails(index)->GetInt("DisplayTitleX") );
	m_iGameTitlePos2x = scheme()->GetProportionalScaledValue( missioninformer->GetCampaignDetails(index)->GetInt("DisplaySubTitleX") );

	int x, y;
	GetPos(x, y);

	int textWide, textTall;

	m_Title = missioninformer->GetCampaignDetails(index)->GetWString("DisplayTitle");
	m_SubTitle = missioninformer->GetCampaignDetails(index)->GetWString("DisplaySubTitle");

	//We make space for the largest title
	if ( V_wcslen(m_SubTitle) > V_wcslen(m_Title) )
		vgui::surface()->GetTextSize( m_hTitleFont, m_SubTitle, textWide, textTall );
	else
		vgui::surface()->GetTextSize( m_hTitleFont, m_Title, textWide, textTall );

	m_nRightArrowX = x + scheme()->GetProportionalScaledValue( 15 ) + textWide;
	m_nRightArrowY = m_nLeftArrowY;
}

CON_COMMAND_F(setvideoframe, "setvideoframe", FCVAR_NONE)
{
	ASWBackgroundMovie()->GetVideoMaterial()->SetTime(1.023765);
}