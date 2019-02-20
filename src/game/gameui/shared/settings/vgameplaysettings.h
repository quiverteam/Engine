//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef __VGAMEPLAY_H__
#define __VGAMEPLAY_H__


#include "basemodui.h"
#include "VFlyoutMenu.h"
#include "../settings_old/OptionsSubAudio.h"

class CNB_Header_Footer;

namespace BaseModUI {

class DropDownMenu;
class SliderControl;
class BaseModHybridButton;


class Gameplay : public CBaseModFrame, public FlyoutMenuListener
{
	DECLARE_CLASS_SIMPLE( Gameplay, CBaseModFrame );

public:
	Gameplay(vgui::Panel *parent, const char *panelName);
	~Gameplay();

	//FloutMenuListener
	virtual void OnNotifyChildFocus( vgui::Panel* child );
	virtual void OnFlyoutMenuClose( vgui::Panel* flyTo );
	virtual void OnFlyoutMenuCancelled();
	virtual void PerformLayout();

	Panel* NavigateBack();

protected:
	virtual void Activate();
	virtual void OnThink();
	virtual void PaintBackground();
	virtual void ApplySchemeSettings( vgui::IScheme* pScheme );
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void OnCommand( const char *command );

private:
	void		UpdateFooter( bool bEnableCloud );

private:
	CNB_Header_Footer *m_pHeaderFooter;

	SliderControl	*m_sldGameVolume;

	DropDownMenu	*m_drpSkillLevel;

	BaseModHybridButton	*m_btnCancel;

	///BaseModHybridButton	*m_btn3rdPartyCredits;
};

};

#endif
