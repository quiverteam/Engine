//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef BUTTONCODE_H
#define BUTTONCODE_H

#ifdef _WIN32
#pragma once
#endif

#include "inputsystem/InputEnums.h"
#include "mathlib/mathlib.h"

#ifndef USE_OLD_INPUTSYSTEM
#include <SDL2/SDL.h>
#include <SDL2/SDL_scancode.h>

/* quick remap for sdl2 inputsys */
#define MOUSETOSDL(x) (x-513)
#define SDLTOMOUSE(x) (x+513)

#endif



//-----------------------------------------------------------------------------
// Button enum. "Buttons" are binary-state input devices (mouse buttons, keyboard keys)
//-----------------------------------------------------------------------------
enum
{
	JOYSTICK_MAX_BUTTON_COUNT = 32,
	JOYSTICK_POV_BUTTON_COUNT = 4,
	JOYSTICK_AXIS_BUTTON_COUNT = MAX_JOYSTICK_AXES * 2,
};

#define JOYSTICK_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_BUTTON + ((_joystick) * JOYSTICK_MAX_BUTTON_COUNT) + (_button) )
#define JOYSTICK_POV_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_POV_BUTTON + ((_joystick) * JOYSTICK_POV_BUTTON_COUNT) + (_button) )
#define JOYSTICK_AXIS_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_AXIS_BUTTON + ((_joystick) * JOYSTICK_AXIS_BUTTON_COUNT) + (_button) )

#define JOYSTICK_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_BUTTON_INTERNAL( _joystick, _button ) )
#define JOYSTICK_POV_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_POV_BUTTON_INTERNAL( _joystick, _button ) )
#define JOYSTICK_AXIS_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_AXIS_BUTTON_INTERNAL( _joystick, _button ) )

enum ButtonCode_t
{
	BUTTON_CODE_INVALID = -1,
	BUTTON_CODE_NONE = 0,

	KEY_FIRST = 0,

#ifdef USE_OLD_INPUTSYSTEM

	KEY_NONE = KEY_FIRST,
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_PAD_0,
	KEY_PAD_1,
	KEY_PAD_2,
	KEY_PAD_3,
	KEY_PAD_4,
	KEY_PAD_5,
	KEY_PAD_6,
	KEY_PAD_7,
	KEY_PAD_8,
	KEY_PAD_9,
	KEY_PAD_DIVIDE,
	KEY_PAD_MULTIPLY,
	KEY_PAD_MINUS,
	KEY_PAD_PLUS,
	KEY_PAD_ENTER,
	KEY_PAD_DECIMAL,
	KEY_LBRACKET,
	KEY_RBRACKET,
	KEY_SEMICOLON,
	KEY_APOSTROPHE,
	KEY_BACKQUOTE,
	KEY_COMMA,
	KEY_PERIOD,
	KEY_SLASH,
	KEY_BACKSLASH,
	KEY_MINUS,
	KEY_EQUAL,
	KEY_ENTER,
	KEY_SPACE,
	KEY_BACKSPACE,
	KEY_TAB,
	KEY_CAPSLOCK,
	KEY_NUMLOCK,
	KEY_ESCAPE,
	KEY_SCROLLLOCK,
	KEY_INSERT,
	KEY_DELETE,
	KEY_HOME,
	KEY_END,
	KEY_PAGEUP,
	KEY_PAGEDOWN,
	KEY_BREAK,
	KEY_LSHIFT,
	KEY_RSHIFT,
	KEY_LALT,
	KEY_RALT,
	KEY_LCONTROL,
	KEY_RCONTROL,
	KEY_LWIN,
	KEY_RWIN,
	KEY_APP,
	KEY_UP,
	KEY_LEFT,
	KEY_DOWN,
	KEY_RIGHT,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_CAPSLOCKTOGGLE,
	KEY_NUMLOCKTOGGLE,
	KEY_SCROLLLOCKTOGGLE,

	KEY_LAST = KEY_SCROLLLOCKTOGGLE,
	KEY_COUNT = KEY_LAST - KEY_FIRST + 1,

	// Mouse
	MOUSE_FIRST = KEY_LAST + 1,

