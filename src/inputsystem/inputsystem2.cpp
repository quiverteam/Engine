//===========================================================================//
// Name: inputsystem2.cpp
// Purpose: Version 2 of source inputsystem
// Date Created: July 16, 2019
// Authors: JJL77 jeremy.lorelli.1337@gmail.com
//===========================================================================//

#include "inputsystem.h"
#include "tier0/platform.h"
#include "tier1/convar.h"
#include "SDLInputMapping.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_gamecontroller.h>

//-----------------------------------------------------------------------------
// Singleton instance
//-----------------------------------------------------------------------------
static CInputSystem g_InputSystem;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CInputSystem, IInputSystem,
								  INPUTSYSTEM_INTERFACE_VERSION, g_InputSystem);

// Constructor, destructor
CInputSystem::CInputSystem()
{

}

CInputSystem::~CInputSystem()
{

}

// Inherited from IAppSystem
InitReturnVal_t Init()
{
	DevWarning("WARNING: Inputsystem2 is currently loaded. This is experimental.");
	SDL_InitSubSystem(SDL_INIT_EVENTS);
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
}
void CInputSystem::Shutdown()
{
	SDL_QuitSubSystem(SDL_INIT_EVENTS);
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

void CInputSystem::AttachToWindow(void* hWnd)
{
	// BIG NOTE: This requires using SDL deeper in the engine.
	Assert(hWnd);
	pWindow = (SDL_Window*)hWnd;
}

void CInputSystem::DetachFromWindow()
{
	pWindow = NULL;
}

void CInputSystem::EnableInput(bool bEnable)
{
	if(bEnable)
	{
		SDL_CaptureMouse(SDL_TRUE);
		SDL_SetRelativeMouseMode(SDL_TRUE);

	}
	else
	{
		SDL_CaptureMouse(SDL_FALSE);
	}
}

void CInputSystem::EnableMessagePump(bool bEnable)
{
	m_bPumpEnabled = bEnable;
}

/* Returns the last poll tick */
int CInputSystem::GetPollTick() const
{

}

void CInputSystem::PollInputState()
{

}

bool CInputSystem::IsButtonDown(ButtonCode_t code) const
{

}

int CInputSystem::GetButtonPressedTick(ButtonCode_t code) const
{

}

int CInputSystem::GetButtonReleasedTick(ButtonCode_t code) const
{

}

/*

Returns an analog value that corresponds to the position of some type of input
This is a RAW analog value, so if you need a relative value (such as a delta)
use GetAnalogDelta

*/
int CInputSystem::GetAnalogValue(AnalogCode_t code) const
{
	int x = 0, y = 0;
	switch(code)
	{
		case MOUSE_X:
			SDL_GetRelativeMouseState(&x, &y);
			return x;
			break;
		case MOUSE_Y:
			SDL_GetRelativeMouseState(&x, &y);
			return y;
			break;
		case MOUSE_XY:
			break;
		case MOUSE_WHEEL:
			break;
		case JOYSTICK_FIRST_AXIS:
			break;
		case JOYSTICK_LAST_AXIS:
			break;
		default:
			return 0;
	}
}

/*

Return an analog delta for the given analog input.
Delta means change, so if you're querying MOUSE_X, you should get the amount the mouse has moved
on the X axis over the last frame. Use GetAnalogValue if you want a raw value instead of this.

*/
int CInputSystem::GetAnalogDelta(AnalogCode_t code) const
{

}

/*

Return the number of events

*/
int CInputSystem::GetEventCount() const
{
	return m_InputEvents.Count();
}

/*

Return the last bit of event data

*/
const InputEvent_t* CInputSystem::GetEventData() const
{
	if(m_InputEvents.Count() > 0)
	{
		return &m_InputEvents.Tail();
	}
}

/*

Insert a user event into the input event queue

*/
void CInputSystem::PostUserEvent(const InputEvent_t &event)
{
	m_InputEvents.AddToTail(event);
}

/*

Return the number of joysticks

*/
int CInputSystem::GetJoystickCount() const
{
	return SDL_NumJoysticks();
}

/*

Enable joystick input from the specified joystick

*/
void CInputSystem::EnableJoystickInput(int nJoystick, bool bEnable)
{

}

/*

Enable joystick diagonal pov

*/
void CInputSystem::EnableJoystickDiagonalPOV(int nJoystick, bool bEnable)
{

}

/* Sample for unregistered input events and place them into the event queue */
void CInputSystem::SampleDevices(void)
{
	m_nLastPollTick = Plat_GetTickCount();
	SDL_PumpEvents();

	SDL_Event ev[256];
	for(int i = SDL_PeepEvents(ev, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT), n = 0; n < i; n++)
	{
		switch(ev[n].type)
		{
			case SDL_KEYUP:
			{
				SDL_KeyboardEvent event = ev[n].key;
				
				break;
			}
			case SDL_KEYDOWN:
			{
				break;
			}
			case SDL_MOUSEMOTION:
			{
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			{
				break;
			}
			case SDL_MOUSEBUTTONUP:
			{
				break;
			}
			case SDL_MOUSEWHEEL:
			{
				break;
			}
		}
	}

}

void CInputSystem::SetRumble(float fLeftMotor, float fRightMotor, int userId)
{
	/* Nothing to be done, x360 only */
}

void CInputSystem::StopRumble(void)
{
	/* Nothing to be done, x360 only */
}

void CInputSystem::ResetInputState(void)
{

}

void CInputSystem::SetPrimaryUserId(int userId)
{

}

const char* CInputSystem::ButtonCodeToString(ButtonCode_t code) const
{

}

const char* CInputSystem::AnalogCodeToString(AnalogCode_t code) const
{

}

ButtonCode_t CInputSystem::StringToButtonCode(const char *pString) const
{

}

AnalogCode_t CInputSystem::StringToAnalogCode(const char *pString) const
{

}

ButtonCode_t CInputSystem::VirtualKeyToButtonCode(int nVirtualKey) const
{

}

int CInputSystem::ButtonCodeToVirtualKey(ButtonCode_t code) const
{

}

ButtonCode_t CInputSystem::ScanCodeToButtonCode(int lParam) const
{

}

void CInputSystem::SleepUntilInput(int nMaxSleepTimeMS)
{

}

int CInputSystem::GetPollCount() const
{
	return m_nPollCount;
}

void CInputSystem::SetCursorPosition(int x, int y)
{
	Assert(pWindow);
	SDL_WarpMouseInWindow(pWindow, x, y);
}