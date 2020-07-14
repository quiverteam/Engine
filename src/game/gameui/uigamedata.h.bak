//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef __UIGAMEDATA_H__
#define __UIGAMEDATA_H__

#include "vgui_controls/Panel.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/Button.h"
#include "tier1/utllinkedlist.h"
#include "tier1/UtlMap.h"
#include "tier1/keyvalues.h"
#include "tier1/fmtstr.h"

#ifndef _X360
#include "steam/steam_api.h"
#endif // _X360

#include "ixboxsystem.h"

#include "basemodpanel.h"
#include "basemodframe.h"
//#include "UIAvatarImage.h"
//#include "tokenset.h"

#include "EngineInterface.h"

extern ConVar x360_audio_english;

extern ConVar demo_ui_enable;
extern ConVar demo_connect_string;

namespace BaseModUI
{
	class CUIGameData
	{
		static CUIGameData* m_Singleton;
		static bool m_bModuleShutDown;

	public:
		static CUIGameData* Get();
		static void Shutdown();

		void RunFrame();
		void RunFrame_Storage();
		void RunFrame_Invite();

		void OnEvent( KeyValues *pEvent );

		void OnGameUIPostInit();
		void NeedConnectionProblemWaitScreen();
		void ShowPasswordUI(char const*pchCurrentPW);

		vgui::IImage * GetAvatarImage( XUID playerID );
		char const * GetPlayerName( XUID playerID, char const *szPlayerNameSpeculative );

		bool IsXUIOpen(){return m_bXUIOpen;};

	protected:
		
		CUtlMap< XUID, CUtlString > m_mapUserXuidToName;

		//XUI info
		bool m_bXUIOpen;

		//char const * GetPlayerName( XUID playerID, char const *szPlayerNameSpeculative );
	};

//=============================================================================
//
//=============================================================================

//
// ISelectStorageDeviceClient
//
//		Client interface for device selector:
//			async flow is as follows:
//			client calls into SelectStorageDevice with its parameters established:
//				GetCtrlrIndex, ForceSelector, AllowDeclined
//			XUI blade shows up (or implicitly determines which device should be picked based on settings).
//			if OnSelectError callback fires, then the process failed.
//			if OnDeviceNotSelected fires, then the process is over, device not picked
//			if OnDeviceFull fires, then device has insufficient capacity and cannot be used
//			if OnDeviceSelected fires, then device has been picked and async operations on containers started
//				should wait for AfterDeviceMounted callback
//			when AfterDeviceMounted callback fires the device is fully mounted and ready
//
class ISelectStorageDeviceClient
{
public:
	virtual int  GetCtrlrIndex() = 0;			// Controller index (0, 1, 2 or 3)
	virtual bool ForceSelector() = 0;			// Whether device selector should be forcefully shown
	virtual bool AllowDeclined() = 0;			// Whether declining storage device is allowed
	virtual bool AllowAnyController() = 0;		// Whether any connected controller can be selecting storage or only game-committed

	enum FailReason_t
	{
		FAIL_ERROR,
		FAIL_NOT_SELECTED,
		FAIL_FULL,
		FAIL_CORRUPT
	};
	virtual void OnDeviceFail( FailReason_t eReason ) = 0; // Storage device has not been set
	
	virtual void OnDeviceSelected() = 0;		// After device has been picked in XUI blade, but before mounting symbolic roots and opening containers
	virtual void AfterDeviceMounted() = 0;		// After device has been successfully mounted, configs processed, etc.
};

}

#endif // __UIGAMEDATA_H__