#include "TextureDx11.h"
#include "shaderapi/ishaderapi.h"
#include "shaderdevicedx11.h"
#include "shaderapidx11_global.h"
#include "shaderapi/ishaderutil.h"
#include "filesystem.h"
#include "bitmap/tgawriter.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

// Returns a matching ShaderResourceView format for the depth texture's format.
FORCEINLINE DXGI_FORMAT GetDepthSRVFormat( DXGI_FORMAT depthResourceFormat )
{
	switch ( depthResourceFormat )
	{
	case DXGI_FORMAT_R16_TYPELESS:
		return DXGI_FORMAT_R16_UNORM;
	case DXGI_FORMAT_R24G8_TYPELESS:
		return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	case DXGI_FORMAT_R32_TYPELESS:
		return DXGI_FORMAT_R32_FLOAT;
	case DXGI_FORMAT_R32G8X24_TYPELESS:
		return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

// Returns a matching DepthStencilView format for the depth texture's format.
FORCEINLINE DXGI_FORMAT GetDepthStencilViewFormat( DXGI_FORMAT depthResourceFormat )
{
	switch ( depthResourceFormat )
	{
	case DXGI_FORMAT_R16_TYPELESS:
		return DXGI_FORMAT_D16_UNORM;
	case DXGI_FORMAT_R24G8_TYPELESS:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case DXGI_FORMAT_R32_TYPELESS:
		return DXGI_FORMAT_D32_FLOAT;
	case DXGI_FORMAT_R32G8X24_TYPELESS:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

// We will have to convert srcFormat into what this function
// returns for use in D3D11.
ImageFormat CTextureDx11::GetClosestSupportedImageFormatForD3D11( ImageFormat srcFormat )
{
	switch ( srcFormat )
	{
	case IMAGE_FORMAT_BGR888:
		return IMAGE_FORMAT_BGRA8888;

	case IMAGE_FORMAT_BGR888_SRGB:
	case IMAGE_FORMAT_BGR888_BLUESCREEN:
		return IMAGE_FORMAT_BGRA8888_SRGB;

	case IMAGE_FORMAT_RGB888:
		return IMAGE_FORMAT_RGBA8888;

	///case IMAGE_FORMAT_DXT1_ONEBITALPHA:
	///case IMAGE_FORMAT_DXT1:
	///case IMAGE_FORMAT_DXT5:
	//	return IMAGE_FORMAT_BGRA8888;
	//case IMAGE_FORMAT_DXT1_ONEBITALPHA_SRGB:
	//case IMAGE_FORMAT_DXT1_SRGB:
	//case IMAGE_FORMAT_DXT5_SRGB:
	//	return IMAGE_FORMAT_BGRA8888_SRGB;	

	case IMAGE_FORMAT_RGB888_SRGB:
	case IMAGE_FORMAT_RGB888_BLUESCREEN:
		return IMAGE_FORMAT_RGBA8888_SRGB;
	default:
		return srcFormat;
	}
}

DXGI_FORMAT GetD3DFormat( ImageFormat format )
{
	switch ( format )
	{
	// These are not exact but have the same number
	// of channels and bits-per-channel.
	case IMAGE_FORMAT_I8:
		return DXGI_FORMAT_R8_UNORM;
	case IMAGE_FORMAT_IA88:
		return DXGI_FORMAT_R8G8_UNORM;
	case IMAGE_FORMAT_UV88:
		return DXGI_FORMAT_R8G8_UNORM;
	case IMAGE_FORMAT_UVWQ8888:
	case IMAGE_FORMAT_UVLX8888:
		return DXGI_FORMAT_R8G8B8A8_UNORM;

	// Depth buffer formats
	case IMAGE_FORMAT_NV_DST24:
		return DXGI_FORMAT_R24G8_TYPELESS;
	case IMAGE_FORMAT_NV_DST16:
		return DXGI_FORMAT_R16_TYPELESS;

	// These ones match D3D.
	case IMAGE_FORMAT_A8:
		return DXGI_FORMAT_A8_UNORM;

	case IMAGE_FORMAT_DXT1:
	case IMAGE_FORMAT_DXT1_ONEBITALPHA:
		return DXGI_FORMAT_BC1_UNORM;
	case IMAGE_FORMAT_DXT1_SRGB:
	case IMAGE_FORMAT_DXT1_ONEBITALPHA_SRGB:
		return DXGI_FORMAT_BC1_UNORM_SRGB;

	case IMAGE_FORMAT_DXT3:
		return DXGI_FORMAT_BC2_UNORM;
	case IMAGE_FORMAT_DXT3_SRGB:
		return DXGI_FORMAT_BC2_UNORM_SRGB;

	case IMAGE_FORMAT_DXT5:
		return DXGI_FORMAT_BC3_UNORM;
	case IMAGE_FORMAT_DXT5_SRGB:
		return DXGI_FORMAT_BC3_UNORM_SRGB;

	case IMAGE_FORMAT_BGRA4444:
		return DXGI_FORMAT_B4G4R4A4_UNORM;
	case IMAGE_FORMAT_BGRX5551:
		return DXGI_FORMAT_B5G5R5A1_UNORM;
	case IMAGE_FORMAT_BGR565:
		return DXGI_FORMAT_B5G6R5_UNORM;

	case IMAGE_FORMAT_BGRX8888:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	case IMAGE_FORMAT_BGRX8888_SRGB:
		return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

	case IMAGE_FORMAT_BGRA8888:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	case IMAGE_FORMAT_BGRA8888_SRGB:
		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

	case IMAGE_FORMAT_RGBA16161616F:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case IMAGE_FORMAT_RGBA16161616:
		return DXGI_FORMAT_R16G16B16A16_UNORM;
	case IMAGE_FORMAT_R32F:
		return DXGI_FORMAT_R32_FLOAT;
	case IMAGE_FORMAT_RGBA32323232F:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;

	case IMAGE_FORMAT_RGBA8888:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case IMAGE_FORMAT_RGBA8888_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

ImageFormat CTextureDx11::GetImageFormat( DXGI_FORMAT d3dFormat )
{
	switch ( d3dFormat )
	{
	case DXGI_FORMAT_UNKNOWN:
		return IMAGE_FORMAT_UNKNOWN;

	// These are not exact but have the same number
	// of channels and bits-per-channel.
	case DXGI_FORMAT_R8_UNORM:
		return IMAGE_FORMAT_I8;
	case DXGI_FORMAT_R8G8_UNORM:
		return IMAGE_FORMAT_IA88;

	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return IMAGE_FORMAT_RGBA8888;
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return IMAGE_FORMAT_RGBA8888_SRGB;
	case DXGI_FORMAT_A8_UNORM:
		return IMAGE_FORMAT_A8;
	case DXGI_FORMAT_BC1_UNORM:
		return IMAGE_FORMAT_DXT1;
	case DXGI_FORMAT_BC1_UNORM_SRGB:
		return IMAGE_FORMAT_DXT1_SRGB;
	case DXGI_FORMAT_BC2_UNORM:
		return IMAGE_FORMAT_DXT3;
	case DXGI_FORMAT_BC2_UNORM_SRGB:
		return IMAGE_FORMAT_DXT3_SRGB;
	case DXGI_FORMAT_BC3_UNORM:
		return IMAGE_FORMAT_DXT5;
	case DXGI_FORMAT_BC3_UNORM_SRGB:
		return IMAGE_FORMAT_DXT5_SRGB;
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		return IMAGE_FORMAT_BGRA4444;
	case DXGI_FORMAT_B5G6R5_UNORM:
		return IMAGE_FORMAT_BGR565;
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return IMAGE_FORMAT_BGRX8888;
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return IMAGE_FORMAT_BGRX8888_SRGB;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return IMAGE_FORMAT_BGRA8888;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		return IMAGE_FORMAT_BGRA8888_SRGB;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return IMAGE_FORMAT_RGBA16161616F;
	case DXGI_FORMAT_R16G16B16A16_UNORM:
		return IMAGE_FORMAT_RGBA16161616;
	case DXGI_FORMAT_R32_FLOAT:
		return IMAGE_FORMAT_R32F;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return IMAGE_FORMAT_RGBA32323232F;

	default:
		return IMAGE_FORMAT_UNKNOWN;
	}
}

int CTextureDx11::CalcRamBytes() const
{
	int rWidth = m_nWidth;
	int rHeight = m_nHeight;
	int rDepth = m_Depth;
	int nRamBytes = 0;
	for ( int i = 0; i < m_NumLevels; i++ )
	{
		nRamBytes += ImageLoader::GetMemRequired( rWidth, rHeight, rDepth, m_Format, false );
		if ( rWidth == 1 && rHeight == 1 && rDepth == 1 )
		{
			break;
		}
		rDepth >>= 1;
		rWidth >>= 1;
		rHeight >>= 1;
		if ( rWidth < 1 )
		{
			rWidth = 1;
		}
		if ( rHeight < 1 )
		{
			rHeight = 1;
		}
		if ( rDepth < 1 )
		{
			rDepth = 1;
		}
	}
	return nRamBytes;
}

// Texture2D is used for regular 2D textures, cubemaps, and 2D texture arrays
// TODO: Support 1D and 3D textures?
ID3D11Resource *CTextureDx11::CreateD3DTexture( int width, int height, int nDepth,
						ImageFormat dstFormat, int numLevels, int nCreationFlags )
{
	if ( nDepth <= 0 )
		nDepth = 1;

	bool isCubeMap = ( nCreationFlags & TEXTURE_CREATE_CUBEMAP ) != 0;
	if ( isCubeMap )
		nDepth = 6;

	bool bIsRenderTarget = ( nCreationFlags & TEXTURE_CREATE_RENDERTARGET ) != 0;
	bool bManaged = ( nCreationFlags & TEXTURE_CREATE_MANAGED ) != 0;
	bool bIsDepthBuffer = ( nCreationFlags & TEXTURE_CREATE_DEPTHBUFFER ) != 0;
	bool isDynamic = ( nCreationFlags & TEXTURE_CREATE_DYNAMIC ) != 0;
	bool bAutoMipMap = ( nCreationFlags & TEXTURE_CREATE_AUTOMIPMAP ) != 0;
	bool bVertexTexture = ( nCreationFlags & TEXTURE_CREATE_VERTEXTEXTURE ) != 0;
	bool bAllowNonFilterable = ( nCreationFlags & TEXTURE_CREATE_UNFILTERABLE_OK ) != 0;
	bool bVolumeTexture = ( nDepth > 1 );
	bool bIsFallback = ( nCreationFlags & TEXTURE_CREATE_FALLBACK ) != 0;
	bool bNoD3DBits = ( nCreationFlags & TEXTURE_CREATE_NOD3DMEMORY ) != 0;

	// NOTE: This function shouldn't be used for creating depth buffers!
	//Assert( !bIsDepthBuffer );

	dstFormat = GetClosestSupportedImageFormatForD3D11( dstFormat );
	m_Format = dstFormat;
	DXGI_FORMAT d3dFormat = DXGI_FORMAT_UNKNOWN;
	d3dFormat = GetD3DFormat( dstFormat );

	if ( d3dFormat == DXGI_FORMAT_UNKNOWN )
	{
		Warning( "ShaderAPIDX11::CreateD3DTexture: Invalid image format %i!\n", (int)dstFormat );
		Assert( 0 );
		return 0;
	}

	m_D3DFormat = d3dFormat;

	D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
	if ( isDynamic )
	{
		usage = D3D11_USAGE_DYNAMIC;
	}

	UINT miscFlags = 0;
	if ( bAutoMipMap )
	{
		miscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}
	if ( isCubeMap )
	{
		miscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
	}

	UINT bindFlags = D3D11_BIND_SHADER_RESOURCE;
	if ( bIsRenderTarget )
	{
		bindFlags |= D3D11_BIND_RENDER_TARGET;
	}
	if ( bIsDepthBuffer )
	{
		bindFlags |= D3D11_BIND_DEPTH_STENCIL;
	}

	UINT cpuAccessFlags = 0;
	if ( isDynamic )
	{
		cpuAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}

	ID3D11Resource *pBaseTexture = NULL;
	HRESULT hr = S_OK;

	// TODO: Support 1D and 3D textures
	// (2D textures are good for regular textures, cubemaps, and texture arrays)
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory( &desc, sizeof( D3D11_TEXTURE2D_DESC ) );
	desc.ArraySize = nDepth;
	desc.Format = d3dFormat;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = numLevels;
	desc.MiscFlags = miscFlags;
	desc.BindFlags = bindFlags;
	desc.CPUAccessFlags = cpuAccessFlags;
	desc.Usage = usage;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	ID3D11Texture2D *pTex2D = NULL;
	hr = D3D11Device()->CreateTexture2D( &desc, NULL, &pTex2D );
	pBaseTexture = pTex2D;

	if ( FAILED( hr ) )
	{
		switch ( hr )
		{
		case E_OUTOFMEMORY:
			Warning( "TextureDx11::CreateD3DTexture: E_OUTOFMEMORY\n" );
			break;
		default:
			break;
		}
		return 0;
	}

	//s_TextureCount++;
	return pBaseTexture;
}

void DestroyD3DTexture( ID3D11Resource *pD3DTex )
{
	if ( pD3DTex )
	{
		int ref = pD3DTex->Release();
		Assert( ref == 0 );
		//s_TextureCount--;
	}
}

//-----------------------------------
// Dx11 implementation of a texture
//-----------------------------------

CTextureDx11::CTextureDx11()
{
	m_pLockedFace = NULL;
	m_nLockedFaceSize = 0;
	m_pLockedTexture = NULL;
	m_LockedSubresource = 0;
	m_bLocked = false;
	m_nFlags	 = 0;
	m_Count		 = 0;
	m_CountIndex	 = 0;
	m_nTimesBoundMax = 0;
	m_nTimesBoundThisFrame = 0;
	m_Anisotropy = 0;
	m_Format	       = IMAGE_FORMAT_RGBA8888;
	m_MinFilter = SHADER_TEXFILTERMODE_LINEAR;
	m_MagFilter = SHADER_TEXFILTERMODE_LINEAR;
	m_UTexWrap = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_VTexWrap = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_WTexWrap = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

	m_pTexture = NULL;
	m_ppTexture = NULL;

	m_pSamplerState = NULL;

	m_pView = NULL;
	m_ppView = NULL;
	m_pDepthStencilView = NULL;
	m_pRenderTargetView = NULL;

	m_pRamImage = NULL;
	m_ppRamImage = NULL;
}

inline int CalcMipLevels( int w, int h )
{
	return ceil( log2( max( w, h ) ) ) + 1;
}

void CTextureDx11::SetupTexture2D( int width, int height, int depth, int count, int i,
				   int flags, int numCopies, int numMipLevels, ImageFormat dstImageFormat )
{
	if ( dstImageFormat == IMAGE_FORMAT_I8 )
	{
		//DebuggerBreak();
	}
	//Log( "Making texture2D\n" );
	bool bIsRenderTarget = ( flags & TEXTURE_CREATE_RENDERTARGET ) != 0;
	bool bIsDepthBuffer = ( flags & TEXTURE_CREATE_DEPTHBUFFER ) != 0;

	if ( bIsDepthBuffer )
	{
		//Log( "Making depth buffer\n" );
		SetupDepthTexture( dstImageFormat, width, height, "depth", true );
		return;
	}
	else if ( bIsRenderTarget )
	{
		//Log( "Making render target\n" );
		SetupBackBuffer( width, height, "rendertarget", NULL, dstImageFormat );
		return;
	}

	//dstImageFormat = IMAGE_FORMAT_RGBA8888;

	//---------------------------------------------------------------------------------

	if ( depth == 0 )
		depth = 1;

	bool bIsCubeMap = ( flags & TEXTURE_CREATE_CUBEMAP ) != 0;
	if ( bIsCubeMap )
		depth = 6;
	bool bIsDynamic = ( flags & TEXTURE_CREATE_DYNAMIC ) != 0;
	bool bAutoMipMap = ( flags & TEXTURE_CREATE_AUTOMIPMAP ) != 0;
	bool bIsManaged = ( flags & TEXTURE_CREATE_MANAGED ) != 0;

	// Can't be both managed + dynamic. Dynamic is an optimization, but
	// if it's not managed, then we gotta do special client-specific stuff
	// So, managed wins out!
	if ( bIsManaged )
		bIsDynamic = false;

	if ( bAutoMipMap && numMipLevels == 0 && !bIsDynamic )
	{
		numMipLevels = CalcMipLevels( width, height );
	}
	else if ( bIsDynamic )
	{
		// Dynamic textures can't have mipmaps
		numMipLevels = 1;
	}

	m_iTextureType = TEXTURE_STANDARD;
	m_iTextureDimensions = TEXTURE_2D;

	unsigned short usSetFlags = 0;
	usSetFlags |= ( flags & TEXTURE_CREATE_VERTEXTEXTURE ) ? CTextureDx11::IS_VERTEX_TEXTURE : 0;

	m_nFlags = CTextureDx11::IS_ALLOCATED;
	m_nWidth = width;
	m_nHeight = height;
	m_Depth = depth;
	m_Count = count;
	m_CountIndex = i;
	m_CreationFlags = flags;
	m_nFlags |= usSetFlags;

	ID3D11Resource *pD3DTex;

	m_Format = GetClosestSupportedImageFormatForD3D11( dstImageFormat );
	int nRamBytes = CalcRamBytes();

	// Set the initial texture state
	if ( numCopies <= 1 )
	{
		m_NumCopies = 1;
		pD3DTex = CreateD3DTexture( width, height, depth, m_Format, numMipLevels, flags );
		SetTexture( pD3DTex );
		if ( bIsDynamic )
		{
			m_pRamImage = new unsigned char[nRamBytes];
			memset( m_pRamImage, 0, nRamBytes );
		}
	}
	else
	{
		m_NumCopies = numCopies;
		m_ppTexture = new ID3D11Resource * [numCopies];
		if ( bIsDynamic )
			m_ppRamImage = new unsigned char * [numCopies];
		for ( int k = 0; k < numCopies; k++ )
		{
			pD3DTex = CreateD3DTexture( width, height, depth, dstImageFormat, numMipLevels, flags );
			SetTexture( k, pD3DTex );
			if ( bIsDynamic )
			{
				m_ppRamImage[k] = new unsigned char[nRamBytes];
				memset( m_ppRamImage[k], 0, nRamBytes );
			}
		}
	}
	m_CurrentCopy = 0;

	if ( m_NumCopies == 1 )
		pD3DTex = m_pTexture;
	else
		pD3DTex = m_ppTexture[m_CurrentCopy];

	m_UTexWrap = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_VTexWrap = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_WTexWrap = D3D11_TEXTURE_ADDRESS_CLAMP;

	m_NumLevels = numMipLevels;
	m_Filter = ( numMipLevels != 1 ) ?
		D3D11_FILTER_MIN_MAG_MIP_LINEAR :
		D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;

	m_SwitchNeeded = false;
	
	AdjustSamplerState();
	MakeView();
}

void CTextureDx11::SetupDepthTexture( ImageFormat depthFormat, int width, int height, const char *pDebugName, bool bTexture )
{
	m_nFlags = CTextureDx11::IS_ALLOCATED;
	if ( bTexture )
		m_nFlags |= CTextureDx11::IS_DEPTH_STENCIL_TEXTURE;
	else
		m_nFlags |= CTextureDx11::IS_DEPTH_STENCIL;

	m_iTextureType = TEXTURE_DEPTHSTENCIL;
	m_iTextureDimensions = TEXTURE_3D;

	m_nWidth = width;
	m_nHeight = height;
	m_Depth = 1;
	m_Count = 1;
	m_CountIndex = 0;
	m_CreationFlags = TEXTURE_CREATE_DEPTHBUFFER;
	m_Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	m_NumLevels = 1;
	m_NumCopies = 1;
	m_CurrentCopy = 0;

	m_pTexture = CreateD3DTexture(
		width, height, m_Depth, depthFormat, m_NumLevels, m_CreationFlags );
	AdjustSamplerState();
	MakeDepthStencilView();	
	if ( bTexture )
	{
		MakeView();
	}
}

void CTextureDx11::SetupBackBuffer( int width, int height, const char *pDebugName,
				    ID3D11Texture2D *pBackBuffer, ImageFormat format )
{
	m_nFlags = CTextureDx11::IS_ALLOCATED;

	m_iTextureType = TEXTURE_RENDERTARGET;
	m_iTextureDimensions = TEXTURE_2D;

	m_nWidth = width;
	m_nHeight = height;
	m_Depth = 1;
	m_Count = 1;
	m_CountIndex = 0;
	m_CreationFlags = TEXTURE_CREATE_RENDERTARGET;
	m_Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	m_NumLevels = 1;
	m_NumCopies = 1;
	m_CurrentCopy = 0;
	m_Format = GetClosestSupportedImageFormatForD3D11( format );
	m_D3DFormat = GetD3DFormat( m_Format );

	if ( pBackBuffer )
		m_pTexture = pBackBuffer;
	else
		m_pTexture = CreateD3DTexture( width, height, m_Depth, format, m_NumLevels, m_CreationFlags );

	AdjustSamplerState();
	MakeRenderTargetView();
	MakeView();
}

void CTextureDx11::SetMinFilter( ShaderTexFilterMode_t texFilterMode )
{
	m_MinFilter = texFilterMode;
	AdjustD3DFilter();
}

void CTextureDx11::SetMagFilter( ShaderTexFilterMode_t texFilterMode )
{
	m_MagFilter = texFilterMode;
	AdjustD3DFilter();
}

void CTextureDx11::SetWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode )
{
	D3D11_TEXTURE_ADDRESS_MODE address;
	switch ( wrapMode )
	{
	case SHADER_TEXWRAPMODE_CLAMP:
		address = D3D11_TEXTURE_ADDRESS_CLAMP;
		break;
	case SHADER_TEXWRAPMODE_REPEAT:
		address = D3D11_TEXTURE_ADDRESS_WRAP;
		break;
	case SHADER_TEXWRAPMODE_BORDER:
		address = D3D11_TEXTURE_ADDRESS_BORDER;
		break;
	default:
		address = D3D11_TEXTURE_ADDRESS_CLAMP;
		Warning( "CTextureDx11::SetWrap: unknown wrapMode %i\n", wrapMode );
		break;
	}

	switch ( coord )
	{
	case SHADER_TEXCOORD_S:
		m_UTexWrap = address;
		break;
	case SHADER_TEXCOORD_T:
		m_VTexWrap = address;
		break;
	case SHADER_TEXCOORD_U:
		m_WTexWrap = address;
		break;
	default:
		Warning( "CTextureDx11::SetWrap: unknown coord %i\n", coord );
		break;
	}

	AdjustSamplerState();
}

void CTextureDx11::AdjustD3DFilter()
{
	// Determines the D3D11 filter from the specified combination
	// of min and mag filter on the texture.

	// Non-mip combinations
	if ( m_MinFilter == SHADER_TEXFILTERMODE_NEAREST &&
	     m_MagFilter == SHADER_TEXFILTERMODE_NEAREST )
	{
		m_Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	}

	else if ( m_MinFilter == SHADER_TEXFILTERMODE_LINEAR &&
		  m_MagFilter == SHADER_TEXFILTERMODE_LINEAR )
	{
		m_Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	}

	else if ( m_MinFilter == SHADER_TEXFILTERMODE_NEAREST &&
		  m_MagFilter == SHADER_TEXFILTERMODE_LINEAR )
	{
		m_Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
	}

	else if ( m_MinFilter == SHADER_TEXFILTERMODE_LINEAR &&
		  m_MagFilter == SHADER_TEXFILTERMODE_NEAREST )
	{
		m_Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
	}

	// Linear_Mipmap combinations

	else if ( m_MinFilter == SHADER_TEXFILTERMODE_LINEAR_MIPMAP_LINEAR &&
		  m_MagFilter == SHADER_TEXFILTERMODE_LINEAR )
	{
		m_Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	}

	else if ( m_MinFilter == SHADER_TEXFILTERMODE_LINEAR_MIPMAP_LINEAR &&
		  m_MagFilter == SHADER_TEXFILTERMODE_NEAREST )
	{
		m_Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
	}

	else if ( m_MinFilter == SHADER_TEXFILTERMODE_LINEAR_MIPMAP_NEAREST &&
		  m_MagFilter == SHADER_TEXFILTERMODE_LINEAR )
	{
		m_Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	}

	else if ( m_MinFilter == SHADER_TEXFILTERMODE_LINEAR_MIPMAP_NEAREST &&
		  m_MagFilter == SHADER_TEXFILTERMODE_NEAREST )
	{
		m_Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
	}

	// Nearest_Mipmap combinations

	else if ( m_MinFilter == SHADER_TEXFILTERMODE_NEAREST_MIPMAP_NEAREST &&
		  m_MagFilter == SHADER_TEXFILTERMODE_LINEAR )
	{
		m_Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	}

	else if ( m_MinFilter == SHADER_TEXFILTERMODE_NEAREST_MIPMAP_NEAREST &&
		  m_MagFilter == SHADER_TEXFILTERMODE_NEAREST )
	{
		m_Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	}

	else if ( m_MinFilter == SHADER_TEXFILTERMODE_NEAREST_MIPMAP_LINEAR &&
		  m_MagFilter == SHADER_TEXFILTERMODE_LINEAR )
	{
		m_Filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
	}

	else if ( m_MinFilter == SHADER_TEXFILTERMODE_NEAREST_MIPMAP_LINEAR &&
		  m_MagFilter == SHADER_TEXFILTERMODE_NEAREST )
	{
		m_Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	}

	else
	{
		Warning( "CTextureDx11::AdjustD3DFilter: Invalid combination of min and mag filter. Min: %i, Mag: %i\n",
			 m_MinFilter, m_MagFilter );
	}

	AdjustSamplerState();
}

void CTextureDx11::SetAnisotropicLevel( int level )
{
	m_Anisotropy = level;
	if ( level > 1 )
		m_Filter = D3D11_FILTER_ANISOTROPIC;
	AdjustSamplerState();
}

void CTextureDx11::AdjustSamplerState()
{
	if ( m_pSamplerState )
		m_pSamplerState->Release();

	m_pSamplerState = NULL;
	D3D11_SAMPLER_DESC desc;
	ZeroMemory( &desc, sizeof( D3D11_SAMPLER_DESC ) );
	desc.AddressU = m_UTexWrap;
	desc.AddressV = m_VTexWrap;
	desc.AddressW = m_WTexWrap;
	desc.Filter = m_Filter;
	desc.MaxAnisotropy = m_Anisotropy;
	HRESULT hr = D3D11Device()->CreateSamplerState( &desc, &m_pSamplerState );
	if ( FAILED( hr ) )
	{
		Warning( "CTextureDx11: Couldn't create sampler state!\n" );
	}
}

void CTextureDx11::MakeRenderTargetView()
{
	if ( m_iTextureType != TEXTURE_RENDERTARGET )
	{
		return;
	}

	D3D11_RENDER_TARGET_VIEW_DESC desc;
	ZeroMemory( &desc, sizeof( D3D11_RENDER_TARGET_VIEW_DESC ) );
	desc.Format = m_D3DFormat;
	desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	if ( m_pRenderTargetView )
	{
		int ref = m_pRenderTargetView->Release();
		Assert( ref == 0 );
	}
	HRESULT hr = D3D11Device()->CreateRenderTargetView( m_pTexture, &desc, &m_pRenderTargetView );
	if ( FAILED( hr ) )
	{
		Warning( "Failed to make D3D render target view!\n" );
	}
}

void CTextureDx11::MakeDepthStencilView()
{
	if ( m_iTextureType != TEXTURE_DEPTHSTENCIL )
	{
		return;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC desc;
	ZeroMemory( &desc, sizeof( D3D11_DEPTH_STENCIL_VIEW_DESC ) );
	desc.Format = GetDepthStencilViewFormat( m_D3DFormat );
	desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	desc.Flags = 0;

	if ( m_pDepthStencilView )
	{
		int ref = m_pDepthStencilView->Release();
		Assert( ref == 0 );
	}
	HRESULT hr = D3D11Device()->CreateDepthStencilView( m_pTexture, &desc, &m_pDepthStencilView );
	if ( FAILED( hr ) )
	{
		Warning( "Failed to make D3D depth stencil view!\n" );
	}
}

void CTextureDx11::MakeView()
{
	bool bGenMipMap = ( m_CreationFlags & TEXTURE_CREATE_AUTOMIPMAP ) != 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	ZeroMemory( &desc, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );
		
	if ( m_iTextureType == TEXTURE_DEPTHSTENCIL )
	{
		desc.Format = GetDepthSRVFormat( m_D3DFormat );
	}
	else
	{
		desc.Format = m_D3DFormat;
	}
	
	if ( m_Depth == 6 && ( ( m_CreationFlags & TEXTURE_CREATE_CUBEMAP ) != 0 ) )
	{
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		desc.TextureCube.MipLevels = m_NumLevels;
		desc.TextureCube.MostDetailedMip = 0;
	}
	else if ( m_Depth > 1 )
	{
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipLevels = m_NumLevels;
		desc.Texture2DArray.MostDetailedMip = 0;
		desc.Texture2DArray.ArraySize = m_Depth;
		desc.Texture2DArray.FirstArraySlice = 0;
	}
	else
	{
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipLevels = m_NumLevels;
		desc.Texture2D.MostDetailedMip = 0;
	}

	if ( m_NumCopies > 1 )
	{
		if ( m_ppView )
		{
			delete[] m_ppView;
		}
		m_ppView = new ID3D11ShaderResourceView * [m_NumCopies];
		for ( int copy = 0; copy < m_NumCopies; copy++ )
		{
			HRESULT hr = D3D11Device()->CreateShaderResourceView( m_ppTexture[copy], &desc, &m_ppView[copy] );
			if ( FAILED( hr ) )
			{
				Warning( "Unable to create shader resource view for texture copy %i!\n", copy );
			}
		}

	}
	else
	{
		if ( m_pView )
		{
			m_pView->Release();
		}
		m_pView = NULL;

		HRESULT hr = D3D11Device()->CreateShaderResourceView( m_pTexture, &desc, &m_pView );
		if ( FAILED( hr ) )
		{
			Warning( "Unable to create D3D11 Texture view!\n" );
		}
	}
	
}

static int s_CubemapBlits = 0;

void CTextureDx11::BlitSurfaceBits( CTextureDx11::TextureLoadInfo_t &info, int xOffset, int yOffset, int srcStride )
{
	if ( m_CreationFlags & TEXTURE_CREATE_DYNAMIC )
	{
		// Dynamic textures need to be updated using Lock() and Unlock()
		Warning( "TextureDX11: Tried to call BlitSurfaceBits() on a dynamic texture!\n" );
		return;
	}

	CD3D11_BOX box;
	box.left = xOffset;
	box.right = xOffset + info.m_nWidth;
	box.top = yOffset;
	box.bottom = yOffset + info.m_nHeight;
	box.front = info.m_nZOffset;
	box.back = info.m_nZOffset + 1;

	int mem = ImageLoader::GetMemRequired( info.m_nWidth, info.m_nHeight, info.m_nZOffset, m_Format, false );
	unsigned char *pNewImage = new unsigned char[mem];
	int dstStride = ImageLoader::SizeInBytes( m_Format );
	int pitch = dstStride * info.m_nWidth;
	if ( m_Format == IMAGE_FORMAT_DXT3 || m_Format == IMAGE_FORMAT_DXT3_SRGB ||
	     m_Format == IMAGE_FORMAT_DXT5 || m_Format == IMAGE_FORMAT_DXT5_SRGB )
	{
		pitch = 16 * ( info.m_nWidth / 4 );
	}
	else if ( m_Format == IMAGE_FORMAT_DXT1 || m_Format == IMAGE_FORMAT_DXT1_SRGB ||
		  m_Format == IMAGE_FORMAT_DXT1_ONEBITALPHA || m_Format == IMAGE_FORMAT_DXT1_ONEBITALPHA_SRGB )
	{
		pitch = 8 * ( info.m_nWidth / 4 );
	}

	bool ret =
		ShaderUtil()->ConvertImageFormat( info.m_pSrcData, info.m_SrcFormat, pNewImage, m_Format,
						  info.m_nWidth, info.m_nHeight, srcStride );
	if ( !ret )
	{
		Warning( "Couldn't convert texture for uploading to D3D!\n" );
		return;
	}

#if 0
	if ( m_CreationFlags & TEXTURE_CREATE_CUBEMAP )
	{
		char cmfile[100];
		sprintf( cmfile, "C:\\Users\\Brian\\Desktop\\SourceDX11Debug\\cubemap-%i-face-%i.tga", s_CubemapBlits, info.m_CubeFaceID );
		TGAWriter::WriteTGAFile( cmfile, info.m_nWidth, info.m_nHeight, m_Format, pNewImage, dstStride * info.m_nWidth );

		if ( info.m_CubeFaceID == 4 )
		{
			s_CubemapBlits++;
		}
	}
#endif
	
	UINT subresource = D3D11CalcSubresource( info.m_nLevel, info.m_CubeFaceID, m_NumLevels );

	D3D11DeviceContext()->UpdateSubresource( info.m_pTexture, subresource, &box, pNewImage,
						 pitch, 0 );

	delete[] pNewImage;

	// If the texture was created with auto mipmap and we have just filled in the base mip,
	// make D3D generate the remaining mips. If the texture was not created with auto mipmap,
	// or we have just filled in a non-base mip, we assume the user is filling in the mips.
	if ( ( ( m_CreationFlags & TEXTURE_CREATE_AUTOMIPMAP ) != 0 ) && m_NumLevels > 1 && info.m_nLevel == 0 )
	{
		D3D11DeviceContext()->GenerateMips( info.m_pView );
	}
}

void CTextureDx11::BlitTextureBits( CTextureDx11::TextureLoadInfo_t &info, int xOffset, int yOffset, int srcStride )
{
	// TODO: Volume texture?

	//Assert( info.m_nZOffset == 0 );
	if ( info.m_nZOffset != 0 )
	{
		return;
	}
	BlitSurfaceBits( info, xOffset, yOffset, srcStride );
}

void CTextureDx11::LoadTexImage( CTextureDx11::TextureLoadInfo_t &info, int xOffset, int yOffset, int srcStride )
{
	MEM_ALLOC_CREDIT();

	Assert( info.m_pSrcData );
	Assert( info.m_pTexture );

	// Copy in the bits...
	BlitTextureBits( info, xOffset, yOffset, srcStride );
}

void CTextureDx11::Delete()
{
	int nDeallocated = 0;

	if ( m_NumCopies == 1 )
	{
		if ( m_pView )
		{
			int refCount = m_pView->Release();
			Assert( refCount == 0 );
			m_pView = 0;
			nDeallocated++;
		}

		if ( m_pTexture )
		{
			DestroyD3DTexture( m_pTexture );
			m_pTexture = 0;
			nDeallocated = 1;
		}

		if ( m_pRamImage )
		{
			delete[] m_pRamImage;
			m_pRamImage = NULL;
		}
	}
	else
	{
		if ( m_ppView )
		{
			for ( int i = 0; i < m_NumCopies; i++ )
			{
				if ( m_ppView[i] )
				{
					int refCount = m_ppView[i]->Release();
					Assert( refCount == 0 );
					nDeallocated++;
				}
			}

			delete[] m_ppView;
			m_ppView = NULL;
		}

		if ( m_ppTexture )
		{
			// Multiple copy texture
			for ( int j = 0; j < m_NumCopies; j++ )
			{
				if ( m_ppTexture[j] )
				{
					DestroyD3DTexture( m_ppTexture[j] );
					m_ppTexture[j] = 0;
					++nDeallocated;
				}
			}
			delete[] m_ppTexture;
			m_ppTexture = NULL;
		}

		if ( m_ppRamImage )
		{
			delete[] m_ppRamImage;
			m_ppRamImage = NULL;
		}
	}

	if ( m_iTextureType == TEXTURE_RENDERTARGET )
	{
		if ( m_pRenderTargetView )
		{
			int refCount = m_pRenderTargetView->Release();
			Assert( refCount == 0 );
			m_pRenderTargetView = 0;
			nDeallocated++;
		}
	}
	else if ( m_iTextureType == TEXTURE_DEPTHSTENCIL )
	{
		if ( m_pDepthStencilView )
		{
			int refCount = m_pDepthStencilView->Release();
			Assert( refCount == 0 );
			m_pDepthStencilView = 0;
			nDeallocated++;
		}
	}	

	if ( m_pSamplerState )
	{
		m_pSamplerState->Release();
		m_pSamplerState = 0;
		nDeallocated++;
	}

	m_NumCopies = 0;
	m_nFlags = 0;
}

bool CTextureDx11::Lock( int copy, int level, int cubeFaceID, int xOffset, int yOffset,
			 int width, int height, bool bDiscard, CPixelWriter &writer )
{
	Assert( !m_bLocked );
	Assert( level == 0 ); // dynamic textures can't have mipmaps

	if ( m_bLocked )
	{
		return false;
	}	

	if ( m_NumCopies == 1 )
	{
		m_pLockedTexture = m_pTexture;
		m_pLockedFace = m_pRamImage;
	}
	else
	{
		m_pLockedTexture = m_ppTexture[copy];
		m_pLockedFace = m_ppRamImage[copy];
	}

	int bytesPerXel = ImageLoader::SizeInBytes( m_Format );
	int bytesPerRow = bytesPerXel * m_nWidth;
	int bytesPerFace = bytesPerXel * m_nWidth * m_nHeight;
	// Where do we want to start modifying?
	int nMemOffset = bytesPerFace * cubeFaceID;
	nMemOffset += yOffset * bytesPerRow;
	nMemOffset += xOffset * bytesPerXel;
	// Offset into the ram image
	unsigned char *pModifyBegin = m_pLockedFace + nMemOffset;

	m_nLockedFaceSize = bytesPerFace;

	m_LockedSubresource = D3D11CalcSubresource( 0, cubeFaceID, 1 );

	ZeroMemory( &m_MappedData, sizeof( D3D11_MAPPED_SUBRESOURCE ) );
	HRESULT hr = D3D11DeviceContext()->Map( m_pLockedTexture, m_LockedSubresource, D3D11_MAP_WRITE_DISCARD, 0, &m_MappedData );
	if ( FAILED( hr ) )
	{
		return false;
	}

	writer.SetPixelMemory( m_Format, pModifyBegin, bytesPerRow );

	m_bLocked = true;
	return true;
}

void CTextureDx11::Unlock( int copy, int level, int cubeFaceID )
{
	Assert( m_bLocked );
	if ( !m_bLocked )
	{
		return;
	}

	memcpy( m_MappedData.pData, m_pLockedFace, m_nLockedFaceSize );
	D3D11DeviceContext()->Unmap( m_pLockedTexture, m_LockedSubresource );

	m_pLockedFace = NULL;
	m_nLockedFaceSize = 0;
	m_pLockedTexture = NULL;
	m_LockedSubresource = 0;

	m_bLocked = false;
}