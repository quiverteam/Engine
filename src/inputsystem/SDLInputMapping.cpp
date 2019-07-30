//===========================================================================//
// Name: SDLInputMapping.cpp
// Purpose: Maps between SDL2 keycodes/button codes and source/windows stuff
// Date Created: July 16, 2019
// Authors: JJL77 jeremy.lorelli.1337@gmail.com
//===========================================================================//
#include "ButtonCode.h"
#include "AnalogCode.h"

/* SDL2 includes */
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_keyboard.h>

/* Array of button codes! */
/* This should be used as so: g_pSDLButtonCodes[SDL_Button_Code] = Source_Button_Code */
#define SDL_BUTTONCODE_COUNT 115
ButtonCode_t g_pSourceButtonCodes[SDL_BUTTONCODE_COUNT] = {
	/* 0-3 are all invalid */
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::BUTTON_CODE_INVALID,

	/* Alphabet */
	ButtonCode_t::KEY_A,
	ButtonCode_t::KEY_B,
	ButtonCode_t::KEY_C,
	ButtonCode_t::KEY_D,
	ButtonCode_t::KEY_E,
	ButtonCode_t::KEY_F,
	ButtonCode_t::KEY_G,
	ButtonCode_t::KEY_H,
	ButtonCode_t::KEY_I,
	ButtonCode_t::KEY_J,
	ButtonCode_t::KEY_K,
	ButtonCode_t::KEY_L,
	ButtonCode_t::KEY_M,
	ButtonCode_t::KEY_N,
	ButtonCode_t::KEY_O,
	ButtonCode_t::KEY_P,
	ButtonCode_t::KEY_Q,
	ButtonCode_t::KEY_R,
	ButtonCode_t::KEY_S,
	ButtonCode_t::KEY_T,
	ButtonCode_t::KEY_U,
	ButtonCode_t::KEY_V,
	ButtonCode_t::KEY_W,
	ButtonCode_t::KEY_X,
	ButtonCode_t::KEY_Y,
	ButtonCode_t::KEY_Z,

	/* 0-9 */
	ButtonCode_t::KEY_0,
	ButtonCode_t::KEY_1,
	ButtonCode_t::KEY_2,
	ButtonCode_t::KEY_3,
	ButtonCode_t::KEY_4,
	ButtonCode_t::KEY_5,
	ButtonCode_t::KEY_6,
	ButtonCode_t::KEY_7,
	ButtonCode_t::KEY_8,
	ButtonCode_t::KEY_9,

	/* Other stuff */
	ButtonCode_t::KEY_ENTER,
	ButtonCode_t::KEY_ESCAPE,
	ButtonCode_t::KEY_BACKSPACE,
	ButtonCode_t::KEY_TAB,
	ButtonCode_t::KEY_SPACE,
	ButtonCode_t::KEY_MINUS,
	ButtonCode_t::KEY_EQUAL,
	ButtonCode_t::KEY_LBRACKET,
	ButtonCode_t::KEY_RBRACKET,
	ButtonCode_t::KEY_BACKSLASH,
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::KEY_SEMICOLON,
	ButtonCode_t::KEY_APOSTROPHE,
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::KEY_COMMA,
	ButtonCode_t::KEY_PERIOD,
	ButtonCode_t::KEY_SLASH,
	ButtonCode_t::KEY_CAPSLOCK,
	ButtonCode_t::KEY_F1,
	ButtonCode_t::KEY_F2,
	ButtonCode_t::KEY_F3,
	ButtonCode_t::KEY_F4,
	ButtonCode_t::KEY_F5,
	ButtonCode_t::KEY_F6,
	ButtonCode_t::KEY_F7,
	ButtonCode_t::KEY_F8,
	ButtonCode_t::KEY_F9,
	ButtonCode_t::KEY_F10,
	ButtonCode_t::KEY_F11,
	ButtonCode_t::KEY_F12,
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::KEY_SCROLLLOCK,
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::KEY_INSERT,
	ButtonCode_t::KEY_HOME,
	ButtonCode_t::KEY_PAGEUP,
	ButtonCode_t::KEY_DELETE,
	ButtonCode_t::KEY_END,
	ButtonCode_t::KEY_PAGEDOWN,
	ButtonCode_t::KEY_RIGHT,
	ButtonCode_t::KEY_LEFT,
	ButtonCode_t::KEY_DOWN,
	ButtonCode_t::KEY_UP,
	ButtonCode_t::KEY_NUMLOCKTOGGLE,
	ButtonCode_t::KEY_PAD_DIVIDE,
	ButtonCode_t::KEY_PAD_MULTIPLY,
	ButtonCode_t::KEY_PAD_MINUS,
	ButtonCode_t::KEY_PAD_PLUS,
	ButtonCode_t::KEY_PAD_ENTER,
	ButtonCode_t::KEY_PAD_1,
	ButtonCode_t::KEY_PAD_2,
	ButtonCode_t::KEY_PAD_3,
	ButtonCode_t::KEY_PAD_4,
	ButtonCode_t::KEY_PAD_5,
	ButtonCode_t::KEY_PAD_6,
	ButtonCode_t::KEY_PAD_7,
	ButtonCode_t::KEY_PAD_8,
	ButtonCode_t::KEY_PAD_9,
	ButtonCode_t::KEY_PAD_0,
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::KEY_APP,
	ButtonCode_t::BUTTON_CODE_INVALID, /* KP_EQUALS */
	ButtonCode_t::BUTTON_CODE_INVALID, /* F13 */
	ButtonCode_t::BUTTON_CODE_INVALID, /* F14 */
	ButtonCode_t::BUTTON_CODE_INVALID, /* F15 */
	ButtonCode_t::BUTTON_CODE_INVALID, /* F16 */
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::BUTTON_CODE_INVALID,
	ButtonCode_t::BUTTON_CODE_INVALID, /* F24 */
	ButtonCode_t::BUTTON_CODE_INVALID, /* Execute */
};

