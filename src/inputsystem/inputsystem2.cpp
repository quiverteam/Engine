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
#include <ctime>
#include <time.h>

//-----------------------------------------------------------------------------
// Singleton instance
//-----------------------------------------------------------------------------
static CInputSystem2 g_InputSystem2;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CInputSystem, IInputSystem,
								  INPUTSYSTEM_INTERFACE_VERSION, g_InputSystem2);

// Constructor, destructor
CInputSystem2::CInputSystem2()
{
	this->last_tick = 0;
	this->last_poll = 0;
	this->poll_count = 0;
	this->joystick_count = 0;
	this->enabled_joystick = 0;
	this->active_joystick = nullptr;
}

CInputSystem2::~CInputSystem2()
{

}

// Inherited from IAppSystem
InitReturnVal_t CInputSystem2::Init()
{
	DevWarning("WARNING: Inputsystem2 is currently loaded. This is experimental.");
	int flags = SDL_WasInit(0);
	if(!(flags & SDL_INIT_JOYSTICK))
		SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	if(!(flags & SDL_INIT_EVENTS))
		SDL_InitSubSystem(SDL_INIT_EVENTS);
	if(!(flags & SDL_INIT_GAMECONTROLLER))
		SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
	
	/* Find all joysticks */
	int numjoy = SDL_NumJoysticks();
	this->joystick_count = numjoy;

	memset(&this->prev_input, 0, sizeof(SInputState_t));
	memset(&this->curr_input, 0, sizeof(SInputState_t));

	return InitReturnVal_t::INIT_OK;
}
void CInputSystem2::Shutdown()
{
	SDL_QuitSubSystem(SDL_INIT_EVENTS);
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

void CInputSystem2::AttachToWindow(void* hWnd)
{
	// BIG NOTE: This requires using SDL deeper in the engine.
	Assert(hWnd);
	pWindow = (SDL_Window*)hWnd;
}

void CInputSystem2::DetachFromWindow()
{
	pWindow = NULL;
}

void CInputSystem2::EnableInput(bool bEnable)
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

void CInputSystem2::EnableMessagePump(bool bEnable)
{
	enable_input = bEnable;
}

/* Returns the last poll tick */
int CInputSystem2::GetPollTick() const
{
	return this->last_poll;
}

void CInputSystem2::PollInputState()
{

}

bool CInputSystem2::IsButtonDown(ButtonCode_t code) const
{
	return this->curr_input.keys[code];
}

int CInputSystem2::GetButtonPressedTick(ButtonCode_t code) const
{
	return this->curr_input.pressed_tick[code];
}

int CInputSystem2::GetButtonReleasedTick(ButtonCode_t code) const
{
	return this->curr_input.released_tick[code];
}

/*

Returns an analog value that corresponds to the position of some type of input
This is a RAW analog value, so if you need a relative value (such as a delta)
use GetAnalogDelta

*/
int CInputSystem2::GetAnalogValue(AnalogCode_t code) const
{
	return curr_input.axis[code];
}

/*

Return an analog delta for the given analog input.
Delta means change, so if you're querying MOUSE_X, you should get the amount the mouse has moved
on the X axis over the last frame. Use GetAnalogValue if you want a raw value instead of this.

*/
int CInputSystem2::GetAnalogDelta(AnalogCode_t code) const
{
	return curr_input.axis[code] - prev_input.axis[code];
}

/*

Return the number of events

*/
int CInputSystem2::GetEventCount() const
{
	return events.Count();
}

/*

Return the last bit of event data

*/
const InputEvent_t* CInputSystem2::GetEventData() const
{
	if(events.Count() > 0)
	{
		return &this->events.Tail();
	}
	return nullptr;
}

/*

Insert a user event into the input event queue

*/
void CInputSystem2::PostUserEvent(const InputEvent_t &event)
{
	this->events.AddToTail(event);
}

/*

Return the number of joysticks

*/
int CInputSystem2::GetJoystickCount() const
{
	return this->joystick_count;
}

/*

Enable joystick input from the specified joystick

*/
void CInputSystem2::EnableJoystickInput(int nJoystick, bool bEnable)
{
	/* Can only disable active joystick */
	if(!bEnable && this->enabled_joystick == nJoystick && this->active_joystick)
	{
		SDL_JoystickClose(this->active_joystick);
		this->active_joystick = nullptr;
		return;
	}

	if(nJoystick < this->joystick_count)
	{
		if(!bEnable) return;
		if(this->active_joystick)
			SDL_JoystickClose(this->active_joystick);
		this->enabled_joystick = nJoystick;
		this->active_joystick = SDL_JoystickOpen(nJoystick);
		if(!this->active_joystick)
			Error("[Inputsystem2] Failed to open joystick. SDL_Error=%s", SDL_GetError());
	}
}

/*

Enable joystick diagonal pov

*/
void CInputSystem2::EnableJoystickDiagonalPOV(int nJoystick, bool bEnable)
{

}

/* Sample for unregistered input events and place them into the event queue */
void CInputSystem2::SampleDevices(void)
{
	last_tick = (clock() / CLOCKS_PER_SEC) * 1000;
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

void CInputSystem2::SetRumble(float fLeftMotor, float fRightMotor, int userId)
{
	/* Nothing to be done, x360 only */
}

void CInputSystem2::StopRumble(void)
{
	/* Nothing to be done, x360 only */
}

void CInputSystem2::ResetInputState(void)
{
	memset(&this->curr_input, 0, sizeof(SInputState_t));
	memset(&this->prev_input, 0, sizeof(SInputState_t));
}

void CInputSystem2::SetPrimaryUserId(int userId)
{

}

const char* CInputSystem2::ButtonCodeToString(ButtonCode_t code) const
{

}

const char* CInputSystem2::AnalogCodeToString(AnalogCode_t code) const
{

}

ButtonCode_t CInputSystem2::StringToButtonCode(const char *pString) const
{

}

AnalogCode_t CInputSystem2::StringToAnalogCode(const char *pString) const
{

}

ButtonCode_t CInputSystem2::VirtualKeyToButtonCode(int nVirtualKey) const
{

}

int CInputSystem2::ButtonCodeToVirtualKey(ButtonCode_t code) const
{

}

ButtonCode_t CInputSystem2::ScanCodeToButtonCode(int lParam) const
{

}

void CInputSystem2::SleepUntilInput(int nMaxSleepTimeMS)
{

}

int CInputSystem2::GetPollCount() const
{
	return poll_count;
}

void CInputSystem2::SetCursorPosition(int x, int y)
{
	Assert(pWindow);
	SDL_WarpMouseInWindow(pWindow, x, y);
}
