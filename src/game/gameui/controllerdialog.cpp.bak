//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "BasePanel.h"
#include "ControllerDialog.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


CControllerDialog::CControllerDialog( vgui::EditablePanel *parent ) : BaseClass( parent )	// TRUE second param says we want the controller options
{
}

void CControllerDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	
	SetControlString( "TitleLabel", "#GameUI_Controller" );
}
