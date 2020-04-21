//===========================================================================//
// Name: inputsystem2.cpp
// Purpose: Version 2 of source inputsystem
// Date Created: July 16, 2019
// Authors: JJL77 jeremy.lorelli.1337@gmail.com
//===========================================================================//
#ifndef USE_OLD_INPUTSYSTEM

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
	return (int)curr_input.axis[code];
}

/*

Return an analog delta for the given analog input.
Delta means change, so if you're querying MOUSE_X, you should get the amount the mouse has moved
on the X axis over the last frame. Use GetAnalogValue if you want a raw value instead of this.

*/
int CInputSystem2::GetAnalogDelta(AnalogCode_t code) const
{
	return (int)(curr_input.axis[code] - prev_input.axis[code]);
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
	return (int)this->joystick_count;
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

/* Polls input state an updates internal buffers */
/* This SHOULD empty out the event queue */
void CInputSystem2::PollInputState()
{
	this->last_poll = (clock() / CLOCKS_PER_SEC) * 1000;
	this->poll_count++;
	this->polling = true;
	/* Swap current and former input state */
	memcpy(&this->prev_input, &this->curr_input, sizeof(SInputState_t));
	SampleDevices(); /* Sample joysticks */
	SDL_PumpEvents();
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		switch(ev.type)
		{
			/* Key events */
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				int key = SDLKeycodeToButtonCode(ev.key.keysym.scancode);
				this->curr_input.keys[key] = (ev.type != SDL_KEYUP);
				if(ev.type == SDL_KEYDOWN)
					this->curr_input.pressed_tick[key] = ev.key.timestamp;
				else
					this->curr_input.released_tick[key] = ev.key.timestamp;
				break;
			}
			/* Handle mouse motion events */
			case SDL_MOUSEMOTION:
			{
				/* According to the SDL2 wiki, these are relative to the window, meaning it's not DELTA x and y */
				this->curr_input.mouse_x = ev.motion.x;
				this->curr_input.mouse_y = ev.motion.y;
				break;
			}
			/* Mouse button events */
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				static int map[SDL_BUTTON_X2+1] = {
						ButtonCode_t::MOUSE_LEFT-ButtonCode_t::MOUSE_FIRST,
						ButtonCode_t::MOUSE_MIDDLE-ButtonCode_t::MOUSE_FIRST,
						ButtonCode_t::MOUSE_RIGHT-ButtonCode_t::MOUSE_FIRST,
						ButtonCode_t::MOUSE_4-ButtonCode_t::MOUSE_FIRST,
						ButtonCode_t::MOUSE_5-ButtonCode_t::MOUSE_FIRST
				};
				this->curr_input.mouse[map[ev.button.button]] = (ev.type != SDL_MOUSEBUTTONUP);
				break;
			}
			/* Mouse wheel events */
			case SDL_MOUSEWHEEL:
			{
				if(ev.wheel.y > 0)
					this->curr_input.mouse[ButtonCode_t::MOUSE_WHEEL_UP-ButtonCode_t::MOUSE_FIRST] = true;
				else if(ev.wheel.y < 0)
					this->curr_input.mouse[ButtonCode_t::MOUSE_WHEEL_DOWN-ButtonCode_t::MOUSE_FIRST] = true;
				else
				{
					this->curr_input.mouse[ButtonCode_t::MOUSE_WHEEL_DOWN-ButtonCode_t::MOUSE_FIRST] = false;
					this->curr_input.mouse[ButtonCode_t::MOUSE_WHEEL_UP-ButtonCode_t::MOUSE_FIRST] = false;
				}
				break;
			}
			default: break;
		}
	}
	/* Grab all posted user events and remove them from the queue */
	/* TODO: This needs to be extended to handle all forms of input */
	for(auto x : this->events)
	{
		switch(x.m_nType)
		{
			case IE_ButtonPressed:
			case IE_ButtonReleased:
			{
				curr_input.keys[x.m_nData] = (IE_ButtonPressed == x.m_nType);
				break;
			}
			default: break;
		}
	}
	this->events.Purge();
	this->polling = false;
}


/* Sample for unregistered input events and place them into the event queue */
/* Should only be done for joysticks/controllers */
void CInputSystem2::SampleDevices(void)
{
	last_tick = (clock() / CLOCKS_PER_SEC) * 1000;
	SDL_PumpEvents();
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		switch(ev.type)
		{
			case SDL_JOYAXISMOTION:
			{
				if(ev.jaxis.which == this->enabled_joystick)
					this->curr_input.axis[ev.jaxis.axis] = ev.jaxis.value;
			}
			case SDL_JOYBALLMOTION: break; /* what even is this lol */
			case SDL_JOYBUTTONUP: /* Joystick buttons are not yet supported for this version of inputsystem (thanks vavel) */
			case SDL_JOYBUTTONDOWN:
			default: break;
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
	/* There should be a somewhat better way to do this, but whatever */
	while (!SDL_HasEvents(SDL_FIRSTEVENT, SDL_LASTEVENT) && nMaxSleepTimeMS > 0)
	{
		Plat_USleep(1000);
		nMaxSleepTimeMS--;
	}
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

#endif //USE_OLD_INPUTSYSTEM