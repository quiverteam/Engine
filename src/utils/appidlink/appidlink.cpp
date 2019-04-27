#include "steam/steam_api.h"
#include "steam/isteamclient.h"
#include <iostream>
#include <windows.h>

int main( int argc, char* argv[] )
{
	if ( argc < 2 || argc > 4 )
		return 1;

	if ( !SteamAPI_Init() )
	{
		std::cout << "SteamAPI_Init() Failed!\n";
		return 1;
	}

	AppId_t appid = atoi( argv[2] );
	if ( !SteamApps()->BIsAppInstalled( appid ) )
		return 1;

	char appiddir[_MAX_PATH];
	SteamApps()->GetAppInstallDir( appid, appiddir, sizeof( appiddir ) );

	if ( argc == 4 )
	{
		//Add the sub folder string
		strcat( appiddir, "\\" );
		strcat( appiddir, argv[3] );
	}

	std::cout << "Creating Sym Link for directory:\n" << appiddir << "\n On Directory:\n" << argv[1] << "\n";
	CreateSymbolicLink( argv[1], appiddir, SYMBOLIC_LINK_FLAG_DIRECTORY );
	SteamAPI_Shutdown();
	return 0;
}