	MOUSE_LEFT = MOUSE_FIRST,
	MOUSE_RIGHT,
	MOUSE_MIDDLE,
	MOUSE_4,
	MOUSE_5,
	MOUSE_WHEEL_UP,		// A fake button which is 'pressed' and 'released' when the wheel is moved up 
	MOUSE_WHEEL_DOWN,	// A fake button which is 'pressed' and 'released' when the wheel is moved down

	MOUSE_LAST = MOUSE_WHEEL_DOWN,
	MOUSE_COUNT = MOUSE_LAST - MOUSE_FIRST + 1,

#else
	/* These keycodes exist only for SDL2 Inputsystem */
	KEY_NONE = KEY_FIRST,
	KEY_0 = SDL_SCANCODE_O,
	KEY_1 = SDL_SCANCODE_1,
	KEY_2 = SDL_SCANCODE_2,
	KEY_3 = SDL_SCANCODE_3,
	KEY_4 = SDL_SCANCODE_4,
	KEY_5 = SDL_SCANCODE_5,
	KEY_6 = SDL_SCANCODE_6,
	KEY_7 = SDL_SCANCODE_7,
	KEY_8 = SDL_SCANCODE_8,
	KEY_9 = SDL_SCANCODE_9,
	KEY_A = SDL_SCANCODE_A,
	KEY_B = SDL_SCANCODE_B,
	KEY_C = SDL_SCANCODE_C,
	KEY_D = SDL_SCANCODE_D,
	KEY_E = SDL_SCANCODE_E,
	KEY_F = SDL_SCANCODE_F,
	KEY_G = SDL_SCANCODE_G,
	KEY_H = SDL_SCANCODE_H,
	KEY_I = SDL_SCANCODE_I,
	KEY_J = SDL_SCANCODE_J,
	KEY_K = SDL_SCANCODE_K,
	KEY_L = SDL_SCANCODE_L,
	KEY_M = SDL_SCANCODE_M,
	KEY_N = SDL_SCANCODE_N,
	KEY_O = SDL_SCANCODE_O,
	KEY_P = SDL_SCANCODE_P,
	KEY_Q = SDL_SCANCODE_Q,
	KEY_R = SDL_SCANCODE_R,
	KEY_S = SDL_SCANCODE_S,
	KEY_T = SDL_SCANCODE_T,
	KEY_U = SDL_SCANCODE_U,
	KEY_V = SDL_SCANCODE_V,
	KEY_W = SDL_SCANCODE_W,
	KEY_X = SDL_SCANCODE_X,
	KEY_Y = SDL_SCANCODE_Y,
	KEY_Z = SDL_SCANCODE_Z,
	KEY_PAD_0 = SDL_SCANCODE_KP_0,
	KEY_PAD_1 = SDL_SCANCODE_KP_1,
	KEY_PAD_2 = SDL_SCANCODE_KP_2,
	KEY_PAD_3 = SDL_SCANCODE_KP_3,
	KEY_PAD_4 = SDL_SCANCODE_KP_4,
	KEY_PAD_5 = SDL_SCANCODE_KP_5,
	KEY_PAD_6 = SDL_SCANCODE_KP_6,
	KEY_PAD_7 = SDL_SCANCODE_KP_7,
	KEY_PAD_8 = SDL_SCANCODE_KP_8,
	KEY_PAD_9 = SDL_SCANCODE_KP_9,
	KEY_PAD_DIVIDE = SDL_SCANCODE_KP_DIVIDE,
	KEY_PAD_MULTIPLY = SDL_SCANCODE_KP_MULTIPLY,
	KEY_PAD_MINUS = SDL_SCANCODE_KP_MINUS,
	KEY_PAD_PLUS = SDL_SCANCODE_KP_PLUS,
	KEY_PAD_ENTER = SDL_SCANCODE_KP_ENTER,
	KEY_PAD_DECIMAL = SDL_SCANCODE_KP_DECIMAL,
	KEY_LBRACKET = SDL_SCANCODE_LEFTBRACKET,
	KEY_RBRACKET = SDL_SCANCODE_RIGHTBRACKET,
	KEY_SEMICOLON = SDL_SCANCODE_SEMICOLON,
	KEY_APOSTROPHE = SDL_SCANCODE_APOSTROPHE,
	KEY_BACKQUOTE = SDL_SCANCODE_APOSTROPHE,
	KEY_COMMA = SDL_SCANCODE_COMMA,
	KEY_PERIOD = SDL_SCANCODE_PERIOD,
	KEY_SLASH  = SDL_SCANCODE_SLASH,
	KEY_BACKSLASH = SDL_SCANCODE_BACKSLASH,
	KEY_MINUS = SDL_SCANCODE_MINUS,
	KEY_EQUAL = SDL_SCANCODE_EQUALS,
	KEY_ENTER = SDL_SCANCODE_EXECUTE,
	KEY_SPACE = SDL_SCANCODE_SPACE,
	KEY_BACKSPACE = SDL_SCANCODE_BACKSPACE,
	KEY_TAB = SDL_SCANCODE_TAB,
	KEY_CAPSLOCK = SDL_SCANCODE_CAPSLOCK,
	KEY_NUMLOCK = SDL_SCANCODE_NUMLOCKCLEAR,
	KEY_ESCAPE = SDL_SCANCODE_ESCAPE,
	KEY_SCROLLLOCK = SDL_SCANCODE_SCROLLLOCK,
	KEY_INSERT = SDL_SCANCODE_INSERT,
	KEY_DELETE = SDL_SCANCODE_DELETE,
	KEY_HOME = SDL_SCANCODE_HOME,
	KEY_END = SDL_SCANCODE_END,
	KEY_PAGEUP = SDL_SCANCODE_PAGEUP,
	KEY_PAGEDOWN = SDL_SCANCODE_PAGEDOWN,
	KEY_BREAK = SDL_SCANCODE_PAUSE,
	KEY_LSHIFT = SDL_SCANCODE_LSHIFT,
	KEY_RSHIFT = SDL_SCANCODE_RSHIFT,
	KEY_LALT = SDL_SCANCODE_LALT,
	KEY_RALT = SDL_SCANCODE_RALT,
	KEY_LCONTROL = SDL_SCANCODE_LCTRL,
	KEY_RCONTROL = SDL_SCANCODE_RCTRL,
	KEY_LWIN = SDL_SCANCODE_LGUI,
	KEY_RWIN = SDL_SCANCODE_RGUI,
	KEY_APP = SDL_SCANCODE_APPLICATION,
	KEY_UP = SDL_SCANCODE_UP,
	KEY_LEFT = SDL_SCANCODE_LEFT,
	KEY_DOWN = SDL_SCANCODE_DOWN,
	KEY_RIGHT = SDL_SCANCODE_RIGHT,
	KEY_F1 = SDL_SCANCODE_F1,
	KEY_F2 = SDL_SCANCODE_F2,
	KEY_F3 = SDL_SCANCODE_F3,
	KEY_F4 = SDL_SCANCODE_F4,
	KEY_F5 = SDL_SCANCODE_F5,
	KEY_F6 = SDL_SCANCODE_F6,
	KEY_F7 = SDL_SCANCODE_F7,
	KEY_F8 = SDL_SCANCODE_F8,
	KEY_F9 = SDL_SCANCODE_F9,
	KEY_F10 = SDL_SCANCODE_F10,
	KEY_F11 = SDL_SCANCODE_F11,
	KEY_F12 = SDL_SCANCODE_F12,
	KEY_CAPSLOCKTOGGLE = SDL_SCANCODE_CAPSLOCK,
	KEY_NUMLOCKTOGGLE = SDL_SCANCODE_NUMLOCKCLEAR,
	KEY_SCROLLLOCKTOGGLE = SDL_SCANCODE_SCROLLLOCK,

