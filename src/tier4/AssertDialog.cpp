/*

AssertDialog.h

Platform independent assertion dialogs

*/
#include "tier4/AssertDialog.h"

#ifdef _POSIX
#include <SDL2/SDL.h>
#elif defined(_WINDOWS)
#include <Windows.h>
#endif
