//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include <d3d11.h>
#include <d3dcompiler.h>

#include "shaderdevicedx11.h"
#include "shaderapi/ishaderutil.h"
#include "shaderapidx11.h"
#include "shadershadowdx11.h"
#include "IndexBufferDx11.h"
#include "VertexBufferDx11.h"
#include "shaderapidx11_global.h"
#include "tier1/keyvalues.h"
#include "tier2/tier2.h"
#include "tier0/icommandline.h"
#include "inputlayoutdx11.h"
#include "shaderapidx9/shaderapibase.h"
#include "meshdx11.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

// this is hooked into the engines convar
ConVar mat_debugalttab( "mat_debugalttab", "0", FCVAR_CHEAT );


//-----------------------------------------------------------------------------
// Explicit instantiation of shader buffer implementation
//-----------------------------------------------------------------------------
template class CShaderBuffer< ID3DBlob >;


//-----------------------------------------------------------------------------
//
// Device manager
//
//-----------------------------------------------------------------------------
static CShaderDeviceMgrDx11 g_ShaderDeviceMgrDx11;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderDeviceMgrDx11, IShaderDeviceMgr, 
	SHADER_DEVICE_MGR_INTERFACE_VERSION, g_ShaderDeviceMgrDx11 )

static CShaderDeviceDx11 g_ShaderDeviceDx11;
CShaderDeviceDx11* g_pShaderDeviceDx11 = &g_ShaderDeviceDx11;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderDeviceDx11, IShaderDevice,
				   SHADER_DEVICE_INTERFACE_VERSION, g_ShaderDeviceDx11 )

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CShaderDeviceMgrDx11::CShaderDeviceMgrDx11()
{
	m_pDXGIFactory = NULL;
	m_bSetupAdapters = false;
	m_bObeyDxCommandlineOverride = true;
}

CShaderDeviceMgrDx11::~CShaderDeviceMgrDx11()
{
}


//-----------------------------------------------------------------------------
// Connect, disconnect
//-----------------------------------------------------------------------------
bool CShaderDeviceMgrDx11::Connect( CreateInterfaceFn factory )
{
	LOCK_SHADERAPI();

	Log("Connecting CShaderDeviceMgrDx11...\n");

	if ( !BaseClass::Connect( factory ) )
		return false;

	HRESULT hr = CreateDXGIFactory( __uuidof(IDXGIFactory), (void**)(&m_pDXGIFactory) );
	if ( FAILED( hr ) )
	{
		Warning( "Failed to create the DXGI Factory!\n" );
		return false;
	}

	InitAdapterInfo();
	return true;
}

void CShaderDeviceMgrDx11::Disconnect()
{
	LOCK_SHADERAPI();

	if ( m_pDXGIFactory )
	{
		m_pDXGIFactory->Release();
		m_pDXGIFactory = NULL;
	}

	BaseClass::Disconnect();
}


//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
InitReturnVal_t CShaderDeviceMgrDx11::Init( )
{
	LOCK_SHADERAPI();

	InitAdapterInfo();

	return INIT_OK;
}


//-----------------------------------------------------------------------------
// Shutdown
//-----------------------------------------------------------------------------
void CShaderDeviceMgrDx11::Shutdown( )
{
	LOCK_SHADERAPI();

	if ( g_pShaderDevice )
	{
		g_pShaderDevice->ShutdownDevice();
		g_pShaderDevice = NULL;
	}
}


//-----------------------------------------------------------------------------
// Initialize adapter information
//-----------------------------------------------------------------------------
void CShaderDeviceMgrDx11::InitAdapterInfo()
{
	if ( m_bSetupAdapters )
		return;

	m_Adapters.RemoveAll();

	IDXGIAdapter *pAdapter;
	for( UINT nCount = 0; m_pDXGIFactory->EnumAdapters( nCount, &pAdapter ) != DXGI_ERROR_NOT_FOUND; ++nCount )
	{
		IDXGIOutput *pOutput = GetAdapterOutput( nCount );
		//Log( "pOutput: %p\n", pAdapter );
		if ( !pOutput )
			break;

		int j = m_Adapters.AddToTail();
		AdapterInfo_t &info = m_Adapters[j];

#ifdef _DEBUG
		memset( &info.m_ActualCaps, 0xDD, sizeof(info.m_ActualCaps) );
#endif

		
		info.m_ActualCaps.m_bDeviceOk = ComputeCapsFromD3D( &info.m_ActualCaps, pAdapter, pOutput );
		if ( !info.m_ActualCaps.m_bDeviceOk )
			continue;

		ReadDXSupportLevels( info.m_ActualCaps );

		// Read dxsupport.cfg which has config overrides for particular cards.
		ReadHardwareCaps( info.m_ActualCaps, info.m_ActualCaps.m_nMaxDXSupportLevel );

		// What's in "-shader" overrides dxsupport.cfg
		const char *pShaderParam = CommandLine()->ParmValue( "-shader" );
		if ( pShaderParam )
		{
			Q_strncpy( info.m_ActualCaps.m_pShaderDLL, pShaderParam, sizeof( info.m_ActualCaps.m_pShaderDLL ) );
		}
	}

	m_bSetupAdapters = true;
}


