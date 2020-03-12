//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef SHADERDEVICEDX11_H
#define SHADERDEVICEDX11_H

#ifdef _WIN32
#pragma once
#endif


#include "ShaderConstantBufferDx11.h"
#include "shaderapi/ishaderapi.h"
#include "shaderapidx9/shaderdevicebase.h"
#include "tier1/utlvector.h"
#include "tier1/utlrbtree.h"
#include "tier1/utllinkedlist.h"

//-----------------------------------------------------------------------------
// Forward declaration
//-----------------------------------------------------------------------------
struct IDXGIFactory;
struct IDXGIAdapter;
struct IDXGIOutput;
struct IDXGISwapChain;
struct ID3D11Device;
struct ID3D11RenderTargetView;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11GeometryShader;
struct ID3D11InputLayout;
struct ID3D11ShaderReflection;

//-----------------------------------------------------------------------------
// The Base implementation of the shader device
//-----------------------------------------------------------------------------
class CShaderDeviceMgrDx11 : public CShaderDeviceMgrBase
{
	typedef CShaderDeviceMgrBase BaseClass;

public:
	// constructor, destructor
	CShaderDeviceMgrDx11();
	virtual ~CShaderDeviceMgrDx11();

	// Methods of IAppSystem
	virtual bool Connect( CreateInterfaceFn factory );
	virtual void Disconnect();
	virtual InitReturnVal_t Init();
	virtual void Shutdown();

	// Methods of IShaderDeviceMgr
	virtual int	 GetAdapterCount() const;
	virtual void GetAdapterInfo( int adapter, MaterialAdapterInfo_t& info ) const;
	virtual int	 GetModeCount( int nAdapter ) const;
	virtual void GetModeInfo( ShaderDisplayMode_t* pInfo, int nAdapter, int mode ) const;
	virtual void GetCurrentModeInfo( ShaderDisplayMode_t* pInfo, int nAdapter ) const;
	virtual bool SetAdapter( int nAdapter, int nFlags );
	virtual CreateInterfaceFn SetMode( void *hWnd, int nAdapter, const ShaderDeviceInfo_t& mode );

private:
	// Initialize adapter information
	void InitAdapterInfo();

	// Determines hardware caps from D3D
	bool ComputeCapsFromD3D( HardwareCaps_t *pCaps, IDXGIAdapter *pAdapter, IDXGIOutput *pOutput );

	// Returns the amount of video memory in bytes for a particular adapter
	virtual int GetVidMemBytes( int nAdapter ) const;

	// Returns the appropriate adapter output to use
	IDXGIOutput* GetAdapterOutput( int nAdapter ) const;

	// Returns the adapter interface for a particular adapter
	IDXGIAdapter* GetAdapter( int nAdapter ) const;

	// Used to enumerate adapters, attach to windows
	IDXGIFactory *m_pDXGIFactory;

	bool m_bObeyDxCommandlineOverride: 1;

	bool m_bSetupAdapters;

	friend class CShaderDeviceDx11;
};