	KEY_LAST = SDL_NUM_SCANCODES,

	KEY_COUNT = KEY_LAST - KEY_FIRST + 1,

	// Mouse
	MOUSE_FIRST = KEY_LAST + 1,

	MOUSE_LEFT = MOUSE_FIRST,
	MOUSE_MIDDLE,
	MOUSE_RIGHT,
	MOUSE_4,
	MOUSE_5,
	MOUSE_WHEEL_UP,		// A fake button which is 'pressed' and 'released' when the wheel is moved up 
	MOUSE_WHEEL_DOWN,	// A fake button which is 'pressed' and 'released' when the wheel is moved down

	MOUSE_LAST = MOUSE_WHEEL_DOWN,
	MOUSE_COUNT = MOUSE_LAST - MOUSE_FIRST + 1,
#endif

	// Joystick
	JOYSTICK_FIRST = MOUSE_LAST + 1,

	JOYSTICK_FIRST_BUTTON = JOYSTICK_FIRST,
	JOYSTICK_LAST_BUTTON = JOYSTICK_BUTTON_INTERNAL( MAX_JOYSTICKS-1, JOYSTICK_MAX_BUTTON_COUNT-1 ),
	JOYSTICK_FIRST_POV_BUTTON,
	JOYSTICK_LAST_POV_BUTTON = JOYSTICK_POV_BUTTON_INTERNAL( MAX_JOYSTICKS-1, JOYSTICK_POV_BUTTON_COUNT-1 ),
	JOYSTICK_FIRST_AXIS_BUTTON,
	JOYSTICK_LAST_AXIS_BUTTON = JOYSTICK_AXIS_BUTTON_INTERNAL( MAX_JOYSTICKS-1, JOYSTICK_AXIS_BUTTON_COUNT-1 ),

