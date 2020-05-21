#
#
# Project script for gameui
#
#
project(gameui C CXX)

# Platform specific sources
set(WINDOWS_SRCS	)
set(POSIX_SRCS	)

set(SRCS
        "BackgroundMenuButton.cpp"
        "BasePanel.cpp"
        "GameConsole.cpp"
        "GameUI_Interface.cpp"
        "LogoFile.cpp"
        "ModInfo.cpp"
        "MouseMessageForwardingPanel.cpp"
        "${ROOT}/Tracker/common/msgbuffer.cpp"
        "${ROOT}/Tracker/common/netapi.cpp"
        "${ROOT}/common/GameUI/ObjectList.cpp"
        "PanelListPanel.cpp"
        "RunGameEngine.cpp"
        "ScriptObject.cpp"
        "${ROOT}/Tracker/common/Socket.cpp"
        "Sys_Utils.cpp"
        "TextEntryBox.cpp"
        "TGAImagePanel.cpp"
        "${PUBLIC}/vgui_controls/vgui_controls.cpp"
        "VGuiSystemModuleLoader.cpp"
        "BonusMapsDatabase.cpp"
        "BonusMapsDatabase.h"
        "${ROOT}/common/language.cpp"
        "SaveGameBrowserDialog.cpp"
        "BackgroundMenuButton.h"
        "BasePanel.h"
        "BaseSaveGameDialog.h"
        "CDKeyEntryDialog.h"
        "ChangeGameDialog.h"
        "CreateMultiplayerGameBotPage.h"
        "CreateMultiplayerGameDialog.h"
        "CreateMultiplayerGameGameplayPage.h"
        "CreateMultiplayerGameServerPage.h"
        "EngineInterface.h"
        "GameConsole.h"
        "GameUI_Interface.h"
        "LoadingDialog.h"
        "${ROOT}/vgui2/src/Memorybitmap.h"
        "ModInfo.h"
        "MouseMessageForwardingPanel.h"
        "PanelListPanel.h"
        "ScriptObject.h"
        "Sys_Utils.h"
        "TextEntryBox.h"
        "TGAImagePanel.h"
        "VGuiSystemModuleLoader.h"
        "SaveGameBrowserDialog.h"
        "${PUBLIC}/iachievementmgr.h"
        "${PUBLIC}/game/client/IGameClientExports.h"
        "${ROOT}/common/GameUI/IGameUI.h"
        "${PUBLIC}/IGameUIFuncs.h"
        "${PUBLIC}/tier1/interface.h"
        "${ROOT}/common/IObjectContainer.h"
        "${ROOT}/common/IRunGameEngine.h"
        "${ROOT}/common/IVguiModule.h"
        "${ROOT}/common/IVGuiModuleLoader.h"
        "${ROOT}/common/GameUI/ObjectList.h"
        "${PUBLIC}/savegame_version.h"
        "${ROOT}/Tracker/common/TrackerMessageFlags.h"
        "${ROOT}/common/ValveCDKeyGameAndTerritoryCodes.h"
        "${ROOT}/common/language.h"
        "BitmapImagePanel.cpp"
        "BitmapImagePanel.h"
        "CommandCheckButton.cpp"
        "CommandCheckButton.h"
        "CvarNegateCheckButton.cpp"
        "CvarNegateCheckButton.h"
        "CvarSlider.cpp"
        "CvarSlider.h"
        "CvarTextEntry.cpp"
        "CvarTextEntry.h"
        "CvarToggleCheckButton.cpp"
        "CvarToggleCheckButton.h"
        "KeyToggleCheckButton.cpp"
        "KeyToggleCheckButton.h"
        "LabeledCommandComboBox.cpp"
        "LabeledCommandComboBox.h"
        "URLButton.cpp"
        "URLButton.h"
        "vcontrolslistpanel.cpp"
        "vcontrolslistpanel.h"
        "BenchmarkDialog.cpp"
        "BenchmarkDialog.h"
        "BonusMapsDialog.cpp"
        "BonusMapsDialog.h"
        "CommentaryDialog.cpp"
        "CommentaryDialog.h"
        "CommentaryExplanationDialog.cpp"
        "CommentaryExplanationDialog.h"
        "ContentControlDialog.cpp"
        "ContentControlDialog.h"
        "GameConsoleDialog.cpp"
        "GameConsoleDialog.h"
        "LoadGameDialog_Xbox.cpp"
        "LoadGameDialog.cpp"
        "LoadGameDialog.h"
        "MultiplayerAdvancedDialog.cpp"
        "MultiplayerAdvancedDialog.h"
        "NewGameDialog.cpp"
        "NewGameDialog.h"
        "PlayerListDialog.cpp"
        "PlayerListDialog.h"
        "SaveGameDialog_Xbox.cpp"
        "SaveGameDialog.cpp"
        "SaveGameDialog.h"
        "LoadCommentaryDialog.cpp"
        "LoadingDialog.cpp"
        "BaseSaveGameDialog.cpp"
        "ChangeGameDialog.cpp"
        "CreateMultiplayerGameBotPage.cpp"
        "CreateMultiplayerGameDialog.cpp"
        "CreateMultiplayerGameGameplayPage.cpp"
        "CreateMultiplayerGameServerPage.cpp"
        "OptionsDialog_Xbox.cpp"
        "ControllerDialog.cpp"
        "ControllerDialog.h"
        "matchmaking/achievementsdialog.cpp"
        "matchmaking/achievementsdialog.h"
        "matchmaking/basedialog.cpp"
        "matchmaking/basedialog.h"
        "matchmaking/dialogmenu.cpp"
        "matchmaking/dialogmenu.h"
        "matchmaking/leaderboarddialog.cpp"
        "matchmaking/leaderboarddialog.h"
        "matchmaking/matchmakingbasepanel.cpp"
        "matchmaking/matchmakingbasepanel.h"
        "matchmaking/pausedialog.cpp"
        "matchmaking/pausedialog.h"
        "matchmaking/sessionlobbydialog.cpp"
        "matchmaking/sessionlobbydialog.h"
        "matchmaking/sessionoptionsdialog.cpp"
        "matchmaking/sessionoptionsdialog.h"
        "matchmaking/sessionbrowserdialog.cpp"
        "matchmaking/sessionbrowserdialog.h"
        "matchmaking/welcomedialog.cpp"
        "matchmaking/welcomedialog.h"
        "OptionsDialog.cpp"
        "OptionsDialog.h"
        "OptionsSubAudio.cpp"
        "OptionsSubAudio.h"
        "OptionsSubDifficulty.cpp"
        "OptionsSubDifficulty.h"
        "OptionsSubGame.cpp"
        "OptionsSubGame.h"
        "OptionsSubKeyboard.cpp"
        "OptionsSubKeyboard.h"
        "OptionsSubMouse.cpp"
        "OptionsSubMouse.h"
        "OptionsSubMultiplayer.cpp"
        "OptionsSubMultiplayer.h"
        "OptionsSubPortal.cpp"
        "OptionsSubPortal.h"
        "OptionsSubVideo.cpp"
        "OptionsSubVideo.h"
        "OptionsSubVoice.cpp"
        "OptionsSubVoice.h")

