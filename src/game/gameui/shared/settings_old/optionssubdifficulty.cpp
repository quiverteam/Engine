//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "OptionsSubDifficulty.h"
#include "tier1/convar.h"
#include "EngineInterface.h"
#include "tier1/KeyValues.h"

#include "vgui_controls/RadioButton.h"

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
COptionsSubDifficulty::COptionsSubDifficulty(vgui::Panel *parent) : BaseClass(parent, NULL)
{
	m_pEasyRadio = new RadioButton(this, "Skill1Radio", "#GameUI_SkillEasy");
	m_pNormalRadio = new RadioButton(this, "Skill2Radio", "#GameUI_SkillNormal");
	m_pHardRadio = new RadioButton(this, "Skill3Radio", "#GameUI_SkillHard");
	//m_pDeathWishRadio = new RadioButton(this, "Skill4Radio", "#HL2CEUI_SkillDeathWish");

	LoadControlSettings("Resource/OptionsSubDifficulty.res");
}

//-----------------------------------------------------------------------------
// Purpose: resets controls
//-----------------------------------------------------------------------------
void COptionsSubDifficulty::OnResetData()
{
	/*ConVarRef var( "difficulty" );

	if (var.GetInt() == 1)
	{
		m_pEasyRadio->SetSelected(true);
	}
	else if (var.GetInt() == 3)
	{
		m_pHardRadio->SetSelected(true);
	}
	else if (var.GetInt() == 4)
	{
		m_pDeathWishRadio->SetSelected(true);
	}
	else
	{
		m_pNormalRadio->SetSelected(true);
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: sets data based on control settings
//-----------------------------------------------------------------------------
void COptionsSubDifficulty::OnApplyChanges()
{
	/*ConVarRef var( "difficulty" );

	if ( m_pEasyRadio->IsSelected() )
	{
		var.SetValue( 1 );
	}
	else if ( m_pHardRadio->IsSelected() )
	{
		var.SetValue( 3 );
	}
	else if ( m_pDeathWishRadio->IsSelected() )
	{
		var.SetValue( 4 );
	}
	else
	{
		var.SetValue( 2 );
	}*/
}


//-----------------------------------------------------------------------------
// Purpose: enables apply button on radio buttons being pressed
//-----------------------------------------------------------------------------
void COptionsSubDifficulty::OnRadioButtonChecked()
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}
