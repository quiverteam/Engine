/*

MessageBox.h

Platform independent message boxes

*/
#include "../public/tier4/MessageBox.h"

#ifdef _POSIX
#include <SDL2/SDL.h>
#elif defined(_WINDOWS)
#include <Windows.h>
#endif


#ifdef _POSIX

#define YES_NO_BTN_CNT 2

static const SDL_MessageBoxButtonData SDLYesNoButtons[] = {
	{
		SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
		MB_BUTTON_YES,
		"Yes",
	},
	{
		SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
		MB_BUTTON_NO,
		"No",
	}
};

#define OK_BTN_CNT 1

static const SDL_MessageBoxButtonData SDLOkButtons[] = {
	{
		SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
		MB_BUTTON_OK,
		"Ok",
	}
};

#define OK_CANCEL_BTN_CNT 2

static const SDL_MessageBoxButtonData SDLOkCancelButtons[] = {
	{
		SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
		MB_BUTTON_OK,
		"Ok",
	},
	{
		SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
		MB_BUTTON_CANCEL,
		"Cancel",
	}
};

#define ABORT_RETRY_CONT_BTN_CNT 3

static const SDL_MessageBoxButtonData SDLAbortRetryContinueButtons[] = {
	{
		SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
		MB_BUTTON_ABORT,
		"Abort",
	},
	{
		SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
		MB_BUTTON_RETRY,
		"Retry",
	},
	{
		0,
		MB_BUTTON_CONTINUE,
		"Continue",
	}
};

#define CANCEL_RETRY_CONT_BTN_CNT 3

static const SDL_MessageBoxButtonData SDLCancelRetryContinueButtons[] = {
	{
		SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
		MB_BUTTON_CANCEL,
		"Cancel",
	},
	{
		SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
		MB_BUTTON_RETRY,
		"Retry",
	},
	{
		0,
		MB_BUTTON_CONTINUE,
		"Continue",
	}
};

#define HELP_BTN_CNT 1

static const SDL_MessageBoxButtonData SDLHelpButtons[] = {
	{
		SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
		MB_BUTTON_HELP,
		"Help",
	}
};

#define RETRY_CANCEL_BTN_CNT 2

static const SDL_MessageBoxButtonData SDLRetryCancelButtons[] = {
	{
		SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
		MB_BUTTON_RETRY,
		"Retry",
	},
	{
		SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
		MB_BUTTON_CANCEL,
		"Cancel",
	}
};

#define YES_NO_CANCEL_BTN_CNT 3

static const SDL_MessageBoxButtonData SDLYesNoCancelButtons[] = {
	{
		SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
		MB_BUTTON_YES,
		"Yes",
	},
	{
		0,
		MB_BUTTON_NO,
		"No",
	},
	{
		SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
		MB_BUTTON_CANCEL,
		"Cancel",
	}
};


// default color scheme
static const SDL_MessageBoxColorScheme SDLColorScheme = {
	{
		{55, 55, 55},
		{245, 245, 245},
		{100, 100, 100},
		{60, 60, 60},
		{75, 75, 75},
	}
};

#endif

#ifdef _WINDOWS

// Basically just used for some simple quick translation of return values
int returns[16];

#endif

