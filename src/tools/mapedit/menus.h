/*

menus for the map editor

*/
#pragma once

#include "toolutils/toolfilemenubutton.h"
#include "toolutils/toolmenubutton.h"
#include "tier1/keyvalues.h"
#include "tier1/utlstring.h"
#include "vgui_controls/menu.h"
#include "vgui_controls/frame.h"
#include "vgui_controls/button.h"
#include "vgui_controls/listpanel.h"
#include "toolutils/enginetools_int.h"

class CEditMenu : public CToolMenuButton
{
	DECLARE_CLASS_SIMPLE(CEditMenu, CToolMenuButton);
public:
	CEditMenu(vgui::Panel* pParent, const char* panelName, const char* text, vgui::Panel* pActionSignalTarget);

	virtual void OnShowMenu(vgui::Menu* menu);
};

class CMapMenu : public CToolMenuButton
{
	DECLARE_CLASS_SIMPLE(CMapMenu, CToolMenuButton);

public:
	CMapMenu(vgui::Panel* pParent, const char* panelName, const char* text, vgui::Panel* pActionSignalTarget);

	virtual void OnShowMenu(vgui::Menu* menu);
};