//-----------------------------------------------------------------------------
// Determines hardware caps from D3D
//-----------------------------------------------------------------------------
bool CShaderDeviceMgrDx11::ComputeCapsFromD3D( HardwareCaps_t *pCaps, IDXGIAdapter *pAdapter, IDXGIOutput *pOutput )
{
	DXGI_ADAPTER_DESC desc;
	HRESULT hr = pAdapter->GetDesc( &desc );
	Assert( !FAILED( hr ) );
	if ( FAILED( hr ) )
	{
		Warning( "Dx11: Couldn't get adapter desc\n" );
		return false;
	}
		

	bool bForceFloatHDR = ( CommandLine()->CheckParm( "-floathdr" ) != NULL );

	// DX10 settings
	// NOTE: We'll need to have different settings for dx10.1 and dx11
	Q_UnicodeToUTF8( desc.Description, pCaps->m_pDriverName, MATERIAL_ADAPTER_NAME_LENGTH );
	pCaps->m_VendorID = desc.VendorId;
	pCaps->m_DeviceID = desc.DeviceId;
	pCaps->m_SubSysID = desc.SubSysId;
	pCaps->m_Revision = desc.Revision;
	pCaps->m_NumSamplers = 16;
	pCaps->m_NumTextureStages = 0;
	pCaps->m_HasSetDeviceGammaRamp = true;
	pCaps->m_bSoftwareVertexProcessing = false;
	pCaps->m_SupportsVertexShaders = true;
	pCaps->m_SupportsVertexShaders_2_0 = false;
	pCaps->m_SupportsPixelShaders = true;
	pCaps->m_SupportsPixelShaders_1_4 = false;
	pCaps->m_SupportsPixelShaders_2_0 = false;
	pCaps->m_SupportsPixelShaders_2_b = false;
	pCaps->m_SupportsShaderModel_3_0 = false;
	pCaps->m_SupportsCompressedTextures = COMPRESSED_TEXTURES_ON;
	pCaps->m_SupportsCompressedVertices = VERTEX_COMPRESSION_ON;
	pCaps->m_bSupportsAnisotropicFiltering = true;
	pCaps->m_bSupportsMagAnisotropicFiltering = true;
	pCaps->m_bSupportsVertexTextures = true;
	pCaps->m_nMaxAnisotropy = 16;
	pCaps->m_MaxTextureWidth = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
	pCaps->m_MaxTextureHeight = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
	pCaps->m_MaxTextureDepth = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
	pCaps->m_MaxTextureAspectRatio = 1024;	// FIXME
	pCaps->m_MaxPrimitiveCount = 65536;		// FIXME
	pCaps->m_ZBiasAndSlopeScaledDepthBiasSupported = true;
	pCaps->m_SupportsMipmapping = true;
	pCaps->m_SupportsOverbright = true;
	pCaps->m_SupportsCubeMaps = true;
	pCaps->m_NumPixelShaderConstants = 1024;	// FIXME
	pCaps->m_NumVertexShaderConstants = 1024;	// FIXME
	pCaps->m_TextureMemorySize = desc.DedicatedVideoMemory;
	pCaps->m_MaxNumLights = 4;
	pCaps->m_SupportsHardwareLighting = false;
	pCaps->m_MaxBlendMatrices = 0;
	pCaps->m_MaxBlendMatrixIndices = 0;
	pCaps->m_MaxVertexShaderBlendMatrices = 53;	// FIXME
	pCaps->m_SupportsMipmappedCubemaps = true;
	pCaps->m_SupportsNonPow2Textures = true;
	pCaps->m_nDXSupportLevel = 110;
	pCaps->m_PreferDynamicTextures = false;
	pCaps->m_HasProjectedBumpEnv = true;
	pCaps->m_MaxUserClipPlanes = 6;		// FIXME
	pCaps->m_HDRType = bForceFloatHDR ? HDR_TYPE_FLOAT : HDR_TYPE_INTEGER;
	pCaps->m_SupportsSRGB = true;
	pCaps->m_bSupportsSpheremapping = true;
	pCaps->m_UseFastClipping = false;
	pCaps->m_pShaderDLL[0] = 0;
	pCaps->m_bNeedsATICentroidHack = false;
	pCaps->m_bColorOnSecondStream = true;
	pCaps->m_bSupportsStreamOffset = true;
	pCaps->m_nMaxDXSupportLevel = 110;
	pCaps->m_bFogColorSpecifiedInLinearSpace = ( desc.VendorId == VENDORID_NVIDIA );
	pCaps->m_nVertexTextureCount = 16;
	pCaps->m_nMaxVertexTextureDimension = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
	pCaps->m_bSupportsAlphaToCoverage = false;	// FIXME
	pCaps->m_bSupportsShadowDepthTextures = true;
	pCaps->m_bSupportsFetch4 = ( desc.VendorId == VENDORID_ATI );
#if defined( COMPRESSED_NORMAL_FORMATS )
	pCaps->m_bSupportsNormalMapCompression = true;
#else
	pCaps->m_bSupportsNormalMapCompression = false;
#endif
	pCaps->m_bSupportsBorderColor = true;
	pCaps->m_ShadowDepthTextureFormat = IMAGE_FORMAT_UNKNOWN;
	pCaps->m_nMaxViewports = 4;


	//DXGI_GAMMA_CONTROL_CAPABILITIES gammaCaps;
	//pOutput->GetGammaControlCapabilities( &gammaCaps );
	//pCaps->m_flMinGammaControlPoint = gammaCaps.MinConvertedValue;
	//pCaps->m_flMaxGammaControlPoint = gammaCaps.MaxConvertedValue;
	//pCaps->m_nGammaControlPointCount = gammaCaps.NumGammaControlPoints;
	pCaps->m_flMinGammaControlPoint = 0.0f;
	pCaps->m_flMaxGammaControlPoint = 65535.0f;
	pCaps->m_nGammaControlPointCount = 256;
	Log( "Set gamma control capabilities %f %f %i\n", pCaps->m_flMaxGammaControlPoint, pCaps->m_flMaxGammaControlPoint, pCaps->m_nGammaControlPointCount );

	return true;
}


