#include "TextureDx11.h"
#include "shaderapi/ishaderapi.h"
#include "shaderdevicedx11.h"
#include "shaderapidx11_global.h"
#include "shaderapi/ishaderutil.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

// We will have to convert srcFormat into what this function
// returns for use in D3D11.
ImageFormat GetClosestSupportedImageFormatForD3D11( ImageFormat srcFormat )
{
	switch ( srcFormat )
	{
	case IMAGE_FORMAT_BGR888:
		return IMAGE_FORMAT_BGRA8888;
	case IMAGE_FORMAT_RGB888:
		return IMAGE_FORMAT_RGBA8888;
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
	case IMAGE_FORMAT_NV_DST24:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;

	// These ones match D3D.
	case IMAGE_FORMAT_A8:
		return DXGI_FORMAT_A8_UNORM;
	case IMAGE_FORMAT_DXT1:
	case IMAGE_FORMAT_DXT1_ONEBITALPHA:
		return DXGI_FORMAT_BC1_UNORM;
	case IMAGE_FORMAT_DXT3:
		return DXGI_FORMAT_BC2_UNORM;
	case IMAGE_FORMAT_DXT5:
		return DXGI_FORMAT_BC3_UNORM;
	case IMAGE_FORMAT_BGRA4444:
		return DXGI_FORMAT_B4G4R4A4_UNORM;
	case IMAGE_FORMAT_BGRX5551:
		return DXGI_FORMAT_B5G5R5A1_UNORM;
	case IMAGE_FORMAT_BGR565:
		return DXGI_FORMAT_B5G6R5_UNORM;
	case IMAGE_FORMAT_BGRX8888:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	case IMAGE_FORMAT_BGRA8888:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
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

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

ImageFormat GetImageFormat( DXGI_FORMAT d3dFormat )
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
	//case DXGI_FORMAT_R8G8_UNORM:
	//	return IMAGE_FORMAT_UV88;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return IMAGE_FORMAT_UVWQ8888;

	case DXGI_FORMAT_A8_UNORM:
		return IMAGE_FORMAT_A8;
	case DXGI_FORMAT_BC1_UNORM:
		return IMAGE_FORMAT_DXT1_ONEBITALPHA;
	case DXGI_FORMAT_BC2_UNORM:
		return IMAGE_FORMAT_DXT3;
	case DXGI_FORMAT_BC3_UNORM:
		return IMAGE_FORMAT_DXT5;
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		return IMAGE_FORMAT_BGRA4444;
	case DXGI_FORMAT_B5G6R5_UNORM:
		return IMAGE_FORMAT_BGR565;
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return IMAGE_FORMAT_BGRX8888;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return IMAGE_FORMAT_BGRA8888;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return IMAGE_FORMAT_RGBA16161616F;
	case DXGI_FORMAT_R16G16B16A16_UNORM:
		return IMAGE_FORMAT_RGBA16161616;
	case DXGI_FORMAT_R32_FLOAT:
		return IMAGE_FORMAT_R32F;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return IMAGE_FORMAT_RGBA32323232F;
	}
}

// Texture2D is used for regular 2D textures, cubemaps, and 2D texture arrays
// TODO: Support 1D and 3D textures?
ID3D11Resource *CreateD3DTexture( int width, int height, int nDepth,
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
	DXGI_FORMAT d3dFormat = DXGI_FORMAT_UNKNOWN;
	d3dFormat = GetD3DFormat( dstFormat );

	if ( d3dFormat == DXGI_FORMAT_UNKNOWN )
	{
		Warning( "ShaderAPIDX11::CreateD3DTexture: Invalid image format %i!\n", (int)dstFormat );
		Assert( 0 );
		return 0;
	}

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
		bindFlags = D3D11_BIND_RENDER_TARGET;
	}
	if ( bIsDepthBuffer )
	{
		bindFlags = D3D11_BIND_DEPTH_STENCIL;
	}

	UINT cpuAccessFlags = 0;
	if ( !bIsDepthBuffer && !bIsRenderTarget )
	{
		cpuAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}

	ID3D11Resource *pBaseTexture = NULL;
	HRESULT hr = S_OK;

	// TODO: Support 1D and 3D textures
	// (2D textures are good for regular textures, cubemaps, and texture arrays)
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory( &desc, sizeof( D3D11_TEXTURE3D_DESC ) );
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
}

inline int CalcMipLevels( int w, int h )
{
	return ceil( log2( max( w, h ) ) ) + 1;
}

