//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines the shader device 
//
// $NoKeywords: $
//
//===========================================================================//
#include "DeviceManager.h"
#include "ShaderAPI.h"
#include "ShaderAPIEmpty.h"

#include "shaderapi/ishaderutil.h"

// Returns from setmode
// Returns a global shader interface factory
static void* ShaderInterfaceFactory( const char *pInterfaceName, int *pReturnCode )
{
	if ( pReturnCode )
	{
		*pReturnCode = IFACE_OK;
	}
	if ( !Q_stricmp( pInterfaceName, SHADER_DEVICE_INTERFACE_VERSION ) )
		return static_cast< IShaderDevice* >( &s_ShaderDeviceEmpty );
	if ( !Q_stricmp( pInterfaceName, SHADERAPI_INTERFACE_VERSION ) )
		return static_cast< IShaderAPI* >( &g_ShaderAPIEmpty );
	if ( !Q_stricmp( pInterfaceName, SHADERSHADOW_INTERFACE_VERSION ) )
		return static_cast< IShaderShadow* >( &g_ShaderShadow );

	if ( pReturnCode )
	{
		*pReturnCode = IFACE_FAILED;
	}
	return NULL;
}


bool CShaderDeviceManager::Connect( CreateInterfaceFn factory )
{
	// So others can access it
	g_pShaderUtil = (IShaderUtil*)factory( SHADER_UTIL_INTERFACE_VERSION, NULL );

	return true;
}

void CShaderDeviceManager::Disconnect()
{
	g_pShaderUtil = NULL;
}

void *CShaderDeviceManager::QueryInterface( const char *pInterfaceName )
{
	if ( !Q_stricmp( pInterfaceName, SHADER_DEVICE_MGR_INTERFACE_VERSION ) )
		return static_cast< IShaderDeviceMgr* >( this );
	if ( !Q_stricmp( pInterfaceName, MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION ) )
		return static_cast< IMaterialSystemHardwareConfig* >( &g_ShaderAPIEmpty );
	return NULL;
}

InitReturnVal_t CShaderDeviceManager::Init()
{
	return INIT_OK;
}

void CShaderDeviceManager::Shutdown()
{

}

// Sets the adapter
bool CShaderDeviceManager::SetAdapter( int nAdapter, int nFlags )
{
	return true;
}

// FIXME: Is this a public interface? Might only need to be private to shaderapi
CreateInterfaceFn CShaderDeviceManager::SetMode( void *hWnd, int nAdapter, const ShaderDeviceInfo_t& mode ) 
{
	return ShaderInterfaceFactory;
}

// Gets the number of adapters...
int	 CShaderDeviceManager::GetAdapterCount() const
{
	return 0;
}

bool CShaderDeviceManager::GetRecommendedConfigurationInfo( int nAdapter, int nDXLevel, KeyValues *pKeyValues ) 
{
	return true;
}

// Returns info about each adapter
void CShaderDeviceManager::GetAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const
{
	memset( &info, 0, sizeof( info ) );
	info.m_nDXSupportLevel = 90;
}

// Returns the number of modes
int	 CShaderDeviceManager::GetModeCount( int nAdapter ) const
{
	return 0;
}

// Returns mode information..
void CShaderDeviceManager::GetModeInfo( ShaderDisplayMode_t *pInfo, int nAdapter, int nMode ) const
{
}

void CShaderDeviceManager::GetCurrentModeInfo( ShaderDisplayMode_t* pInfo, int nAdapter ) const
{
}

