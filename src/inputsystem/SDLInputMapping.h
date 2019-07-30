/*

Maps SDL2 keycodes to Source keycodes

Note: this is only temporary, need to define this to ensure that Source doesnt break if we redefine enums

*/
#pragma once

#include "inputsystem/ButtonCode.h"
#include "inputsystem/AnalogCode.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>

/*
Convert Source keycode to SDL2 scancode
*/
SDL_Scancode SourceKeycodeToSDL(ButtonCode_t src);

/*
Convert SDL scancode to source button code
*/
ButtonCode_t SDLKeycodeToButtonCode(SDL_Scancode sdl);