//-----------------------------------------------------------------------------
// Gets the number of adapters...
//-----------------------------------------------------------------------------
int CShaderDeviceMgrDx11::GetAdapterCount() const
{
	return m_Adapters.Count();
}


//-----------------------------------------------------------------------------
// Returns info about each adapter
//-----------------------------------------------------------------------------
void CShaderDeviceMgrDx11::GetAdapterInfo( int nAdapter, MaterialAdapterInfo_t& info ) const
{
	Assert( ( nAdapter >= 0 ) && ( nAdapter < m_Adapters.Count() ) );
	const HardwareCaps_t &caps = m_Adapters[ nAdapter ].m_ActualCaps;
	//Log( "Driver name: %s\n", caps.m_pDriverName );
	memcpy( &info, &caps, sizeof(MaterialAdapterInfo_t) );
}


//-----------------------------------------------------------------------------
// Returns the adapter interface for a particular adapter
//-----------------------------------------------------------------------------
IDXGIAdapter* CShaderDeviceMgrDx11::GetAdapter( int nAdapter ) const
{
	Assert( m_pDXGIFactory );

	IDXGIAdapter *pAdapter;
	HRESULT hr = m_pDXGIFactory->EnumAdapters( nAdapter, &pAdapter );
	return ( FAILED(hr) ) ? NULL : pAdapter;
}


//-----------------------------------------------------------------------------
// Returns the amount of video memory in bytes for a particular adapter
//-----------------------------------------------------------------------------
int CShaderDeviceMgrDx11::GetVidMemBytes( int nAdapter ) const
{
	LOCK_SHADERAPI();
	IDXGIAdapter *pAdapter = GetAdapter( nAdapter );
	if ( !pAdapter )
		return 0;

	DXGI_ADAPTER_DESC desc;

#ifdef _DEBUG
	HRESULT hr = 
#endif
		pAdapter->GetDesc( &desc );
	Assert( !FAILED( hr ) );
	return desc.DedicatedVideoMemory;
}


