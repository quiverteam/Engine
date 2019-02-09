//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Standard file menu
//
//=============================================================================


#ifndef TOOLFILEMENUBUTTON_H
#define TOOLFILEMENUBUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include "toolutils/toolmenubutton.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
namespace vgui
{
class Panel;
class Menu;
}

class CToolMenuButton;


//-----------------------------------------------------------------------------
// Called back by the file menu 
//-----------------------------------------------------------------------------
class IFileMenuCallbacks
{
public:
	enum MenuItems_t
	{
		FILE_NEW	= 0x01,
		FILE_OPEN	= 0x02,
		FILE_SAVE	= 0x04,
		FILE_SAVEAS = 0x08,
		FILE_CLOSE	= 0x10,
		FILE_RECENT	= 0x20,
		FILE_CLEAR_RECENT	= 0x40,
		FILE_EXIT	= 0x80,

		FILE_ALL = 0xFFFFFFFF
	};

	// Logically OR together all items that should be enabled
	virtual int	 GetFileMenuItemsEnabled( ) = 0;

	// Add recent files to the menu passed in
	virtual void AddRecentFilesToMenu( vgui::Menu *menu ) = 0;

	// Gets the root vgui panel
	virtual vgui::Panel *GetRootPanel() = 0;
};


//-----------------------------------------------------------------------------
// Standard file menu
//-----------------------------------------------------------------------------
class CToolFileMenuButton : public CToolMenuButton
{
	DECLARE_CLASS_SIMPLE( CToolFileMenuButton, CToolMenuButton );
public:

	CToolFileMenuButton( vgui::Panel *parent, const char *panelName, const char *text, vgui::Panel *pActionTarget, IFileMenuCallbacks *pFileMenuCallback );
	virtual void OnShowMenu( vgui::Menu *menu );

private:

	vgui::Menu			*m_pRecentFiles;
	int					m_nRecentFiles;
	IFileMenuCallbacks *m_pFileMenuCallback;
};


//-----------------------------------------------------------------------------
// Global function to create the switch menu
//-----------------------------------------------------------------------------
CToolMenuButton* CreateToolFileMenuButton( vgui::Panel *parent, const char *panelName, 
	const char *text, vgui::Panel *pActionTarget, IFileMenuCallbacks *pCallbacks );

#endif // TOOLFILEMENUBUTTON_H