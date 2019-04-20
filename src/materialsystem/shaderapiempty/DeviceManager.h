//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines the shader device 
//
// $NoKeywords: $
//
//===========================================================================//
#pragma once

#include "shaderapi/ishaderapi.h"

/*

Basic shader device manager

This class appears to be an exposed interface for materialsystem to use
It queries basic system info and allows materialsystem to control
how everything is set up.

*/
class CShaderDeviceManager : public IShaderDeviceMgr
{
public:
	// Methods of IAppSystem
	virtual bool Connect( CreateInterfaceFn factory );
	virtual void Disconnect();
	virtual void *QueryInterface( const char *pInterfaceName );
	virtual InitReturnVal_t Init();
	virtual void Shutdown();

public:
	/*
    Returns num of adapters
    */
	virtual int	 GetAdapterCount() const;

    /*
    Returns info about adapters
    */
	virtual void GetAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const;

    /*
    Returns true if the specified adapter has the best GPU the system has
    */
	virtual bool GetRecommendedConfigurationInfo( int nAdapter, int nDXLevel, KeyValues *pKeyValues );

    /*
    Returns number of display modes
    */
	virtual int	 GetModeCount( int adapter ) const;

    /*
    Queries info about a specific display mode on the adapter nAdapter with the mode mode
    */
	virtual void GetModeInfo( ShaderDisplayMode_t *pInfo, int nAdapter, int mode ) const;

    /*
    Returns the currently in use mode info on the adapter nAdapter
    */
	virtual void GetCurrentModeInfo( ShaderDisplayMode_t* pInfo, int nAdapter ) const;

    /*
    Sets the used adapter to nAdapter with the flags nFlags
    */
	virtual bool SetAdapter( int nAdapter, int nFlags );

    /*
    Sets the mode?
    Returns a factory function (see DeviceManager.cpp)
    */
	virtual CreateInterfaceFn SetMode( void *hWnd, int nAdapter, const ShaderDeviceInfo_t& mode );

    /*
    Sets a callback for when mode changes
    */
	virtual void AddModeChangeCallback( ShaderModeChangeCallbackFunc_t func ) {}

    /*
    Removes the callback
    */
	virtual void RemoveModeChangeCallback( ShaderModeChangeCallbackFunc_t func ) {}
};