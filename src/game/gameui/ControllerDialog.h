//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CONTROLLERDIALOG_H
#define CONTROLLERDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "shared/settings_old/OptionsDialog.h"

class CControllerDialog : public COptionsDialog
{
	DECLARE_CLASS_SIMPLE( CControllerDialog, COptionsDialog );

public:
	CControllerDialog(vgui::EditablePanel *parent);

	virtual void		ApplySchemeSettings( vgui::IScheme *pScheme );

};

#endif // CONTROLLERDIALOG_H

