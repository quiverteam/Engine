/*

menus.cpp

Menus for the map editor

*/

#include "menus.h"

//====================================================================================================//
// CEditMenu - Map Edit menu
//====================================================================================================//
CEditMenu::CEditMenu(vgui::Panel* pParent, const char* panelName, const char* text, vgui::Panel* pActionSignalTarget) :
	BaseClass(pParent, panelName, text, pActionSignalTarget)
{
	AddMenuItem("redo", "&Redo", new KeyValues("OnRedo"), pActionSignalTarget);
	AddMenuItem("undo", "&Undo", new KeyValues("OnUndo"), pActionSignalTarget);

	this->SetMenu(m_pMenu);
}

void CEditMenu::OnShowMenu(vgui::Menu* menu)
{
	BaseClass::OnShowMenu(menu);

	this->SetItemEnabled(this->m_Items.Find("Undo"), true);
	this->SetItemEnabled(this->m_Items.Find("Redo"), true);
}

//====================================================================================================//
// CMapMenu - Map map menu
//====================================================================================================//
CMapMenu::CMapMenu(vgui::Panel* pParent, const char* panelName, const char* text, vgui::Panel* pActionSignalTarget) :
	BaseClass(pParent, panelName, text, pActionSignalTarget)
{
	AddCheckableMenuItem("snaptogrid", "&Snap To Grid", new KeyValues("OnSnapToGrid"), pActionSignalTarget);
	AddCheckableMenuItem("showgrid", "&Show Grid", new KeyValues("OnShowGrid"), pActionSignalTarget);

	AddSeparator();

	AddMenuItem("showinfo", "&Show Map Info", new KeyValues("OnShowMapInfo"), pActionSignalTarget);
	AddMenuItem("showmapinfo", "&Map Properties", new KeyValues("OnEditMapProperties"), pActionSignalTarget);

	this->SetMenu(m_pMenu);
}

void CMapMenu::OnShowMenu(vgui::Menu* menu)
{
	BaseClass::OnShowMenu(menu);
}