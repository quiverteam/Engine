//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "OptionsSubHL2CE.h"
#include "CvarSlider.h"
#include "EngineInterface.h"
#include "BasePanel.h"
#include "igameuifuncs.h"
#include "modes.h"
#include "materialsystem/materialsystem_config.h"
#include "filesystem.h"
#include "GameUI_Interface.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/QueryBox.h"
#include "CvarToggleCheckButton.h"
#include "tier1/KeyValues.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "vgui/ISystem.h"
#include "tier0/ICommandLine.h"
#include "tier1/convar.h"
#include "ModInfo.h"

#include "inetchannelinfo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COptionsSubHL2CE::COptionsSubHL2CE(vgui::Panel *parent) : PropertyPage(parent, NULL)
{
	SetBuildModeEditable(true);

	m_pAutoAimCheckBox = new CCvarToggleCheckButton(
		this, 
		"AutoAim", 
		"#HL2CEUI_AutoAim", 
		"sv_autoaim" );

	m_pViewmodelCheckBox = new CCvarToggleCheckButton(
		this, 
		"VMMode", 
		"#HL2CEUI_ViewmodelMode", 
		"cl_usepredictedviewmodel" );

	m_pFOVSlider = new CCvarSlider(this, "FOVSlider", "#HL2CEUI_FovSlider", 30, 95, "viewmodel_fov", false);

	m_pFOVTextEntry = new TextEntry(this, "FOVBox");
	m_pFOVTextEntry->AddActionSignalTarget(this);
	
	ConVarRef var( "viewmodel_fov" );
	if ( var.IsValid() )
	{
		float vmfov = var.GetInt();

		char buf[64];
		Q_snprintf(buf, sizeof(buf), " %.1f", vmfov);
		m_pFOVTextEntry->SetText(buf);
	}

	m_pLeftHandComboBox = new ComboBox(this, "HandSwitch", 6, false);
	m_pLeftHandComboBox->AddItem("#HL2CEUI_Left_Handed", new KeyValues("HandSwitch", "handness", 0));
	m_pLeftHandComboBox->AddItem("#HL2CEUI_Right_Handed", new KeyValues("HandSwitch", "handness", 1));
	
	LoadControlSettings("Resource/OptionsSubHL2CE.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COptionsSubHL2CE::~COptionsSubHL2CE()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubHL2CE::OnResetData()
{
	m_pAutoAimCheckBox->Reset();
	m_pViewmodelCheckBox->Reset();
	m_pFOVSlider->Reset();

	ConVarRef cl_righthand("cl_righthand");
	if (cl_righthand.IsValid())
	{
		m_pLeftHandComboBox->ActivateItem(cl_righthand.GetInt());
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubHL2CE::OnApplyChanges()
{
	m_pAutoAimCheckBox->ApplyChanges();
	m_pViewmodelCheckBox->ApplyChanges();
	m_pFOVSlider->ApplyChanges();

	if (m_pLeftHandComboBox->IsEnabled())
	{
		ConVarRef cl_righthand("cl_righthand");
		cl_righthand.SetValue(m_pLeftHandComboBox->GetActiveItem());
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubHL2CE::OnControlModified(Panel *panel)
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));

    // the HasBeenModified() check is so that if the value is outside of the range of the
    // slider, it won't use the slider to determine the display value but leave the
    // real value that we determined in the constructor
	if (panel == m_pFOVSlider && m_pFOVSlider->HasBeenModified())
    {
        UpdateSensitivityLabel();
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubHL2CE::OnTextChanged(Panel *panel)
{
	if (panel == m_pFOVTextEntry)
    {
        char buf[64];
		m_pFOVTextEntry->GetText(buf, 64);

        float fValue = (float) atof(buf);
        if (fValue >= 1.0)
        {
			m_pFOVSlider->SetSliderValue(fValue);
            PostActionSignal(new KeyValues("ApplyButtonEnable"));
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubHL2CE::UpdateSensitivityLabel()
{
    char buf[64];
	Q_snprintf(buf, sizeof(buf), " %.1f", m_pFOVSlider->GetSliderValue());
	m_pFOVTextEntry->SetText(buf);
}

