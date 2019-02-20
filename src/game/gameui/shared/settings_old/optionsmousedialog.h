//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef OPTIONSMOUSEDIALOG_H
#define OPTIONSMOUSEDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyDialog.h"

//-----------------------------------------------------------------------------
// Purpose: Holds all the game option pages
//-----------------------------------------------------------------------------
class COptionsMouseDialog : public vgui::PropertyDialog
{
	DECLARE_CLASS_SIMPLE( COptionsMouseDialog, vgui::PropertyDialog );

public:
	COptionsMouseDialog(vgui::Panel *parent );
	~COptionsMouseDialog();

	void Run();
	virtual void Activate();

	MESSAGE_FUNC( OnGameUIHidden, "GameUIHidden" );	// called when the GameUI is hidden
};


#define OPTIONS_MAX_NUM_ITEMS 15

struct OptionData_t;

#endif // OPTIONSDIALOG_H
