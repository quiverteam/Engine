//====== Copyright 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: New input system using SDL2
//
//===========================================================================//

#include "inputsystem.h"
#include "tier0/platform.h"
#include "tier1/convar.h"

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

int CInputSystem::GetAnalogValue(AnalogCode_t code) const
{
	switch(code)
	{
		case MOUSE_X:
			int x = 0, y = 0;
			SDL_GetRelativeMouseState(&x, &y);
			return x;
		case MOUSE_Y:
			int x = 0, y = 0;
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

int CInputSystem::GetAnalogDelta(AnalogCode_t code) const
{

}

int CInputSystem::GetEventCount() const
{

}

const InputEvent_t* CInputSystem::GetEventData() const
{

}

void CInputSystem::PostUserEvent(const InputEvent_t &event)
{

}

int CInputSystem::GetJoystickCount() const
{
	return SDL_NumJoysticks();
}

void CInputSystem::EnableJoystickInput(int nJoystick, bool bEnable)
{

}

void CInputSystem::EnableJoystickDiagonalPOV(int nJoystick, bool bEnable)
{

}

/* Sample for unregistered input events and place them into the even queue */
void CInputSystem::SampleDevices(void)
{
	m_nLastPollTick = Plat_GetTickCount();
	SDL_PumpEvents();

	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		switch(ev.type)
		{
			case SDL_KEYUP:
			{
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