	JOYSTICK_LAST = JOYSTICK_LAST_AXIS_BUTTON,

	BUTTON_CODE_LAST,
	BUTTON_CODE_COUNT = BUTTON_CODE_LAST - KEY_FIRST + 1,

	// Helpers for XBox 360
	KEY_XBUTTON_UP = JOYSTICK_FIRST_POV_BUTTON,	// POV buttons
	KEY_XBUTTON_RIGHT,
	KEY_XBUTTON_DOWN,
	KEY_XBUTTON_LEFT,

	KEY_XBUTTON_A = JOYSTICK_FIRST_BUTTON,		// Buttons
	KEY_XBUTTON_B,
	KEY_XBUTTON_X,
	KEY_XBUTTON_Y,
	KEY_XBUTTON_LEFT_SHOULDER,
	KEY_XBUTTON_RIGHT_SHOULDER,
	KEY_XBUTTON_BACK,
	KEY_XBUTTON_START,
	KEY_XBUTTON_STICK1,
	KEY_XBUTTON_STICK2,

	KEY_XSTICK1_RIGHT = JOYSTICK_FIRST_AXIS_BUTTON,	// XAXIS POSITIVE
	KEY_XSTICK1_LEFT,							// XAXIS NEGATIVE
	KEY_XSTICK1_DOWN,							// YAXIS POSITIVE
	KEY_XSTICK1_UP,								// YAXIS NEGATIVE
	KEY_XBUTTON_LTRIGGER,						// ZAXIS POSITIVE
	KEY_XBUTTON_RTRIGGER,						// ZAXIS NEGATIVE
	KEY_XSTICK2_RIGHT,							// UAXIS POSITIVE
	KEY_XSTICK2_LEFT,							// UAXIS NEGATIVE
	KEY_XSTICK2_DOWN,							// VAXIS POSITIVE
	KEY_XSTICK2_UP,								// VAXIS NEGATIVE
};

inline bool IsAlpha( ButtonCode_t code )
{
	return ( code >= KEY_A ) && ( code <= KEY_Z );
}

inline bool IsAlphaNumeric( ButtonCode_t code )
{
	return ( code >= KEY_0 ) && ( code <= KEY_Z );
}

inline bool IsSpace( ButtonCode_t code )
{
	return ( code == KEY_ENTER ) || ( code == KEY_TAB ) || ( code == KEY_SPACE );
}

inline bool IsKeypad( ButtonCode_t code )
{
	return ( code >= MOUSE_FIRST ) && ( code <= KEY_PAD_DECIMAL );
}

inline bool IsPunctuation( ButtonCode_t code )
{
	return ( code >= KEY_0 ) && ( code <= KEY_SPACE ) && !IsAlphaNumeric( code ) && !IsSpace( code ) && !IsKeypad( code );
}