# Preprocessor defs
set(DEFINES			-DGAMEUI_EXPORTS)

# Platform-specific preprocessor defines
set(WIN32_DEFINES	)
set(POSIX32_DEFINES	)
set(WIN64_DEFINES	)
set(POSIX64_DEFINES	)
set(WINDOWS_DEFINES	)
set(POSIX_DEFINES	)

set(PLATFORM_64BIT_DEFINES	)
set(PLATFORM_32BIT_DEFINES 	)

# Links dirs to search for link libs in
set(LINK_DIRS			)
set(WINDOWS_LINK_DIRS	)
set(POSIX_LINK_DIRS		)
set(WIN32_LINK_DIRS		)
set(WIN64_LINK_DIRS		)
set(POSIX32_LINK_DIRS	)
set(POSIX64_LINK_DIRS	)

# Generic libs to link against
set(LINK_LIBS			)

# Other projects this one depends on
set(DEPENDENCIES 		bitmap tier2 tier3 mathlib vgui_controls vtf jpeglib steam_api tier1 tier0 vstdlib)

# Platform specific link libs
set(WINDOWS_LINK_LIBS	ws2_32.lib odbc32.lib odbccp32.lib shlwapi.lib)
set(WIN32_LINK_LIBS		)
set(WIN64_LINK_LIBS		)
set(POSIX32_LINK_LIBS	)
set(POSIX64_LINK_LIBS	)

# Generic include dirs
set(INCLUDE_DIRS					./
        ${ROOT}/vgui2/include
        ${ROOT}/vgui2/controls)

# Platform specific includes
set(WIN32_INCLUDE_DIRS		)
set(WIN64_INCLUDE_DIRS		)
set(POSIX32_INCLUDE_DIRS	)
set(POSIX64_INCLUDE_DIRS	)

# Set the variable of target for stuff
set(TARGET gameui)

# Set the output file name
set(OUTPUT_FILE_NAME gameui)

# If this is a shared lib
set(SHARED_LIB 1)

# If this is a static lib
set(STATIC_LIB 0)

# Include this to handle all the defines
include(${CMAKESCRIPTS_DIR}/library-base.cmake)

#target_link_libraries(gameui steam_api)
