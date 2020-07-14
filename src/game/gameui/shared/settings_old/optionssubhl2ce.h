//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef OPTIONS_SUB_HL2CE_H
#define OPTIONS_SUB_HL2CE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/panel.h>
#include <vgui_controls/combobox.h>
#include <vgui_controls/propertypage.h>
#include "engineinterface.h"
#include "igameuifuncs.h"
#include "urlbutton.h"

class CCvarSlider;
class CCvarToggleCheckButton;
class CLabeledCommandComboBox;

//-----------------------------------------------------------------------------
// Purpose: Video Details, Part of OptionsDialog
//-----------------------------------------------------------------------------
class COptionsSubHL2CE : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( COptionsSubHL2CE, vgui::PropertyPage );

public:
	COptionsSubHL2CE(vgui::Panel *parent);
	~COptionsSubHL2CE();

	virtual void OnResetData();
	virtual void OnApplyChanges();

	CCvarToggleCheckButton		*m_pAutoAimCheckBox;
	CCvarToggleCheckButton		*m_pViewmodelCheckBox;
	vgui::ComboBox				*m_pLeftHandComboBox;
	CCvarSlider					*m_pFOVSlider;
	vgui::TextEntry				*m_pFOVTextEntry;
	vgui::Button				*m_pResetButton;
	
private:
	MESSAGE_FUNC_PTR( OnControlModified, "ControlModified", panel );
    MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel )
	{
		OnControlModified( panel );
	}
	void UpdateSensitivityLabel();
};



#endif // OPTIONS_SUB_HL2CE_H