inline bool IsKeyCode( ButtonCode_t code )
{
	return ( code >= KEY_FIRST ) && ( code <= KEY_LAST );
}

inline bool IsMouseCode( ButtonCode_t code )
{
	return ( code >= MOUSE_FIRST ) && ( code <= MOUSE_LAST );
}

inline bool IsJoystickCode( ButtonCode_t code )
{
	return ( code >= JOYSTICK_FIRST ) && ( code <= JOYSTICK_LAST );
}

inline bool IsJoystickButtonCode( ButtonCode_t code )
{
	return ( code >= JOYSTICK_FIRST_BUTTON ) && ( code <= JOYSTICK_LAST_BUTTON );
}

inline bool IsJoystickPOVCode( ButtonCode_t code )
{
	return ( code >= JOYSTICK_FIRST_POV_BUTTON ) && ( code <= JOYSTICK_LAST_POV_BUTTON );
}

inline bool IsJoystickAxisCode( ButtonCode_t code )
{
	return ( code >= JOYSTICK_FIRST_AXIS_BUTTON ) && ( code <= JOYSTICK_LAST_AXIS_BUTTON );
}

inline ButtonCode_t GetBaseButtonCode( ButtonCode_t code )
{
	if ( IsJoystickButtonCode( code ) )
	{
		int offset = ( code - JOYSTICK_FIRST_BUTTON ) % JOYSTICK_MAX_BUTTON_COUNT;
		return (ButtonCode_t)( JOYSTICK_FIRST_BUTTON + offset );
	}

	if ( IsJoystickPOVCode( code ) )
	{
		int offset = ( code - JOYSTICK_FIRST_POV_BUTTON ) % JOYSTICK_POV_BUTTON_COUNT;
		return (ButtonCode_t)( JOYSTICK_FIRST_POV_BUTTON + offset );
	}

	if ( IsJoystickAxisCode( code ) )
	{
		int offset = ( code - JOYSTICK_FIRST_AXIS_BUTTON ) % JOYSTICK_AXIS_BUTTON_COUNT;
		return (ButtonCode_t)( JOYSTICK_FIRST_AXIS_BUTTON + offset );
	}

	return code;
}

inline int GetJoystickForCode( ButtonCode_t code )
{
	if ( !IsJoystickCode( code ) )
		return 0;

	if ( IsJoystickButtonCode( code ) )
	{
		int offset = ( code - JOYSTICK_FIRST_BUTTON ) / JOYSTICK_MAX_BUTTON_COUNT;
		return offset;
	}
	if ( IsJoystickPOVCode( code ) )
	{
		int offset = ( code - JOYSTICK_FIRST_POV_BUTTON ) / JOYSTICK_POV_BUTTON_COUNT;
		return offset;
	}
	if ( IsJoystickAxisCode( code ) )
	{
		int offset = ( code - JOYSTICK_FIRST_AXIS_BUTTON ) / JOYSTICK_AXIS_BUTTON_COUNT;
		return offset;
	}

	return 0;
}

inline ButtonCode_t ButtonCodeToJoystickButtonCode( ButtonCode_t code, int nDesiredJoystick )
{
	if ( !IsJoystickCode( code ) || nDesiredJoystick == 0 )
		return code;

	nDesiredJoystick = ::clamp<int>( nDesiredJoystick, 0, MAX_JOYSTICKS - 1 );

	code = GetBaseButtonCode( code );

	// Now upsample it
	if ( IsJoystickButtonCode( code ) )
	{
		int nOffset = code - JOYSTICK_FIRST_BUTTON;
		return JOYSTICK_BUTTON( nDesiredJoystick, nOffset );
	}

	if ( IsJoystickPOVCode( code ) )
	{
		int nOffset = code - JOYSTICK_FIRST_POV_BUTTON;
		return JOYSTICK_POV_BUTTON( nDesiredJoystick, nOffset );
	}

	if ( IsJoystickAxisCode( code ) )
	{
		int nOffset = code - JOYSTICK_FIRST_AXIS_BUTTON;
		return JOYSTICK_AXIS_BUTTON( nDesiredJoystick, nOffset );
	}

	return code;
}

#endif // BUTTONCODE_H
