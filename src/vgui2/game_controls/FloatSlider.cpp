//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <stdio.h>
#define PROTECTED_THINGS_DISABLE

#include <vgui/MouseCode.h>
#include <KeyValues.h>
#include <vgui/IBorder.h>
#include <vgui/IInput.h>
#include <vgui/ISystem.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include "game_controls/FloatSlider.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/TextImage.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

DECLARE_BUILD_FACTORY( FloatSlider );

static const float NOB_SIZE = 8.0f;

//-----------------------------------------------------------------------------
// Purpose: Create a slider bar with ticks underneath it
//-----------------------------------------------------------------------------
FloatSlider::FloatSlider(Panel *parent, const char *panelName ) : BaseClass(parent, panelName)
{
	m_bIsDragOnRepositionNob = false;
	_dragging = false;
	_value = 0;
	_range[0] = 0;
	_range[1] = 0;
	_buttonOffset = 0;
	_sliderBorder = NULL;
	_insetBorder = NULL;
	m_nNumTicks = 10;
	_leftCaption = NULL;
	_rightCaption = NULL;

	_subrange[ 0 ] = 0;
	_subrange[ 1 ] = 0;
	m_bUseSubRange = false;
	m_bInverted = false;

	SetThumbWidth( 8 );
	RecomputeNobPosFromValue();
	AddActionSignalTarget(this);
	SetBlockDragChaining( true );
}

// This allows the slider to behave like it's larger than what's actually being drawn
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bEnable - 
//			0 - 
//			100 - 
//-----------------------------------------------------------------------------
void FloatSlider::SetSliderThumbSubRange( bool bEnable, int nMin /*= 0*/, int nMax /*= 100*/ )
{
	m_bUseSubRange = bEnable;
	_subrange[ 0 ] = nMin;
	_subrange[ 1 ] = nMax;
}

//-----------------------------------------------------------------------------
// Purpose: Set the size of the slider bar.
//			Warning less than 30 pixels tall and everything probably won't fit.
//-----------------------------------------------------------------------------
void FloatSlider::OnSizeChanged(int wide,int tall)
{
	BaseClass::OnSizeChanged(wide,tall);

	RecomputeNobPosFromValue();
}

