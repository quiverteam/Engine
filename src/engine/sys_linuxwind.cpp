//========= Copyright (C) 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Linux support for the IGame interface
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=======================================================================================//
#include "iengine.h"
#include <stdlib.h>

#include "engine_launcher_api.h"
#include "basetypes.h"
#include "ivideomode.h"
#include "igame.h"

/* SDL2 includes */
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>

#define HWND int
#define UINT unsigned int
#define WPARAM int
#define LPARAM int

#include "profile.h"
#include "server.h"
#include "cdll_int.h"

/*
 * The IGame interface will most likely need some modification in the future.
 * We have no way of asking for a list of displays, nor do we have a way to set the
 * active display. In addition, since we're moving to SDL, the Win32 version of this (sys_mainwind.cpp) will probably just get ported.
 */

void ForceReloadProfile( void );

void ClearIOStates( void );
//-----------------------------------------------------------------------------
// Purpose: Main game interface, including message pump and window creation
//-----------------------------------------------------------------------------
class CGame : public IGame
{
public:
	SDL_Window* pWindow;
	SDL_DisplayMode** pDisplayModes;
	int *nDisplayModes, nCurrDispMode, nDispIndex, nDisplays;
	char* pWindowName;

	int x,y,w,h;

public:
					CGame( void );
	virtual			~CGame( void );

	bool			Init( void *pvInstance );
	bool			Shutdown( void );

	bool			CreateGameWindow( void );
	virtual void	DestroyGameWindow( void );
	virtual void	SetGameWindow( void *hWnd );

	virtual bool	InputAttachToGameWindow();
	virtual void	InputDetachFromGameWindow();

	void*			GetMainWindow( void );
	void**			GetMainWindowAddress( void );

	void			SetWindowXY( int x, int y );
	void			SetWindowSize( int w, int h );
	void			GetWindowRect( int *x, int *y, int *w, int *h );

	bool			IsActiveApp( void );
	virtual void		DispatchAllStoredGameMessages();
	virtual void		PlayStartupVideos() {}
	virtual void		GetDesktopInfo( int &width, int &height, int &refreshRate );

private:
	void			SetActiveApp( bool fActive );

private:
	bool m_bActiveApp;
	static const char CLASSNAME[];

};

static CGame g_Game;
IGame *game = ( IGame * )&g_Game;

const char CGame::CLASSNAME[] = "Valve001";

// In VCR playback mode, it sleeps this amount each frame.
int g_iVCRPlaybackSleepInterval = 0;

// During VCR playback, if this is true, then it'll pause at the end of each frame.
bool g_bVCRSingleStep = false;

void VCR_EnterPausedState()
{
        // Turn this off in case they're in single-step mode.
        g_bVCRSingleStep = false;

        // This is cheesy, but GetAsyncKeyState is blocked (in protected_things. h)
        // from being accidentally used, so we get it through it by getting its pointer directly.

         // In this mode, we enter a wait state where we only pay attention to R and Q.
 /*        while ( 1 )
         {
                 if ( pfn( 'R' ) & 0x8000 )
                        break;

                if ( pfn( 'Q' ) & 0x8000 )
                      kill( getpid(), SIGKILL );

                if ( pfn( 'S' ) & 0x8000 )
                {
                        // Do a single step.
                        g_bVCRSingleStep = true;
                        break;
                }

                Sleep( 2 );
        }
*/
}

CGame::CGame( void ) :
		pWindow(0),
		pWindowName(0),
		nDisplayModes(0),
		pDisplayModes(0),
		nCurrDispMode(0),
		nDispIndex(0),
		nDisplays(-1)
{
	m_bActiveApp = true;
}

CGame::~CGame( void )
{
	if(pWindowName) free(pWindowName);
}

bool CGame::CreateGameWindow( void )
{
	/* First, read some info from the gameinfo.txt for our window title */
	KeyValues* pKV = new KeyValues("ModInfo");
	if(pKV->LoadFromFile(g_pFullFileSystem, "gameinfo.txt"))
	{
		pWindowName = strdup(pKV->GetString("game"));
	}
	else
	{
		Warning("Couldn't get game from gameinfo.txt, using default window title.\n");
		pWindowName = strdup("hl2");
	}
	pKV->deleteThis();

	/* Set the flags */
	unsigned int flags = SDL_WINDOW_OPENGL; /* TODO: This might need to get changed in the future to support vulkan */
	if(videomode->IsNoborderWindowMode())
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	else
		flags |= SDL_WINDOW_FULLSCREEN;

	/* Actually create the widow now */
	this->pWindow = SDL_CreateWindow(this->pWindowName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			1000, 1000, flags);
	this->w = 1000;
	this->h = 1000;

	if(!this->pWindow)
	{
		Error("Failed to create main window.\n");
		return false;
	}

	/* Get the window position */
	SDL_GetWindowPosition(this->pWindow, &this->w, &this->h);

	Msg("Created window: title=\"%s\", flags=%u, w=%i, h=%i, x=%i, y=%i", this->pWindowName, flags,
			this->w, this->h, this->x, this->y);

	return true;
}

void CGame::DestroyGameWindow( void )
{
	if(this->pWindow)
		SDL_DestroyWindow(this->pWindow);
}


// This is used in edit mode to override the default wnd proc associated w/
bool CGame::InputAttachToGameWindow()
{
	return true;
}

void CGame::InputDetachFromGameWindow()
{
}

void CGame::SetGameWindow( void *hWnd )
{
	// stub?
}

bool CGame::Init( void *pvInstance )
{
	return true;
}

bool CGame::Shutdown( void )
{
	return true;
}

void *CGame::GetMainWindow( void )
{
	return this->pWindow;
}

void **CGame::GetMainWindowAddress( void )
{
	return reinterpret_cast<void**>(&this->pWindow);
}

void CGame::SetWindowXY( int x, int y )
{
	if(this->pWindow)
	{
		SDL_SetWindowPosition(this->pWindow, x, y);
		Msg("Set window pos: x=%i, y=%i\n", x, y);
		this->x = x;
		this->y = y;
	}
}

void CGame::SetWindowSize( int w, int h )
{
	if(this->pWindow)
	{
		SDL_SetWindowSize(this->pWindow, w, h);
		Msg("Set window size: w=%i, h=%i\n", w, h);
		this->w = w;
		this->h = h;
	}
}

void CGame::GetWindowRect( int *x, int *y, int *w, int *h )
{
	if ( x )
		*x = this->x;
	if ( y )
		*y = this->y;
	if ( w )
		*w = this->w;
	if ( h )
		*h = this->h;
}

bool CGame::IsActiveApp( void )
{
	return m_bActiveApp;
}

void CGame::SetActiveApp( bool active )
{
	m_bActiveApp = active;
}

void CGame::DispatchAllStoredGameMessages()
{
}

void CGame::GetDesktopInfo( int &width, int &height, int &refreshRate )
{
	SDL_DisplayMode mode;
	SDL_GetDesktopDisplayMode(0, &mode);
    width = mode.w;
    height = mode.h;
	refreshRate = mode.refresh_rate;
}