//-----------------------------------------------------------------------------
// The Dx11 implementation of the shader device
//-----------------------------------------------------------------------------
class CShaderDeviceDx11 : public CShaderDeviceBase
{
public:
	// constructor, destructor
	CShaderDeviceDx11();
	virtual ~CShaderDeviceDx11();

public:
	// Methods of IShaderDevice
	virtual bool IsUsingGraphics() const;
	virtual int GetCurrentAdapter() const;
	virtual ImageFormat GetBackBufferFormat() const;
	virtual void GetBackBufferDimensions( int& width, int& height ) const;
	virtual void SpewDriverInfo() const;
	virtual void Present();
	virtual IShaderBuffer* CompileShader( const char *pProgram, size_t nBufLen, const char *pShaderVersion );
	virtual VertexShaderHandle_t CreateVertexShader( IShaderBuffer *pShader );
	VertexShaderHandle_t CreateVertexShader( const void* pBuffer, size_t nBufLen );
	virtual void DestroyVertexShader( VertexShaderHandle_t hShader );
	virtual GeometryShaderHandle_t CreateGeometryShader( IShaderBuffer* pShaderBuffer );
	GeometryShaderHandle_t CreateGeometryShader( const void* pBuffer, size_t nBufLen );
	virtual void DestroyGeometryShader( GeometryShaderHandle_t hShader );
	virtual PixelShaderHandle_t CreatePixelShader( IShaderBuffer* pShaderBuffer );
	PixelShaderHandle_t CreatePixelShader( const void* pBuffer, size_t nBufLen );
	virtual void DestroyPixelShader( PixelShaderHandle_t hShader );
	virtual void ReleaseResources() {}
	virtual void ReacquireResources() {}
	virtual IMesh* CreateStaticMesh( VertexFormat_t format, const char *pTextureBudgetGroup, IMaterial * pMaterial );
	virtual void DestroyStaticMesh( IMesh* mesh );
	virtual IVertexBuffer *CreateVertexBuffer( ShaderBufferType_t type, VertexFormat_t fmt, int nVertexCount, const char *pTextureBudgetGroup );
	virtual void DestroyVertexBuffer( IVertexBuffer *pVertexBuffer );
	virtual IIndexBuffer *CreateIndexBuffer( ShaderBufferType_t type, MaterialIndexFormat_t fmt, int nIndexCount, const char *pTextureBudgetGroup );
	virtual void DestroyIndexBuffer( IIndexBuffer *pIndexBuffer );
	virtual ConstantBufferHandle_t CreateConstantBuffer( size_t nBufSize );
	virtual void UpdateConstantBuffer( ConstantBufferHandle_t hBuffer, void *pData );
	virtual void UploadConstantBuffers( ConstantBufferHandle_t *pBuffers, int nBuffers );
	virtual void DestroyConstantBuffer( ConstantBufferHandle_t hBuffer );
	virtual ConstantBufferHandle_t GetInternalConstantBuffer( int type );
	virtual IVertexBuffer *GetDynamicVertexBuffer( int nStreamID, VertexFormat_t vertexFormat, bool bBuffered = true );
	virtual IIndexBuffer *GetDynamicIndexBuffer( MaterialIndexFormat_t fmt, bool bBuffered = true );
	virtual void SetHardwareGammaRamp( float fGamma, float fGammaTVRangeMin, float fGammaTVRangeMax, float fGammaTVExponent, bool bTVEnabled );

	// A special path used to tick the front buffer while loading on the 360
	virtual void EnableNonInteractiveMode( MaterialNonInteractiveMode_t mode, ShaderNonInteractiveInfo_t *pInfo ) {}
	virtual void RefreshFrontBufferNonInteractive( ) {}

	ShaderAPIOcclusionQuery_t CreateOcclusionQuery();
	void DestroyOcclusionQuery( ShaderAPIOcclusionQuery_t hQuery );

public:
	// Methods of CShaderDeviceBase
	virtual bool InitDevice( void *hWnd, int nAdapter, const ShaderDeviceInfo_t& mode );
	virtual void ShutdownDevice();
	virtual bool IsDeactivated() const { return !m_bDeviceInitialized; }

	// Other public methods
	ID3D11VertexShader* GetVertexShader( VertexShaderHandle_t hShader ) const;
	ID3D11GeometryShader* GetGeometryShader( GeometryShaderHandle_t hShader ) const;
	ID3D11PixelShader* GetPixelShader( PixelShaderHandle_t hShader ) const;
	ID3D11InputLayout* GetInputLayout( VertexShaderHandle_t hShader, VertexFormat_t format,
					   bool bStaticLit, bool bUsingFlex, bool bUsingMorph );

private:
	struct InputLayout_t
	{
		ID3D11InputLayout *m_pInputLayout;
		VertexFormat_t m_VertexFormat;
		bool m_bStaticLit;
		bool m_bUsingFlex;
		bool m_bUsingMorph;

		InputLayout_t()
		{
			m_pInputLayout = NULL;
			m_VertexFormat = VERTEX_FORMAT_UNKNOWN;
			m_bStaticLit = false;
			m_bUsingFlex = false;
			m_bUsingMorph = false;
		}
	};

	typedef CUtlRBTree< InputLayout_t, unsigned short > InputLayoutDict_t;

