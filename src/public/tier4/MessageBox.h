/*

MessageBox.h

Platform independent message boxes

*/
#pragma once

//
// The type of icon to show in the message box	
//
#define MB_TYPE_INFO		(1<<0)
#define MB_TYPE_ERROR		(1<<1)
#define MB_TYPE_WARN		(1<<2)
#define MB_TYPE_FATAL		(1<<3)

//
// Selects the buttons to display on the dialog
//
#define MB_BUTTONS_OK					(1<<0)
#define MB_BUTTONS_OKCANCEL				(1<<1)
#define MB_BUTTONS_ABORTRETRYIGNORE		(1<<2)
#define MB_BUTTONS_CANCELRETRYCONTINUE	(1<<3)
#define MB_BUTTONS_HELP					(1<<4)
#define MB_BUTTONS_RETRYCANCEL			(1<<5)
#define MB_BUTTONS_YESNO				(1<<6)
#define MB_BUTTONS_YESNOCANCEL			(1<<7)

//
// Identifies the button that was selected
//
#define MB_BUTTON_OK		(1<<0)
#define MB_BUTTON_CANCEL	(1<<1)
#define MB_BUTTON_ABORT 	(1<<2)
#define MB_BUTTON_HELP		(1<<3)
#define MB_BUTTON_NO		(1<<4)
#define MB_BUTTON_YES		(1<<5)
#define MB_BUTTON_RETRY		(1<<6)
#define MB_BUTTON_IGNORE	(1<<7)
#define MB_BUTTON_CONTINUE	(1<<8)
/*

Displays a message box

Returns the ID of the selected option (see the MB_BUTTON_* defines)

pTitle points to a null terminated string representing the title of the box
pText points to a null terminated string representing the text inside the box
type is the type of the box (see the MB_TYPE_* defines)
buttons specifies the buttons to include in the box (see the MB_BUTTONS_* defines)

*/
int Plat_ShowMessageBox(const char* pTitle, const char* pText, int type, int buttons);