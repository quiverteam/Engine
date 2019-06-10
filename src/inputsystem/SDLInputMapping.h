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

int SourceKeycodeToSDL(int src);

int SDLKeycodeToSource(int sdl);