void CTextureDx11::SetupTexture2D( int width, int height, int depth, int count, int i,
				   int flags, int numCopies, int numMipLevels, ImageFormat dstImageFormat )
{
	bool bIsRenderTarget = ( flags & TEXTURE_CREATE_RENDERTARGET ) != 0;
	bool bIsDepthBuffer = ( flags & TEXTURE_CREATE_DEPTHBUFFER ) != 0;

	if ( bIsDepthBuffer )
	{
		SetupDepthTexture( dstImageFormat, width, height, "depth", true );
		return;
	}
	else if ( bIsRenderTarget )
	{
		SetupBackBuffer( width, height, "rendertarget", NULL, dstImageFormat );
		return;
	}

	//---------------------------------------------------------------------------------

	if ( depth == 0 )
		depth == 1;

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

	if ( bAutoMipMap && numMipLevels == 0 )
	{
		numMipLevels = CalcMipLevels( width, height );
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

	// Set the initial texture state
	if ( numCopies <= 1 )
	{
		m_NumCopies = 1;
		pD3DTex = CreateD3DTexture( width, height, depth, dstImageFormat, numMipLevels, flags );
		SetTexture( pD3DTex );
	}
	else
	{
		m_NumCopies = numCopies;
		m_ppTexture = new ID3D11Resource * [numCopies];
		for ( int k = 0; k < numCopies; k++ )
		{
			pD3DTex = CreateD3DTexture( width, height, depth, dstImageFormat, numMipLevels, flags );
			SetTexture( k, pD3DTex );
		}
	}
	m_CurrentCopy = 0;

	if ( m_NumCopies == 1 )
		pD3DTex = m_pTexture;
	else
		pD3DTex = m_ppTexture[m_CurrentCopy];

	m_Format = dstImageFormat;
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

void CTextureDx11::SetupDepthTexture( ImageFormat renderFormat, int width, int height, const char *pDebugName, bool bTexture )
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
		width, height, m_Depth, renderFormat, m_NumLevels, m_CreationFlags );
	AdjustSamplerState();
	MakeDepthStencilView();		
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

	if ( pBackBuffer )
		m_pTexture = pBackBuffer;
	else
		m_pTexture = CreateD3DTexture( width, height, m_Depth, format, m_NumLevels, m_CreationFlags );

	AdjustSamplerState();
	MakeRenderTargetView();
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

	if ( m_pRenderTargetView )
	{
		int ref = m_pRenderTargetView->Release();
		Assert( ref == 0 );
	}
	HRESULT hr = D3D11Device()->CreateRenderTargetView( m_pTexture, NULL, &m_pRenderTargetView );
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

	if ( m_pDepthStencilView )
	{
		int ref = m_pDepthStencilView->Release();
		Assert( ref == 0 );
	}
	HRESULT hr = D3D11Device()->CreateDepthStencilView( m_pTexture, NULL, &m_pDepthStencilView );
	if ( FAILED( hr ) )
	{
		Warning( "Failed to make D3D depth stencil view!\n" );
	}
}

void CTextureDx11::MakeView()
{
	if ( m_iTextureType != TEXTURE_STANDARD )
	{
		return;
	}

	bool bGenMipMap = ( m_CreationFlags & TEXTURE_CREATE_AUTOMIPMAP ) != 0;

	if ( m_NumCopies > 1 )
	{
		if ( m_ppView )
		{
			delete[] m_ppView;
		}
		m_ppView = new ID3D11ShaderResourceView * [m_NumCopies];
		for ( int copy = 0; copy < m_NumCopies; copy++ )
		{
			HRESULT hr = D3D11Device()->CreateShaderResourceView( m_ppTexture[copy], NULL, &m_ppView[copy] );
			if ( FAILED( hr ) )
			{
				Warning( "Unable to create shader resource view for texture copy %i!\n", copy );
			}
			if ( bGenMipMap && m_ppView[copy] )
			{
				D3D11DeviceContext()->GenerateMips( m_ppView[copy] );
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

		HRESULT hr = D3D11Device()->CreateShaderResourceView( m_pTexture, NULL, &m_pView );
		if ( FAILED( hr ) )
		{
			Warning( "Unable to create D3D11 Texture view!\n" );
		}
		if ( bGenMipMap && m_pView )
		{
			D3D11DeviceContext()->GenerateMips( m_pView );
		}
	}
	
}

void CTextureDx11::BlitSurfaceBits( CTextureDx11::TextureLoadInfo_t &info, int xOffset, int yOffset, int srcStride )
{
	CD3D11_BOX box;
	box.left = xOffset;
	box.right = xOffset + info.m_nWidth;
	box.top = yOffset;
	box.bottom = yOffset + info.m_nHeight;
	box.front = 0;
	box.back = 1;

	int mem = ImageLoader::GetMemRequired( info.m_nWidth, info.m_nHeight, info.m_nZOffset, m_Format, false );
	unsigned char *pNewImage = new unsigned char[mem];
	int dstStride = ImageLoader::SizeInBytes( m_Format );

	bool ret = 
		ShaderUtil()->ConvertImageFormat( info.m_pSrcData, info.m_SrcFormat, pNewImage, m_Format,
						  info.m_nWidth, info.m_nHeight, srcStride, dstStride );
	if ( !ret )
	{
		Warning( "Couldn't convert texture for uploading to D3D!\n" );
		return;
	}

	UINT subresource = D3D11CalcSubresource( info.m_nLevel, info.m_CubeFaceID, m_NumLevels );

	D3D11DeviceContext()->UpdateSubresource( info.m_pTexture, subresource, &box, pNewImage, dstStride * info.m_nWidth, dstStride * info.m_nWidth * info.m_nHeight );

	delete[] pNewImage;

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

void CTextureDx11::LoadTexImage( CTextureDx11::TextureLoadInfo_t &info )
{
	MEM_ALLOC_CREDIT();

	Assert( info.m_pSrcData );
	Assert( info.m_pTexture );

	// Copy in the bits...
	BlitTextureBits( info, 0, 0, 0 );
}

void CTextureDx11::Delete()
{
	int nDeallocated = 0;

	if ( m_NumCopies == 1 )
	{
		if ( m_pTexture )
		{
			DestroyD3DTexture( m_pTexture );
			m_pTexture = 0;
			nDeallocated = 1;
		}
	}
	else
	{
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
		}			
		m_ppTexture = 0;
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
	else
	{
		if ( m_NumCopies > 1 )
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
			}

			m_ppView = NULL;
		}
		else
		{
			if ( m_pView )
			{
				int refCount = m_pView->Release();
				Assert( refCount == 0 );
				m_pView = 0;
				nDeallocated++;
			}
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