	static bool InputLayoutLessFunc( const InputLayout_t &lhs, const InputLayout_t &rhs )	
	{ 
		if ( lhs.m_VertexFormat != rhs.m_VertexFormat )
		{
			return lhs.m_VertexFormat < rhs.m_VertexFormat;
		}
		if ( lhs.m_bStaticLit != rhs.m_bStaticLit )
		{
			return lhs.m_bStaticLit < rhs.m_bStaticLit;
		}
		if ( lhs.m_bUsingFlex != rhs.m_bUsingFlex )
		{
			return lhs.m_bUsingFlex < rhs.m_bUsingFlex;
		}
		return lhs.m_bUsingMorph < rhs.m_bUsingMorph;	
	}

	struct VertexShader_t
	{
		ID3D11VertexShader *m_pShader;
		ID3D11ShaderReflection *m_pInfo;
		void *m_pByteCode;
		size_t m_nByteCodeLen;
		InputLayoutDict_t m_InputLayouts;

		VertexShader_t() : m_InputLayouts( 0, 0, InputLayoutLessFunc ) {}
	};

	struct GeometryShader_t
	{
		ID3D11GeometryShader *m_pShader;
		ID3D11ShaderReflection *m_pInfo;
	};

	struct PixelShader_t
	{
		ID3D11PixelShader *m_pShader;
		ID3D11ShaderReflection *m_pInfo;
	};

	typedef CUtlFixedLinkedList< VertexShader_t >::IndexType_t VertexShaderIndex_t;
	typedef CUtlFixedLinkedList< GeometryShader_t >::IndexType_t GeometryShaderIndex_t;
	typedef CUtlFixedLinkedList< PixelShader_t >::IndexType_t PixelShaderIndex_t;

	void SetupHardwareCaps();
	void ReleaseInputLayouts( VertexShaderIndex_t nIndex );

	bool m_bDeviceInitialized;

	IDXGIOutput *m_pOutput;
	ID3D11Device *m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;
	IDXGISwapChain *m_pSwapChain;

	CUtlFixedLinkedList< VertexShader_t > m_VertexShaderDict;
	CUtlFixedLinkedList< GeometryShader_t > m_GeometryShaderDict;
	CUtlFixedLinkedList< PixelShader_t > m_PixelShaderDict;

	friend ID3D11Device *D3D11Device();
	friend ID3D11DeviceContext *D3D11DeviceContext();
	friend IDXGISwapChain *D3D11SwapChain();

	friend class CShaderDeviceMgrDx11;
};

//-----------------------------------------------------------------------------
// Inline methods of CShaderDeviceDx11
//-----------------------------------------------------------------------------
inline ID3D11VertexShader* CShaderDeviceDx11::GetVertexShader( VertexShaderHandle_t hShader ) const
{
	if ( hShader != VERTEX_SHADER_HANDLE_INVALID )
		return m_VertexShaderDict[ (VertexShaderIndex_t)hShader ].m_pShader;
	return NULL;
}

inline ID3D11GeometryShader* CShaderDeviceDx11::GetGeometryShader( GeometryShaderHandle_t hShader ) const
{
	if ( hShader != GEOMETRY_SHADER_HANDLE_INVALID )
		return m_GeometryShaderDict[ (GeometryShaderIndex_t)hShader ].m_pShader;
	return NULL;
}

inline ID3D11PixelShader* CShaderDeviceDx11::GetPixelShader( PixelShaderHandle_t hShader ) const
{
	if ( hShader != PIXEL_SHADER_HANDLE_INVALID )
		return m_PixelShaderDict[ (PixelShaderIndex_t)hShader ].m_pShader;
	return NULL;
}

//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
extern CShaderDeviceDx11* g_pShaderDeviceDx11;


//-----------------------------------------------------------------------------
// Utility methods
//-----------------------------------------------------------------------------
inline ID3D11Device *D3D11Device()
{
	return g_pShaderDeviceDx11->m_pDevice;	
}

inline ID3D11DeviceContext *D3D11DeviceContext()
{
	return g_pShaderDeviceDx11->m_pDeviceContext;
}

inline IDXGISwapChain *D3D11SwapChain()
{
	return g_pShaderDeviceDx11->m_pSwapChain;	
}

#endif // SHADERDEVICEDX11_H