int Plat_ShowMessageBox(const char* pTitle, const char* pText, int type, int buttons)
{
#ifdef _POSIX

	SDL_MessageBoxData data;
	data.title = pTitle;
	data.message = pText;
	data.window = NULL;

	switch(buttons)
	{
		case MB_BUTTONS_ABORTRETRYIGNORE:
			data.buttons = SDLAbortRetryContinueButtons;
			data.numbuttons = ABORT_RETRY_CONT_BTN_CNT;
			break;
		case MB_BUTTONS_CANCELRETRYCONTINUE:
			data.buttons = SDLCancelRetryContinueButtons;
			data.numbuttons = CANCEL_RETRY_CONT_BTN_CNT;
			break;
		case MB_BUTTONS_HELP:
			data.buttons = SDLHelpButtons;
			data.numbuttons = HELP_BTN_CNT;
			break;
		case MB_BUTTONS_OKCANCEL:
			data.buttons = SDLOkCancelButtons;
			data.numbuttons = OK_CANCEL_BTN_CNT;
			break;
		case MB_BUTTONS_RETRYCANCEL:
			data.buttons = SDLRetryCancelButtons;
			data.numbuttons = RETRY_CANCEL_BTN_CNT;
			break;
		case MB_BUTTONS_YESNO:
			data.buttons = SDLYesNoButtons;
			data.numbuttons = YES_NO_BTN_CNT;
			break;
		case MB_BUTTONS_YESNOCANCEL:
			data.buttons = SDLYesNoCancelButtons;
			data.numbuttons = YES_NO_CANCEL_BTN_CNT;
			break;
		default:
			data.buttons = SDLOkButtons;
			data.numbuttons = OK_BTN_CNT;
			break;
	}

	// Do this anyway, just in case these defineschange in a later version of SDL
	switch(type)
	{
		case MB_TYPE_ERROR:
			data.flags = SDL_MESSAGEBOX_ERROR;
			break;
		case MB_TYPE_FATAL:
			data.flags = SDL_MESSAGEBOX_ERROR;
			break;
		case MB_TYPE_WARN:
			data.flags = SDL_MESSAGEBOX_WARNING;
			break;
		default:
			data.flags = SDL_MESSAGEBOX_INFORMATION;
			break;
	}

	data.colorScheme = &SDLColorScheme;
	
	int usedBtn = 0;
	
	// Tbh we can just ignore errors in this
	SDL_ShowMessageBox(&data, &usedBtn);

	return usedBtn;



#elif defined(_WINDOWS)
	
	// hehe butts
	DWORD butts = 0;

	switch(buttons)
	{
		case MB_BUTTONS_OKCANCEL:
			butts = MB_OKCANCEL;
			break;
		case MB_BUTTONS_ABORTRETRYIGNORE:
			butts = MB_ABORTRETRYIGNORE;
			break;
		case MB_BUTTONS_CANCELRETRYCONTINUE:
			butts = MB_ABORTRETRYCONTINUE;
			break;
		case MB_BUTTONS_HELP:
			butts = MB_HELP;
			break;
		case MB_BUTTONS_RETRYCANCEL:
			butts = MB_RETRYCANCEL;
			break;
		case MB_BUTTONS_YESNO:
			butts = MB_YESNO;
			break;
		case MB_BUTTONS_YESNOCANCEL:
			butts = MB_YESNOCANCEL;
			break;
		default:
			butts = MB_OK;
			break;
	}

	DWORD icon = 0;

	switch(type)
	{
		case MB_TYPE_WARN:
			icon = MB_ICONWARNING;
			break;
		case MB_TYPE_ERROR:
			icon = MB_ICONERROR;
			break;
		case MB_TYPE_FATAL:
			icon = MB_ICONERROR;
			break;
		default:
			icon = MB_ICONINFORMATION;
			break;
	}

	DWORD ret = MessageBoxA(NULL, (LPCTSTR)pText, (LPCTSTR)pTitle, butts | type);

	// Only used here because ID* defines are LESS than some absurd number
	static const int Returns[16];
	Returns[IDABORT] = MB_BUTTON_ABORT;
	Returns[IDCANCEL] = MB_BUTTON_CANCEL;
	Returns[IDCONTINUE] = MB_BUTTON_CONTINUE;
	Returns[IDIGNORE] = MB_BUTTON_IGNORE;
	Returns[IDNO] = MB_BUTTON_NO;
	Returns[IDYES] = MB_BUTTON_YES;
	Returns[IDOK] = MB_BUTTON_OK;
	Returns[IDRETRY] = MB_BUTTON_RETRY;
	Returns[IDTRYAGAIN] = MB_BUTTON_RETRY;

	if(ret < 16 && ret >= 0)
		return Returns[ret];
	else
		return ret;
#endif
}