//-----------------------------------------------------------------------------
// Returns the appropriate adapter output to use
//-----------------------------------------------------------------------------
IDXGIOutput* CShaderDeviceMgrDx11::GetAdapterOutput( int nAdapter ) const
{
	LOCK_SHADERAPI();
	IDXGIAdapter *pAdapter = GetAdapter( nAdapter );
	if ( !pAdapter )
		return 0;

	IDXGIOutput *pOutput;
	for( UINT i = 0; pAdapter->EnumOutputs( i, &pOutput ) != DXGI_ERROR_NOT_FOUND; ++i )
	{
		DXGI_OUTPUT_DESC desc;
		HRESULT hr = pOutput->GetDesc( &desc );
		if ( FAILED( hr ) )
			continue;

		// FIXME: Is this what I want? Or should I be looking at other fields,
		// like DXGI_MODE_ROTATION_IDENTITY?
		if ( !desc.AttachedToDesktop )
			continue;

		return pOutput;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Returns the number of modes
//-----------------------------------------------------------------------------
int CShaderDeviceMgrDx11::GetModeCount( int nAdapter ) const
{
	LOCK_SHADERAPI();
	Assert( m_pDXGIFactory && ( nAdapter < GetAdapterCount() ) );

	IDXGIOutput *pOutput = GetAdapterOutput( nAdapter );
	if ( !pOutput )
		return 0;
	
	UINT num = 0;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //desired color format
	UINT flags         = 0; //desired scanline order and/or scaling

	// get the number of available display mode for the given format and scanline order
	HRESULT hr = pOutput->GetDisplayModeList( format, flags, &num, 0 );
	return ( FAILED(hr) ) ? 0 : num;
}


//-----------------------------------------------------------------------------
// Returns mode information..
//-----------------------------------------------------------------------------
void CShaderDeviceMgrDx11::GetModeInfo( ShaderDisplayMode_t* pInfo, int nAdapter, int nMode ) const
{
	// Default error state
	pInfo->m_nWidth = pInfo->m_nHeight = 0;
	pInfo->m_Format = IMAGE_FORMAT_UNKNOWN;
	pInfo->m_nRefreshRateNumerator = pInfo->m_nRefreshRateDenominator = 0;

	LOCK_SHADERAPI();
	Assert( m_pDXGIFactory && ( nAdapter < GetAdapterCount() ) );

	IDXGIOutput *pOutput = GetAdapterOutput( nAdapter );
	if ( !pOutput )
		return;

	UINT num = 0;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //desired color format
	UINT flags         = DXGI_ENUM_MODES_INTERLACED; //desired scanline order and/or scaling

	// get the number of available display mode for the given format and scanline order
	HRESULT hr = pOutput->GetDisplayModeList( format, flags, &num, 0 );
	Assert( !FAILED( hr ) );

	if ( (UINT)nMode >= num )
		return;

	DXGI_MODE_DESC *pDescs = (DXGI_MODE_DESC*)_alloca( num * sizeof( DXGI_MODE_DESC ) );
	hr = pOutput->GetDisplayModeList( format, flags, &num, pDescs );
	Assert( !FAILED( hr ) );

	pInfo->m_nWidth      = pDescs[nMode].Width;
	pInfo->m_nHeight     = pDescs[nMode].Height;
//	pInfo->m_Format      = ImageLoader::D3DFormatToImageFormat( pDescs[nMode].Format );
	pInfo->m_nRefreshRateNumerator = pDescs[nMode].RefreshRate.Numerator;
	pInfo->m_nRefreshRateDenominator = pDescs[nMode].RefreshRate.Denominator;
}


//-----------------------------------------------------------------------------
// Returns the current mode for an adapter
//-----------------------------------------------------------------------------
void CShaderDeviceMgrDx11::GetCurrentModeInfo( ShaderDisplayMode_t* pInfo, int nAdapter ) const
{
	// FIXME: Implement!
	Assert( 0 );
}


//-----------------------------------------------------------------------------
// Initialization, shutdown
//-----------------------------------------------------------------------------
bool CShaderDeviceMgrDx11::SetAdapter( int nAdapter, int nFlags )
{
	Assert( m_bSetupAdapters && nAdapter < m_Adapters.Count() );
	HardwareCaps_t &actualCaps = g_pHardwareConfig->ActualCapsForEdit();
	ComputeCapsFromD3D( &actualCaps, GetAdapter( nAdapter ), GetAdapterOutput( nAdapter ) );
	ReadDXSupportLevels( actualCaps );
	ReadHardwareCaps( actualCaps, actualCaps.m_nMaxDXSupportLevel );

	// What's in "-shader" overrides dxsupport.cfg
	const char *pShaderParam = CommandLine()->ParmValue( "-shader" );
	if ( pShaderParam )
	{
		Q_strncpy( actualCaps.m_pShaderDLL, pShaderParam, sizeof( actualCaps.m_pShaderDLL ) );
	}

	g_pHardwareConfig->SetupHardwareCaps( actualCaps.m_nDXSupportLevel, actualCaps );

	g_pShaderDevice = g_pShaderDeviceDx11;
	g_pShaderDeviceDx11->m_nAdapter = nAdapter;

	return true;
}


//-----------------------------------------------------------------------------
// Sets the mode
//-----------------------------------------------------------------------------
CreateInterfaceFn CShaderDeviceMgrDx11::SetMode( void *hWnd, int nAdapter, const ShaderDeviceInfo_t& mode )
{
	LOCK_SHADERAPI();

	//Log("Calling CShaderDeviceMgrDx11::SetMode()\n");

	Assert( nAdapter < GetAdapterCount() );
	int nDXLevel = mode.m_nDXLevel != 0 ? mode.m_nDXLevel : m_Adapters[nAdapter].m_ActualCaps.m_nDXSupportLevel;
	if ( m_bObeyDxCommandlineOverride )
	{
		nDXLevel = CommandLine()->ParmValue( "-dxlevel", nDXLevel );
		m_bObeyDxCommandlineOverride = false;
	}
	if ( nDXLevel > m_Adapters[nAdapter].m_ActualCaps.m_nMaxDXSupportLevel )
	{
		nDXLevel = m_Adapters[nAdapter].m_ActualCaps.m_nMaxDXSupportLevel;
	}
	nDXLevel = GetClosestActualDXLevel( nDXLevel );
	Log("nDXLevel: %i\n", nDXLevel);
	if ( nDXLevel < 110 )
	{
		// Fall back to the Dx9 implementations
		//return g_pShaderDeviceMgrDx8->SetMode( hWnd, nAdapter, mode );
		Error( "DirectX 11 unsupported!" );
		Assert( 0 );
		return NULL;
	}

	if ( g_pShaderAPI )
	{
		g_pShaderAPI->OnDeviceShutdown();
		g_pShaderAPI = NULL;
	}

	if ( g_pShaderDevice )
	{
		g_pShaderDevice->ShutdownDevice();
		g_pShaderDevice = NULL;
	}

	g_pShaderShadow = NULL;

	ShaderDeviceInfo_t adjustedMode = mode;
	adjustedMode.m_nDXLevel = nDXLevel;
	if ( !g_pShaderDeviceDx11->InitDevice( hWnd, nAdapter, adjustedMode ) )
		return NULL;

	if ( !g_pShaderAPIDx11->OnDeviceInit() )
		return NULL;

	g_pShaderDevice = g_pShaderDeviceDx11;
	g_pShaderAPI = g_pShaderAPIDx11;
	g_pShaderShadow = g_pShaderShadowDx11;

	return ShaderInterfaceFactory;
}


//-----------------------------------------------------------------------------
//
// Device
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CShaderDeviceDx11::CShaderDeviceDx11() :
	m_ConstantBuffers( 32 )
{
	m_pDevice = NULL;
	m_pDeviceContext = NULL;
	m_pOutput = NULL;
	m_pSwapChain = NULL;
	m_bDeviceInitialized = false;
}

CShaderDeviceDx11::~CShaderDeviceDx11()
{
}


//-----------------------------------------------------------------------------
// Sets the mode
//-----------------------------------------------------------------------------
bool CShaderDeviceDx11::InitDevice( void *hWnd, int nAdapter, const ShaderDeviceInfo_t& mode )
{
	// Make sure we've been shutdown previously
	if ( m_bDeviceInitialized )
	{
		Warning( "CShaderDeviceDx11::SetMode: Previous mode has not been shut down!\n" );
		return false;
	}

	LOCK_SHADERAPI();
	IDXGIAdapter *pAdapter = g_ShaderDeviceMgrDx11.GetAdapter( nAdapter );
	if ( !pAdapter )
		return false;

	m_pOutput = g_ShaderDeviceMgrDx11.GetAdapterOutput( nAdapter );
	if ( !m_pOutput )
		return false;
	m_pOutput->AddRef();

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof(sd) );
	sd.BufferDesc.Width = mode.m_DisplayMode.m_nWidth;
	sd.BufferDesc.Height = mode.m_DisplayMode.m_nHeight;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = mode.m_DisplayMode.m_nRefreshRateNumerator;
	sd.BufferDesc.RefreshRate.Denominator = mode.m_DisplayMode.m_nRefreshRateDenominator;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = mode.m_nBackBufferCount;
	sd.OutputWindow = (HWND)hWnd;
	sd.Windowed = mode.m_bWindowed ? TRUE : FALSE;
	sd.Flags = mode.m_bWindowed ? 0 : DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// NOTE: Having more than 1 back buffer disables MSAA!
	sd.SwapEffect = mode.m_nBackBufferCount > 1 ? DXGI_SWAP_EFFECT_SEQUENTIAL : DXGI_SWAP_EFFECT_DISCARD;

	// FIXME: Chicken + egg problem with SampleDesc.
	sd.SampleDesc.Count = mode.m_nAASamples ? mode.m_nAASamples : 1;
	sd.SampleDesc.Quality = mode.m_nAAQuality;

	UINT nDeviceFlags = 0;
#ifdef _DEBUG
	nDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT hr = D3D11CreateDeviceAndSwapChain( pAdapter, D3D_DRIVER_TYPE_UNKNOWN,
						    NULL, nDeviceFlags, NULL, 0, D3D11_SDK_VERSION, &sd, &m_pSwapChain,
						    &m_pDevice, NULL, &m_pDeviceContext );

	if ( FAILED( hr ) )
		return false;

	m_hWnd = hWnd;
	m_nAdapter = nAdapter;

	// This is our current view.
	m_ViewHWnd = hWnd;
	GetWindowSize( m_nWindowWidth, m_nWindowHeight );

	Log( "InitDevice: setupHardwareCaps\n" );
	const HardwareCaps_t &caps = g_ShaderDeviceMgrDx11.GetHardwareCaps( nAdapter );
	g_pHardwareConfig->SetupHardwareCaps( mode, caps );

	// Create shared constant buffers
	m_hTransformBuffer = CreateConstantBuffer( sizeof( TransformBuffer_t ) );
	m_hLightingBuffer = CreateConstantBuffer( sizeof( LightingBuffer_t ) );

	m_bDeviceInitialized = true;

	return true;
}


//-----------------------------------------------------------------------------
// Shuts down the mode
//-----------------------------------------------------------------------------
void CShaderDeviceDx11::ShutdownDevice() 
{
	if ( m_pDeviceContext )
	{
		m_pDeviceContext->Release();
		m_pDeviceContext = NULL;
	}

	if ( m_pDevice )
	{
		m_pDevice->Release();
		m_pDevice = NULL;
	}

	if ( m_pSwapChain )
	{
		m_pSwapChain->Release();
		m_pSwapChain = NULL;
	}

	if ( m_pOutput )
	{
		m_pOutput->Release();
		m_pOutput = NULL;
	}

	m_hWnd = NULL;
	//m_nAdapter = -1;
	m_bDeviceInitialized = false;
}


//-----------------------------------------------------------------------------
// Are we using graphics?
//-----------------------------------------------------------------------------
bool CShaderDeviceDx11::IsUsingGraphics() const
{
	return m_bDeviceInitialized;
}


//-----------------------------------------------------------------------------
// Returns the adapter
//-----------------------------------------------------------------------------
int CShaderDeviceDx11::GetCurrentAdapter() const
{
	return m_nAdapter;
}


//-----------------------------------------------------------------------------
// Get back buffer information
//-----------------------------------------------------------------------------
ImageFormat CShaderDeviceDx11::GetBackBufferFormat() const
{
	return IMAGE_FORMAT_RGB888;
}

void CShaderDeviceDx11::GetBackBufferDimensions( int& width, int& height ) const
{
	//width = 1024;
	//height = 768;
	DXGI_SWAP_CHAIN_DESC desc;
	m_pSwapChain->GetDesc( &desc );
	width = desc.BufferDesc.Width;
	height = desc.BufferDesc.Height;
}


//-----------------------------------------------------------------------------
// Use this to spew information about the 3D layer 
//-----------------------------------------------------------------------------
void CShaderDeviceDx11::SpewDriverInfo() const
{
	Warning( "Dx11 Driver!\n" );
}



//-----------------------------------------------------------------------------
// Swap buffers
//-----------------------------------------------------------------------------
void CShaderDeviceDx11::Present()
{
	// Draw buffered primitives
	g_pShaderAPIDx11->FlushBufferedPrimitives();

	// FIXME: Deal with window occlusion, alt-tab, etc.
	HRESULT hr = m_pSwapChain->Present( 0, 0 );
	if ( FAILED(hr) )
	{
		HRESULT removed = D3D11Device()->GetDeviceRemovedReason();
		Assert( 0 );
	}

	// Make all vertex and index buffers flush
	// the next time they are locked.
	MeshMgr()->DiscardVertexBuffers();
}


//-----------------------------------------------------------------------------
// Camma ramp
//-----------------------------------------------------------------------------
void CShaderDeviceDx11::SetHardwareGammaRamp( float fGamma, float fGammaTVRangeMin, float fGammaTVRangeMax, float fGammaTVExponent, bool bTVEnabled )
{
	DevMsg( "SetHardwareGammaRamp( %f )\n", fGamma );

	Assert( m_pOutput );
	if( !m_pOutput )
		return;

	float flMin = g_pHardwareConfig->Caps().m_flMinGammaControlPoint;
	float flMax = g_pHardwareConfig->Caps().m_flMaxGammaControlPoint;
	int nGammaPoints = g_pHardwareConfig->Caps().m_nGammaControlPointCount;

	DXGI_GAMMA_CONTROL gammaControl;
	gammaControl.Scale.Red = gammaControl.Scale.Green = gammaControl.Scale.Blue = 1.0f;
	gammaControl.Offset.Red = gammaControl.Offset.Green = gammaControl.Offset.Blue = 0.0f;
	float flOOCount = 1.0f / ( nGammaPoints - 1 );
	for ( int i = 0; i < nGammaPoints; i++ )
	{
		float flGamma22 = i * flOOCount;
		float flCorrection = pow( flGamma22, fGamma / 2.2f );
		flCorrection = clamp( flCorrection, flMin, flMax );

		gammaControl.GammaCurve[i].Red = flCorrection;
		gammaControl.GammaCurve[i].Green = flCorrection;
		gammaControl.GammaCurve[i].Blue = flCorrection;
	}

	HRESULT hr = m_pOutput->SetGammaControl( &gammaControl );
	if ( FAILED(hr) )
	{
		Warning( "CShaderDeviceDx11::SetHardwareGammaRamp: Unable to set gamma controls!\n" );
	}
}


//-----------------------------------------------------------------------------
// Compiles all manner of shaders
//-----------------------------------------------------------------------------
IShaderBuffer* CShaderDeviceDx11::CompileShader( const char *pProgram, size_t nBufLen, const char *pShaderVersion )
{
	int nCompileFlags = D3DCOMPILE_AVOID_FLOW_CONTROL;
	nCompileFlags |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;

#ifdef _DEBUG
	nCompileFlags |= D3DCOMPILE_DEBUG;
#endif

	ID3DBlob *pCompiledShader, *pErrorMessages;
	HRESULT hr = D3DCompile( pProgram, nBufLen, "",
		NULL, NULL, "main", pShaderVersion, nCompileFlags, 0, &pCompiledShader, 
		&pErrorMessages );

	if ( FAILED( hr ) )
	{
		if ( pErrorMessages )
		{
			const char *pErrorMessage = (const char *)pErrorMessages->GetBufferPointer();
			Warning( "Vertex shader compilation failed! Reported the following errors:\n%s\n", pErrorMessage );
			pErrorMessages->Release();
		}
		return NULL;
	}

	// NOTE: This uses small block heap allocator; so I'm not going
	// to bother creating a memory pool.
	CShaderBuffer< ID3DBlob > *pShaderBuffer = new CShaderBuffer< ID3DBlob >( pCompiledShader );
	if ( pErrorMessages )
	{
		pErrorMessages->Release();
	}

	return pShaderBuffer;
}


//-----------------------------------------------------------------------------
// Release input layouts
//-----------------------------------------------------------------------------
void CShaderDeviceDx11::ReleaseInputLayouts( VertexShaderIndex_t nIndex )
{
	InputLayoutDict_t &dict = m_VertexShaderDict[nIndex].m_InputLayouts;
	unsigned short hCurr = dict.FirstInorder();
	while( hCurr != dict.InvalidIndex() )
	{
		if ( dict[hCurr].m_pInputLayout )
		{
			dict[hCurr].m_pInputLayout->Release();
			dict[hCurr].m_pInputLayout = NULL;
		}
		hCurr = dict.NextInorder( hCurr );
	}
}


//-----------------------------------------------------------------------------
// Create, destroy vertex shader
//-----------------------------------------------------------------------------
VertexShaderHandle_t CShaderDeviceDx11::CreateVertexShader( IShaderBuffer* pShaderBuffer )
{
	return CreateVertexShader( pShaderBuffer->GetBits(), pShaderBuffer->GetSize() );
}

VertexShaderHandle_t CShaderDeviceDx11::CreateVertexShader( const void* pBuffer, size_t nBufLen )
{
	// Create the vertex shader
	ID3D11VertexShader* pShader = NULL;
	Log( "Creating vertex shader with len %u\n", nBufLen );
	HRESULT hr = m_pDevice->CreateVertexShader( pBuffer, nBufLen, NULL, &pShader );

	if ( FAILED( hr ) || !pShader )
	{
		Log( "D3D11Device->CreateVertexShader() failed: error %i\n", hr );
		return VERTEX_SHADER_HANDLE_INVALID;
	}

	ID3D11ShaderReflection* pInfo;
	hr = D3DReflect( pBuffer, nBufLen, IID_ID3D11ShaderReflection, (void**)&pInfo );
	if ( FAILED( hr ) || !pInfo )
	{
		pShader->Release();
		return VERTEX_SHADER_HANDLE_INVALID;
	}

	// Insert the shader into the dictionary of shaders
	VertexShaderIndex_t i = m_VertexShaderDict.AddToTail();
	VertexShader_t& dict = m_VertexShaderDict[i];
	dict.m_pShader = pShader;
	dict.m_pInfo = pInfo;
	dict.m_nByteCodeLen = nBufLen;
	dict.m_pByteCode = new unsigned char[dict.m_nByteCodeLen];
	memcpy( dict.m_pByteCode, pBuffer, dict.m_nByteCodeLen );
	return (VertexShaderHandle_t)i;
}

void CShaderDeviceDx11::DestroyVertexShader( VertexShaderHandle_t hShader )
{
	if ( hShader == VERTEX_SHADER_HANDLE_INVALID )
		return;

	g_pShaderAPIDx11->Unbind( hShader );

	VertexShaderIndex_t i = (VertexShaderIndex_t)hShader;
	VertexShader_t &dict = m_VertexShaderDict[i];
	VerifyEquals( dict.m_pShader->Release(), 0 );
	VerifyEquals( dict.m_pInfo->Release(), 0 );
	delete[] dict.m_pByteCode;
	ReleaseInputLayouts( i );
	m_VertexShaderDict.Remove( i );
}


//-----------------------------------------------------------------------------
// Create, destroy geometry shader
//-----------------------------------------------------------------------------
GeometryShaderHandle_t CShaderDeviceDx11::CreateGeometryShader( IShaderBuffer* pShaderBuffer )
{
	return CreateGeometryShader( pShaderBuffer->GetBits(), pShaderBuffer->GetSize() );
}

GeometryShaderHandle_t CShaderDeviceDx11::CreateGeometryShader( const void* pBuffer, size_t nBufLen )
{
	// Create the geometry shader
	ID3D11GeometryShader* pShader = NULL;
	HRESULT hr = m_pDevice->CreateGeometryShader( pBuffer,
						      nBufLen, NULL, &pShader );

	if ( FAILED( hr ) || !pShader )
		return GEOMETRY_SHADER_HANDLE_INVALID;

	ID3D11ShaderReflection* pInfo;
	hr = D3DReflect( pBuffer, nBufLen, IID_ID3D11ShaderReflection, (void**)&pInfo );
	if ( FAILED( hr ) || !pInfo )
	{
		pShader->Release();
		return GEOMETRY_SHADER_HANDLE_INVALID;
	}

	// Insert the shader into the dictionary of shaders
	GeometryShaderIndex_t i = m_GeometryShaderDict.AddToTail();
	m_GeometryShaderDict[i].m_pShader = pShader;
	m_GeometryShaderDict[i].m_pInfo = pInfo;
	return (GeometryShaderHandle_t)i;
}

void CShaderDeviceDx11::DestroyGeometryShader( GeometryShaderHandle_t hShader )
{
	if ( hShader == GEOMETRY_SHADER_HANDLE_INVALID )
		return;

	g_pShaderAPIDx11->Unbind( hShader );

	GeometryShaderIndex_t i = (GeometryShaderIndex_t)hShader;
	VerifyEquals( m_GeometryShaderDict[ i ].m_pShader->Release(), 0 );
	VerifyEquals( m_GeometryShaderDict[ i ].m_pInfo->Release(), 0 );
	m_GeometryShaderDict.Remove( i );
}


//-----------------------------------------------------------------------------
// Create, destroy pixel shader
//-----------------------------------------------------------------------------
PixelShaderHandle_t CShaderDeviceDx11::CreatePixelShader( IShaderBuffer* pShaderBuffer )
{
	return CreatePixelShader( pShaderBuffer->GetBits(), pShaderBuffer->GetSize() );
}

PixelShaderHandle_t CShaderDeviceDx11::CreatePixelShader( const void* pBuffer, size_t nBufLen )
{
	Log( "Creating pixel shader with len %u\n", nBufLen );
	// Create the pixel shader
	ID3D11PixelShader* pShader = NULL;
	HRESULT hr = m_pDevice->CreatePixelShader( pBuffer,
						   nBufLen, NULL, &pShader );

	if ( FAILED( hr ) || !pShader )
		return PIXEL_SHADER_HANDLE_INVALID;

	ID3D11ShaderReflection* pInfo;
	hr = D3DReflect( pBuffer, nBufLen, IID_ID3D11ShaderReflection, (void**)&pInfo );
	if ( FAILED( hr ) || !pInfo )
	{
		pShader->Release();
		return PIXEL_SHADER_HANDLE_INVALID;
	}

	// Insert the shader into the dictionary of shaders
	PixelShaderIndex_t i = m_PixelShaderDict.AddToTail();
	m_PixelShaderDict[i].m_pShader = pShader;
	m_PixelShaderDict[i].m_pInfo = pInfo;
	return (PixelShaderHandle_t)i;
}

void CShaderDeviceDx11::DestroyPixelShader( PixelShaderHandle_t hShader )
{
	if ( hShader == PIXEL_SHADER_HANDLE_INVALID )
		return;

	g_pShaderAPIDx11->Unbind( hShader );

	PixelShaderIndex_t i = (PixelShaderIndex_t)hShader;
	VerifyEquals( m_PixelShaderDict[ i ].m_pShader->Release(), 0 );
	VerifyEquals( m_PixelShaderDict[ i ].m_pInfo->Release(), 0 );
	m_PixelShaderDict.Remove( i );
}


//-----------------------------------------------------------------------------
// Finds or creates an input layout for a given vertex shader + stream format
//-----------------------------------------------------------------------------
ID3D11InputLayout* CShaderDeviceDx11::GetInputLayout( VertexShaderHandle_t hShader, VertexFormat_t format )
{
	if ( hShader == VERTEX_SHADER_HANDLE_INVALID )
		return NULL;

	// FIXME: VertexFormat_t is not the appropriate way of specifying this
	// because it has no stream information
	InputLayout_t insert;
	insert.m_VertexFormat = format;

	VertexShaderIndex_t i = (VertexShaderIndex_t)hShader;
	InputLayoutDict_t &dict = m_VertexShaderDict[i].m_InputLayouts;
	unsigned short hIndex = dict.Find( insert );
	if ( hIndex != dict.InvalidIndex() )
		return dict[hIndex].m_pInputLayout;

	VertexShader_t &shader = m_VertexShaderDict[i];
	insert.m_pInputLayout = CreateInputLayout( format, shader.m_pInfo, shader.m_pByteCode, shader.m_nByteCodeLen );
	dict.Insert( insert );
	return insert.m_pInputLayout;
}


//-----------------------------------------------------------------------------
// Creates/destroys Mesh
//-----------------------------------------------------------------------------
IMesh* CShaderDeviceDx11::CreateStaticMesh( VertexFormat_t vertexFormat, const char *pBudgetGroup, IMaterial * pMaterial )
{
	LOCK_SHADERAPI();
	return g_pShaderAPIDx11->CreateStaticMesh( vertexFormat, pBudgetGroup, pMaterial );
}

void CShaderDeviceDx11::DestroyStaticMesh( IMesh* pMesh )
{
	LOCK_SHADERAPI();
	g_pShaderAPIDx11->DestroyStaticMesh( pMesh );
}


//-----------------------------------------------------------------------------
// Creates/destroys vertex buffers + index buffers
//-----------------------------------------------------------------------------
IVertexBuffer *CShaderDeviceDx11::CreateVertexBuffer( ShaderBufferType_t type, VertexFormat_t fmt, int nVertexCount, const char *pBudgetGroup )
{
	LOCK_SHADERAPI();
	CVertexBufferDx11 *pVertexBuffer = new CVertexBufferDx11( type, fmt, nVertexCount, pBudgetGroup );
	return pVertexBuffer;
}

void CShaderDeviceDx11::DestroyVertexBuffer( IVertexBuffer *pVertexBuffer )
{
	LOCK_SHADERAPI();
	if ( pVertexBuffer )
	{
		CVertexBufferDx11 *pVertexBufferBase = assert_cast<CVertexBufferDx11*>( pVertexBuffer );
		g_pShaderAPIDx11->UnbindVertexBuffer( pVertexBufferBase->GetDx11Buffer() );
		delete pVertexBufferBase;
	}
}

IIndexBuffer *CShaderDeviceDx11::CreateIndexBuffer( ShaderBufferType_t type, MaterialIndexFormat_t fmt, int nIndexCount, const char *pBudgetGroup )
{
	LOCK_SHADERAPI();
	CIndexBufferDx11 *pIndexBuffer = new CIndexBufferDx11( type, fmt, nIndexCount, pBudgetGroup );
	return pIndexBuffer;
}

void CShaderDeviceDx11::DestroyIndexBuffer( IIndexBuffer *pIndexBuffer )
{
	LOCK_SHADERAPI();
	if ( pIndexBuffer )
	{
		CIndexBufferDx11 *pIndexBufferBase = assert_cast<CIndexBufferDx11*>( pIndexBuffer );
		g_pShaderAPIDx11->UnbindIndexBuffer( pIndexBufferBase->GetDx11Buffer() );
		delete pIndexBufferBase;
	}
}

ConstantBuffer_t CShaderDeviceDx11::CreateConstantBuffer( size_t nBufLen )
{
	CShaderConstantBufferDx11 buf;
	buf.Create( nBufLen );
	return m_ConstantBuffers.AddToTail( buf );
}

void CShaderDeviceDx11::UpdateConstantBuffer( ConstantBuffer_t hBuffer, void *pData )
{
	Assert( m_ConstantBuffers.IsValidIndex( hBuffer ) );
	CShaderConstantBufferDx11 &buf = m_ConstantBuffers.Element( hBuffer );
	buf.Update( pData );
}

void CShaderDeviceDx11::UploadConstantBuffers( ConstantBuffer_t *pBuffers, int nBuffers )
{
	for ( int i = 0; i < nBuffers; i++ )
	{
		CShaderConstantBufferDx11 &buf = m_ConstantBuffers.Element( pBuffers[i] );
		buf.UploadToGPU();
	}
}

ConstantBufferHandle_t CShaderDeviceDx11::GetConstantBuffer( ConstantBuffer_t iBuffer )
{
	return (ConstantBufferHandle_t)&m_ConstantBuffers.Element( iBuffer );
}

ConstantBuffer_t CShaderDeviceDx11::GetInternalConstantBuffer( int buffer )
{
	switch ( buffer )
	{
	case SHADER_INTERNAL_CONSTANTBUFFER_TRANSFORM:
		return m_hTransformBuffer;
	case SHADER_INTERNAL_CONSTANTBUFFER_LIGHTING:
		return m_hLightingBuffer;
	case SHADER_INTERNAL_CONSTANTBUFFER_FOG:
		return m_hFogBuffer;
	default:
		return CONSTANT_BUFFER_INVALID;
	}
}

void CShaderDeviceDx11::DestroyConstantBuffer( ConstantBuffer_t hBuffer )
{
	Assert( m_ConstantBuffers.IsValidIndex( hBuffer ) );
	CShaderConstantBufferDx11 &buf = m_ConstantBuffers.Element( hBuffer );
	buf.Destroy();
	m_ConstantBuffers.Remove( hBuffer );
}

// NOTE: I don't see these functions being called by anybody. I think they should be removed.
IVertexBuffer *CShaderDeviceDx11::GetDynamicVertexBuffer( int nStreamID, VertexFormat_t vertexFormat, bool bBuffered )
{
	LOCK_SHADERAPI();
	return NULL;
}

IIndexBuffer *CShaderDeviceDx11::GetDynamicIndexBuffer( MaterialIndexFormat_t fmt, bool bBuffered )
{
	LOCK_SHADERAPI();
	return NULL;
}

ShaderAPIOcclusionQuery_t CShaderDeviceDx11::CreateOcclusionQuery()
{
	CD3D11_QUERY_DESC desc;
	desc.Query = D3D11_QUERY_OCCLUSION;
	desc.MiscFlags = 0;
	ID3D11Query *pQuery = NULL;
	HRESULT hr = D3D11Device()->CreateQuery( &desc, &pQuery );
	if ( FAILED( hr ) )
	{
		return INVALID_SHADERAPI_OCCLUSION_QUERY_HANDLE;
	}
	return (ShaderAPIOcclusionQuery_t)pQuery;
}

void CShaderDeviceDx11::DestroyOcclusionQuery( ShaderAPIOcclusionQuery_t hQuery )
{
	if ( hQuery != INVALID_SHADERAPI_OCCLUSION_QUERY_HANDLE )
	{
		( (ID3D11Query *)hQuery )->Release();
	}
}