/* Array of SDL2 button codes */
/* This should be used as so: g_pSourceButtonCodes[Source_Button_Code] = SDL_Button_Code */
#define SOURCE_BUTTONCODE_COUNT 107
SDL_Scancode g_pSDLButtonCodes[SOURCE_BUTTONCODE_COUNT] = {
	/* Index 0 = key_first */
	SDL_SCANCODE_UNKNOWN,
	/* KEY_0 = 1 */
	SDL_SCANCODE_0,
	SDL_SCANCODE_1,
	SDL_SCANCODE_2,
	SDL_SCANCODE_3,
	SDL_SCANCODE_4,
	SDL_SCANCODE_5,
	SDL_SCANCODE_6,
	SDL_SCANCODE_7,
	SDL_SCANCODE_8,
	SDL_SCANCODE_9,

	/* KEY_A */
	SDL_SCANCODE_A,
	SDL_SCANCODE_B,
	SDL_SCANCODE_C,
	SDL_SCANCODE_D,
	SDL_SCANCODE_E,
	SDL_SCANCODE_F,
	SDL_SCANCODE_G,
	SDL_SCANCODE_H,
	SDL_SCANCODE_I,
	SDL_SCANCODE_J,
	SDL_SCANCODE_K,
	SDL_SCANCODE_L,
	SDL_SCANCODE_M,
	SDL_SCANCODE_N,
	SDL_SCANCODE_O,
	SDL_SCANCODE_P,
	SDL_SCANCODE_Q,
	SDL_SCANCODE_R,
	SDL_SCANCODE_S,
	SDL_SCANCODE_T,
	SDL_SCANCODE_U,
	SDL_SCANCODE_V,
	SDL_SCANCODE_W,
	SDL_SCANCODE_X,
	SDL_SCANCODE_Y,
	SDL_SCANCODE_Z,

	/* Keypad stuff */
	SDL_SCANCODE_KP_0,
	SDL_SCANCODE_KP_1,
	SDL_SCANCODE_KP_2,
	SDL_SCANCODE_KP_3,
	SDL_SCANCODE_KP_4,
	SDL_SCANCODE_KP_5,
	SDL_SCANCODE_KP_6,
	SDL_SCANCODE_KP_7,
	SDL_SCANCODE_KP_8,
	SDL_SCANCODE_KP_9,
	SDL_SCANCODE_KP_DIVIDE,
	SDL_SCANCODE_KP_MULTIPLY,
	SDL_SCANCODE_KP_MINUS,
	SDL_SCANCODE_KP_PLUS,
	SDL_SCANCODE_KP_ENTER,
	SDL_SCANCODE_KP_DECIMAL,

	/* Normal stuff */
	SDL_SCANCODE_LEFTBRACKET,
	SDL_SCANCODE_RIGHTBRACKET,
	SDL_SCANCODE_SEMICOLON,
	SDL_SCANCODE_APOSTROPHE,
	SDL_SCANCODE_UNKNOWN, /* No double quote?? */
	SDL_SCANCODE_COMMA,
	SDL_SCANCODE_PERIOD,
	SDL_SCANCODE_SLASH,
	SDL_SCANCODE_BACKSLASH,
	SDL_SCANCODE_MINUS,
	SDL_SCANCODE_EQUALS,
	SDL_SCANCODE_RETURN,
	SDL_SCANCODE_SPACE,
	SDL_SCANCODE_BACKSPACE,
	SDL_SCANCODE_TAB,
	SDL_SCANCODE_CAPSLOCK,
	SDL_SCANCODE_NUMLOCKCLEAR,
	SDL_SCANCODE_ESCAPE,
	SDL_SCANCODE_SCROLLLOCK,
	SDL_SCANCODE_INSERT,
	SDL_SCANCODE_DELETE,
	SDL_SCANCODE_HOME,
	SDL_SCANCODE_END,
	SDL_SCANCODE_PAGEUP,
	SDL_SCANCODE_PAGEDOWN,
	SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_LSHIFT,
	SDL_SCANCODE_RSHIFT,
	SDL_SCANCODE_LALT,
	SDL_SCANCODE_RALT,
	SDL_SCANCODE_LCTRL,
	SDL_SCANCODE_RCTRL,
	SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_UNKNOWN,
	SDL_SCANCODE_APPLICATION,
	SDL_SCANCODE_UP,
	SDL_SCANCODE_LEFT,
	SDL_SCANCODE_DOWN,
	SDL_SCANCODE_RIGHT,
	SDL_SCANCODE_F1,
	SDL_SCANCODE_F2,
	SDL_SCANCODE_F3,
	SDL_SCANCODE_F4,
	SDL_SCANCODE_F5,
	SDL_SCANCODE_F6,
	SDL_SCANCODE_F7,
	SDL_SCANCODE_F8,
	SDL_SCANCODE_F9,
	SDL_SCANCODE_F10,
	SDL_SCANCODE_F11,
	SDL_SCANCODE_F12,
	SDL_SCANCODE_CAPSLOCK,
	SDL_SCANCODE_NUMLOCKCLEAR,
	SDL_SCANCODE_NUMLOCKCLEAR,
};

/*
Convert Source keycode to SDL2 scancode
*/
SDL_Scancode SourceKeycodeToSDL(ButtonCode_t src)
{
	int code = (int)src; /* Cast to avoid warning */
	if(code < 0 || code > SDL_BUTTONCODE_COUNT)
		return SDL_SCANCODE_UNKNOWN;
	return g_pSDLButtonCodes[code];
}

/*
Convert SDL scancode to source button code
*/
ButtonCode_t SDLKeycodeToButtonCode(SDL_Scancode sdl)
{
	int code = (int)sdl;
	if(code < 0 || code > SOURCE_BUTTONCODE_COUNT)
		return ButtonCode_t::BUTTON_CODE_INVALID;
	return g_pSourceButtonCodes[code];
}
