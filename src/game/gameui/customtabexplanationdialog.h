//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef CUSTOMTABEXPLANATIONDIALOG_H
#define CUSTOMTABEXPLANATIONDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/frame.h"
#include "utlvector.h"
#include <vgui/keycode.h>
#include "vgui_controls/urllabel.h"

//-----------------------------------------------------------------------------
// Purpose: Dialog that explains the custom tab
//-----------------------------------------------------------------------------
class CCustomTabExplanationDialog : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CCustomTabExplanationDialog, vgui::Frame );

public:
	CCustomTabExplanationDialog(vgui::Panel *parent);
	~CCustomTabExplanationDialog();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void OnCommand( const char *command );
	virtual void OnClose( void );
};

#endif // CUSTOMTABEXPLANATIONDIALOG_H