//-----------------------------------------------------------------------------
// Purpose: Set the value of the slider to one of the ticks.
//-----------------------------------------------------------------------------
void FloatSlider::SetValue(float value, bool bTriggerChangeMessage)
{
	float oldValue=_value;

	if ( _range[0] < _range[1] )
	{
		if(value<_range[0])
		{
			value=_range[0];
		}
		if(value>_range[1])
		{
			value=_range[1];	
		}
	}
	else
	{
		if(value<_range[1])
		{
			value=_range[1];
		}
		if(value>_range[0])
		{
			value=_range[0];	
		}
	}

	_value = value;
	
	RecomputeNobPosFromValue();

	if (_value != oldValue && bTriggerChangeMessage)
	{
		SendSliderMovedMessage();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return the value of the slider
//-----------------------------------------------------------------------------
float FloatSlider::GetValue()
{
	return _value;
}

//-----------------------------------------------------------------------------
// Purpose: Layout the slider before drawing it on screen.
//-----------------------------------------------------------------------------
void FloatSlider::PerformLayout()
{
	BaseClass::PerformLayout();
	RecomputeNobPosFromValue();

	if (_leftCaption)
	{
		_leftCaption->ResizeImageToContent();
	}
	if (_rightCaption)
	{
		_rightCaption->ResizeImageToContent();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Move the nob on the slider in response to changing its value.
//-----------------------------------------------------------------------------
void FloatSlider::RecomputeNobPosFromValue()
{
	//int wide,tall;
	//GetPaintSize(wide,tall);
	int x, y, wide, tall;
	GetTrackRect( x, y, wide, tall );

	float usevalue = _value;
	float *userange = &_range[ 0 ];
	if ( m_bUseSubRange )
	{
		userange = &_subrange[ 0 ];
		usevalue = clamp( _value, _subrange[ 0 ], _subrange[ 1 ] );
	}

	float fwide=(float)wide;
	float frange=(float)(userange[1] -userange[0]);
	float fvalue=(float)(usevalue -userange[0]);
	float fper = (frange != 0.0f) ? fvalue / frange : 0.0f;

	if ( m_bInverted )
		fper = 1.0f - fper;

	float freepixels = fwide - _nobSize;
	float leftpixel = (float)x;
	float firstpixel = leftpixel + freepixels * fper + 0.5f;

	_nobPos[0]=(int)( firstpixel );
	_nobPos[1]=(int)( firstpixel + _nobSize );


	int rightEdge = x + wide;

	if(_nobPos[1]> rightEdge )
	{
		_nobPos[0]=rightEdge-_nobSize;
		_nobPos[1]=rightEdge;
	}
	
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Sync the slider's value up with the nob's position.
//-----------------------------------------------------------------------------
void FloatSlider::RecomputeValueFromNobPos()
{
	float value = EstimateValueAtPos( _nobPos[ 0 ], 0 );
	SetValue( value );
}

float FloatSlider::EstimateValueAtPos( int localMouseX, int /*localMouseY*/ )
{
	int x, y, wide, tall;
	GetTrackRect( x, y, wide, tall );

	float *userange = &_range[ 0 ];
	if ( m_bUseSubRange )
	{
		userange = &_subrange[ 0 ];
	}

	float fwide = (float)wide;
	float fvalue = (float)( _value - userange[0] );
	float fnob = (float)( localMouseX - x );
	float freepixels = fwide - _nobSize;

	// Map into reduced range
	fvalue = freepixels != 0.0f ? fnob / freepixels : 0.0f;

	return RemapVal( fvalue, 0.0, 1.0, userange[0], userange[1] );
}

void FloatSlider::SetInverted( bool bInverted )
{
	m_bInverted = bInverted;
}


//-----------------------------------------------------------------------------
// Purpose: Send a message to interested parties when the slider moves
//-----------------------------------------------------------------------------
void FloatSlider::SendSliderMovedMessage()
{	
	// send a changed message
	KeyValues *pParams = new KeyValues("SliderMoved", "position", _value);
	pParams->SetPtr( "panel", this );
	PostActionSignal( pParams );
}

//-----------------------------------------------------------------------------
// Purpose: Send a message to interested parties when the user begins dragging the slider
//-----------------------------------------------------------------------------
void FloatSlider::SendSliderDragStartMessage()
{	
	// send a message
	KeyValues *pParams = new KeyValues("SliderDragStart", "position", _value);
	pParams->SetPtr( "panel", this );
	PostActionSignal( pParams );
}

//-----------------------------------------------------------------------------
// Purpose: Send a message to interested parties when the user ends dragging the slider
//-----------------------------------------------------------------------------
void FloatSlider::SendSliderDragEndMessage()
{	
	// send a message
	KeyValues *pParams = new KeyValues("SliderDragEnd", "position", _value);
	pParams->SetPtr( "panel", this );
	PostActionSignal( pParams );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FloatSlider::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetFgColor(GetSchemeColor("FloatSlider.NobColor", pScheme));
	// this line is useful for debugging
	//SetBgColor(GetSchemeColor("0 0 0 255"));

	m_TickColor = pScheme->GetColor( "FloatSlider.TextColor", GetFgColor() );
	m_TrackColor = pScheme->GetColor( "FloatSlider.TrackColor", GetFgColor() );

#ifdef _X360
	m_DepressedBgColor = GetSchemeColor("FloatSlider.NobFocusColor", pScheme);
#endif

	m_DisabledTextColor1 = pScheme->GetColor( "FloatSlider.DisabledTextColor1", GetFgColor() );
	m_DisabledTextColor2 = pScheme->GetColor( "FloatSlider.DisabledTextColor2", GetFgColor() );

	_sliderBorder = pScheme->GetBorder("ButtonBorder");
	_insetBorder = pScheme->GetBorder("ButtonDepressedBorder");

	if ( _leftCaption )
	{
		_leftCaption->SetFont(pScheme->GetFont("DefaultVerySmall", IsProportional() ));
	}

	if ( _rightCaption )
	{
		_rightCaption->SetFont(pScheme->GetFont("DefaultVerySmall", IsProportional() ));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FloatSlider::GetSettings(KeyValues *outResourceData)
{
	BaseClass::GetSettings(outResourceData);
	
	char buf[256];
	if (_leftCaption)
	{
		_leftCaption->GetUnlocalizedText(buf, sizeof(buf));
		outResourceData->SetString("leftText", buf);
	}
	
	if (_rightCaption)
	{
		_rightCaption->GetUnlocalizedText(buf, sizeof(buf));
		outResourceData->SetString("rightText", buf);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FloatSlider::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	const char *left = inResourceData->GetString("leftText", NULL);
	const char *right = inResourceData->GetString("rightText", NULL);

	int thumbWidth = inResourceData->GetInt("thumbwidth", 0);
	if (thumbWidth != 0)
	{
		SetThumbWidth(thumbWidth);
	}

	SetTickCaptions(left, right);

	int nNumTicks = inResourceData->GetInt( "numTicks", -1 );
	if ( nNumTicks >= 0 )
	{
		SetNumTicks( nNumTicks );
	}

	float nCurrentRange[2];
	GetRange( nCurrentRange[0], nCurrentRange[1] );
	KeyValues *pRangeMin = inResourceData->FindKey( "rangeMin", false );
	KeyValues *pRangeMax = inResourceData->FindKey( "rangeMax", false );
	bool bDoClamp = false;
	if ( pRangeMin )
	{
		_range[0] = inResourceData->GetFloat( "rangeMin" );
		bDoClamp = true;
	}
	if ( pRangeMax )
	{
		_range[1] = inResourceData->GetFloat( "rangeMax" );
		bDoClamp = true;
	}

	if ( bDoClamp )
	{
		ClampRange();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *FloatSlider::GetDescription()
{
	static char buf[1024];
	Q_snprintf(buf, sizeof(buf), "%s, string leftText, string rightText", BaseClass::GetDescription());
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: Get the rectangle to draw the slider track in.
//-----------------------------------------------------------------------------
void FloatSlider::GetTrackRect( int& x, int& y, int& w, int& h )
{
	int wide, tall;
	GetPaintSize( wide, tall );

	x = 0;
	y = 8;
	w = wide - (int)_nobSize;
	h = 4;
}

//-----------------------------------------------------------------------------
// Purpose: Draw everything on screen
//-----------------------------------------------------------------------------
void FloatSlider::Paint()
{
	DrawTicks();

	DrawTickLabels();

	// Draw nob last so it draws over ticks.
	DrawNob();
}

//-----------------------------------------------------------------------------
// Purpose: Draw the ticks below the slider.
//-----------------------------------------------------------------------------
void FloatSlider::DrawTicks()
{
	int x, y;
	int wide,tall;
	GetTrackRect( x, y, wide, tall );

	// Figure out how to draw the ticks
//	GetPaintSize( wide, tall );

	float fwide  = (float)wide;
	float freepixels = fwide - _nobSize;

	float leftpixel = _nobSize / 2.0f;

	float pixelspertick = freepixels / ( m_nNumTicks );

	y += (int)_nobSize;
	int tickHeight = 5;

    if (IsEnabled())
    {
        surface()->DrawSetColor( m_TickColor ); //vgui::Color( 127, 140, 127, 255 ) );
    	for ( int i = 0; i <= m_nNumTicks; i++ )
    	{
    		int xpos = (int)( leftpixel + i * pixelspertick );
    
    		surface()->DrawFilledRect( xpos, y, xpos + 1, y + tickHeight );
    	}
    }
    else
    {
        surface()->DrawSetColor( m_DisabledTextColor1 ); //vgui::Color( 127, 140, 127, 255 ) );
    	for ( int i = 0; i <= m_nNumTicks; i++ )
    	{
    		int xpos = (int)( leftpixel + i * pixelspertick );
    		surface()->DrawFilledRect( xpos+1, y+1, xpos + 2, y + tickHeight + 1 );
    	}
        surface()->DrawSetColor( m_DisabledTextColor2 ); //vgui::Color( 127, 140, 127, 255 ) );
    	for ( int i = 0; i <= m_nNumTicks; i++ )
    	{
    		int xpos = (int)( leftpixel + i * pixelspertick );
    		surface()->DrawFilledRect( xpos, y, xpos + 1, y + tickHeight );
    	}
    }
}

//-----------------------------------------------------------------------------
// Purpose: Draw Tick labels under the ticks.
//-----------------------------------------------------------------------------
void FloatSlider::DrawTickLabels()
{
	int x, y;
	int wide,tall;
	GetTrackRect( x, y, wide, tall );

	// Figure out how to draw the ticks
//	GetPaintSize( wide, tall );
	y += (int)NOB_SIZE + 4;

	// Draw Start and end range values
    if (IsEnabled())
	    surface()->DrawSetTextColor( m_TickColor ); //vgui::Color( 127, 140, 127, 255 ) );
    else
	    surface()->DrawSetTextColor( m_DisabledTextColor1 ); //vgui::Color( 127, 140, 127, 255 ) );


	if ( _leftCaption != NULL )
	{
		_leftCaption->SetPos(0, y);
        if (IsEnabled())
		{
		    _leftCaption->SetColor( m_TickColor ); 
		}
        else
		{
		    _leftCaption->SetColor( m_DisabledTextColor1 ); 
		}

		_leftCaption->Paint();
	}

	if ( _rightCaption != NULL)
	{
		int rwide, rtall;
		_rightCaption->GetSize(rwide, rtall);
		_rightCaption->SetPos((int)(wide - rwide) , y);
        if (IsEnabled())
		{
    		_rightCaption->SetColor( m_TickColor );
		}
		else
		{
    		_rightCaption->SetColor( m_DisabledTextColor1 );
		}

		_rightCaption->Paint();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw the nob part of the slider.
//-----------------------------------------------------------------------------
void FloatSlider::DrawNob()
{
	// horizontal nob
	int x, y;
	int wide,tall;
	GetTrackRect( x, y, wide, tall );
	Color col = GetFgColor();
#ifdef _X360
	if(HasFocus())
	{
		col = m_DepressedBgColor;
	}
#endif
	surface()->DrawSetColor(col);

	int nobheight = 16;

	surface()->DrawFilledRect(
		_nobPos[0], 
		y + tall / 2 - nobheight / 2, 
		_nobPos[1], 
		y + tall / 2 + nobheight / 2);
	// border
	if (_sliderBorder)
	{
		_sliderBorder->Paint(
			_nobPos[0], 
			y + tall / 2 - nobheight / 2, 
			_nobPos[1], 
			y + tall / 2 + nobheight / 2);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the text labels of the Start and end ticks.
//-----------------------------------------------------------------------------
void FloatSlider::SetTickCaptions( const char *left, const char *right )
{
	if (left)
	{
		if (_leftCaption)
		{
			_leftCaption->SetText(left);
		}
		else
		{
			_leftCaption = new TextImage(left);
		}
	}
	if (right)
	{
		if (_rightCaption)
		{
			_rightCaption->SetText(right);
		}
		else
		{
			_rightCaption = new TextImage(right);
		}
	}
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Set the text labels of the Start and end ticks.
//-----------------------------------------------------------------------------
void FloatSlider::SetTickCaptions( const wchar_t *left, const wchar_t *right )
{
	if (left)
	{
		if (_leftCaption)
		{
			_leftCaption->SetText(left);
		}
		else
		{
			_leftCaption = new TextImage(left);
		}
	}
	if (right)
	{
		if (_rightCaption)
		{
			_rightCaption->SetText(right);
		}
		else
		{
			_rightCaption = new TextImage(right);
		}
	}
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Draw the slider track
//-----------------------------------------------------------------------------
void FloatSlider::PaintBackground()
{
	BaseClass::PaintBackground();
	
	int x, y;
	int wide,tall;

	GetTrackRect( x, y, wide, tall );

	surface()->DrawSetColor( m_TrackColor ); 
	surface()->DrawFilledRect( x, y, x + wide, y + tall );
	if (_insetBorder)
	{
		_insetBorder->Paint( x, y, x + wide, y + tall );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the range of the slider.
//-----------------------------------------------------------------------------
void FloatSlider::SetRange(float min, float max)
{
	_range[0]=min;
	_range[1]=max;

	ClampRange();
}

//-----------------------------------------------------------------------------
// Purpose: Sanity check and clamp the range if necessary.
//-----------------------------------------------------------------------------
void FloatSlider::ClampRange()
{
	if ( _range[0] < _range[1] )
	{
		if(_value<_range[0])
		{
			SetValue( _range[0], false );
		}
		else if( _value>_range[1])
		{
			SetValue( _range[1], false );
		}
	}
	else
	{
		if(_value<_range[1])
		{
			SetValue( _range[1], false );
		}
		else if( _value>_range[0])
		{
			SetValue( _range[0], false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the max and min values of the slider
//-----------------------------------------------------------------------------
void FloatSlider::GetRange(float& min, float& max)
{
	min=_range[0];
	max=_range[1];
}

//-----------------------------------------------------------------------------
// Purpose: Respond when the cursor is moved in our window if we are clicking
// and dragging.
//-----------------------------------------------------------------------------
void FloatSlider::OnCursorMoved(int x,int y)
{
	if(!_dragging)
	{
		return;
	}

//	input()->GetCursorPos(x,y);
	input()->GetCursorPosition( x, y );
	ScreenToLocal(x,y);

//	int wide,tall;
//	GetPaintSize(wide,tall);
	int _x, _y, wide, tall;
	GetTrackRect( _x, _y, wide, tall );

	_nobPos[0]=_nobDragStartPos[0]+(x-_dragStartPos[0]);
	_nobPos[1]=_nobDragStartPos[1]+(x-_dragStartPos[0]);

	int rightEdge = _x +wide;
	int unclamped = _nobPos[ 0 ];

	if(_nobPos[1]>rightEdge)
	{
		_nobPos[0]=rightEdge-(_nobPos[1]-_nobPos[0]);
		_nobPos[1]=rightEdge;
	}
		
	if(_nobPos[0]<_x)
	{
		int offset = _x - _nobPos[0];
		_nobPos[1]=_nobPos[1]-offset;
		_nobPos[0]=0;
	}

	int value = EstimateValueAtPos( unclamped, 0 );
	SetValue( value );

	// RecomputeValueFromNobPos();
	Repaint();
	SendSliderMovedMessage();
}

//-----------------------------------------------------------------------------
// Purpose: If you click on the slider outside of the nob, the nob jumps
// to the click position, and if this setting is enabled, the nob
// is then draggable from the new position until the mouse is released
// Input  : state - 
//-----------------------------------------------------------------------------
void FloatSlider::SetDragOnRepositionNob( bool state )
{
	m_bIsDragOnRepositionNob = state;
}

bool FloatSlider::IsDragOnRepositionNob() const
{
	return m_bIsDragOnRepositionNob;
}

bool FloatSlider::IsDragged( void ) const
{
	return _dragging;
}

//-----------------------------------------------------------------------------
// Purpose: Respond to mouse presses. Trigger Record staring positon.
//-----------------------------------------------------------------------------
void FloatSlider::OnMousePressed(MouseCode code)
{
	int x,y;

    if (!IsEnabled())
        return;

//	input()->GetCursorPos(x,y);
	input()->GetCursorPosition( x, y );

	ScreenToLocal(x,y);
    RequestFocus();

	bool startdragging = false, bPostDragStartSignal = false;

	if ((x >= _nobPos[0]) && (x < _nobPos[1]))
	{
		startdragging = true;
		bPostDragStartSignal = true;
	}
	else
	{
		// we clicked elsewhere on the slider; move the nob to that position
		float min, max;
		GetRange(min, max);
		if ( m_bUseSubRange )
		{
			min = _subrange[ 0 ];
			max = _subrange[ 1 ];
		}

//		int wide = GetWide();
		int _x, _y, wide, tall;
		GetTrackRect( _x, _y, wide, tall );
		if ( wide > 0 )
		{
			float frange = ( float )( max - min );
			float clickFrac = clamp( ( float )( x - _x ) / (float)( wide - 1 ), 0.0f, 1.0f );

			float value = (float)min + clickFrac * frange;

			startdragging = IsDragOnRepositionNob();

			if ( startdragging )
			{
				_dragging = true; // Required when as
				SendSliderDragStartMessage();
			}

			SetValue( value );
		}
	}

	if ( startdragging )
	{
		// drag the nob
		_dragging = true;
		input()->SetMouseCapture(GetVPanel());
		_nobDragStartPos[0] = _nobPos[0];
		_nobDragStartPos[1] = _nobPos[1];
		_dragStartPos[0] = x;
		_dragStartPos[1] = y;
	}

	if ( bPostDragStartSignal )
		SendSliderDragStartMessage();
}

//-----------------------------------------------------------------------------
// Purpose: Just handle double presses like mouse presses
//-----------------------------------------------------------------------------
void FloatSlider::OnMouseDoublePressed(MouseCode code)
{
	OnMousePressed(code);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#ifdef _X360
void FloatSlider::OnKeyCodePressed(KeyCode code)
{
	switch ( GetBaseButtonCode( code ) )
	{
	case KEY_XBUTTON_LEFT:
	case KEY_XSTICK1_LEFT:
	case KEY_XSTICK2_LEFT:
		SetValue(GetValue() - 1);
		break;
	case KEY_XBUTTON_RIGHT:
	case KEY_XSTICK1_RIGHT:
	case KEY_XSTICK2_RIGHT:
		SetValue(GetValue() + 1);
		break;
	default:
		BaseClass::OnKeyCodePressed(code);
		break;
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Handle key presses
//-----------------------------------------------------------------------------
void FloatSlider::OnKeyCodeTyped(KeyCode code)
{
	switch (code)
	{
		// for now left and right arrows just open or close submenus if they are there.
        case KEY_LEFT:
        case KEY_DOWN:
            {
                float val = GetValue();
                SetValue(val-0.1f);
                break;
            }
    	case KEY_RIGHT:
        case KEY_UP:
    		{
                int val = GetValue();
                SetValue(val+0.1f);
    			break;
    		}
        case KEY_PAGEDOWN:
            {
                float min, max;
                GetRange(min, max);
                float range = max-min;
                float pertick = range/m_nNumTicks;
                float val = GetValue();
                SetValue(val - pertick);
                break;
            }
        case KEY_PAGEUP:
            {
                float min, max;
                GetRange(min, max);
                float range = max-min;
                float pertick = range/m_nNumTicks;
                float val = GetValue();
                SetValue(val + pertick);
                break;
            }
        case KEY_HOME:
            {
                float min, max;
                GetRange(min, max);
                SetValue(min);
    			break;
            }
        case KEY_END:
            {
                float min, max;
                GetRange(min, max);
                SetValue(max);
    			break;
            }
    	default:
    		BaseClass::OnKeyCodeTyped(code);
    		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop dragging when the mouse is released.
//-----------------------------------------------------------------------------
void FloatSlider::OnMouseReleased(MouseCode code)
{
	if ( _dragging )
	{
		_dragging=false;
		input()->SetMouseCapture(null);
	}

	if ( IsEnabled() )
	{
		SendSliderDragEndMessage();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the nob's position (the ends of each side of the nob)
//-----------------------------------------------------------------------------
void FloatSlider::GetNobPos(float& min, float& max)
{
	min=_nobPos[0];
	max=_nobPos[1];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FloatSlider::SetButtonOffset(int buttonOffset)
{
	_buttonOffset=buttonOffset;
}

void FloatSlider::SetThumbWidth( int width )
{
	_nobSize = (float)width;
}


//-----------------------------------------------------------------------------
// Purpose: Set the number of ticks that appear under the slider.
//-----------------------------------------------------------------------------
void FloatSlider::SetNumTicks( int ticks )
{
	m_nNumTicks = ticks;
}
