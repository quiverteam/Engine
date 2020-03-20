#pragma once

#include "platform.h"
#include "bitmap/imageformat.h"
#include "Dx11Global.h"
#include "shaderapi/shareddefs.h"
#include "tier0/dbg.h"
#include "shaderapi/ishaderapi.h"
#include <pixelwriter.h>
#include <utlvector.h>
#include <utlmap.h>
#include <utlbuffer.h>

class CTextureDx11
{
public:
	//-----------------------------------------------------------------------------
	// Info for texture loading
	//-----------------------------------------------------------------------------
	struct TextureLoadInfo_t
	{
		ShaderAPITextureHandle_t	m_TextureHandle;
		int							m_nCopy;
		ID3D11Resource *m_pTexture;
		ID3D11ShaderResourceView *m_pView;
		int							m_nLevel;
		D3D11_TEXTURECUBE_FACE			m_CubeFaceID;
		int							m_nWidth;
		int							m_nHeight;
		int							m_nZOffset;				// What z-slice of the volume texture are we loading?
		ImageFormat					m_SrcFormat;
		unsigned char *m_pSrcData;
	};

	enum
	{
		TEXTURE_1D = 1,
		TEXTURE_2D,
		TEXTURE_3D,
	};

	enum
	{
		TEXTURE_STANDARD,
		TEXTURE_DEPTHSTENCIL,
		TEXTURE_RENDERTARGET,
	};

	enum
	{
		IS_ALLOCATED		 = 1 << 0,
		IS_DEPTH_STENCIL	 = 1 << 1,
		IS_DEPTH_STENCIL_TEXTURE = 1 << 2,
		IS_RENDERABLE		 = ( IS_DEPTH_STENCIL | IS_ALLOCATED ),
		IS_VERTEX_TEXTURE	 = 1 << 3,
	};

	CTextureDx11();
	ID3D11Resource *GetTexture() const;
	ID3D11Resource *GetTexture( int n ) const;
	ID3D11ShaderResourceView *GetView( int n ) const;
	ID3D11ShaderResourceView *GetView() const;
	ID3D11DepthStencilView *GetDepthStencilView() const;
	ID3D11RenderTargetView *GetRenderTargetView() const;
	ID3D11Texture1D *GetTexture1D() const;
	ID3D11Texture1D *GetTexture1D( int n ) const;
	ID3D11Texture2D *GetTexture2D() const;
	ID3D11Texture2D *GetTexture2D( int n ) const;
	ID3D11Texture3D *GetTexture3D() const;
	ID3D11Texture3D *GetTexture3D( int n ) const;
	ID3D11SamplerState *GetSamplerState() const;

	void Delete();

	void AdjustSamplerState();
	void MakeView();
	void MakeRenderTargetView();
	void MakeDepthStencilView();

	void SetAnisotropicLevel( int level );
	void SetMinFilter( ShaderTexFilterMode_t texFilterMode );
	void SetMagFilter( ShaderTexFilterMode_t texFilterMode );
	void SetWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode );

	void AdjustD3DFilter();

	void SetupTexture2D( int width, int height, int depth, int count, int i,
			     int flags, int numCopies,
			     int numMipLevels, ImageFormat dstImageFormat );
	void SetupDepthTexture( ImageFormat renderFormat, int width, int height,
				const char *pDebugName, bool bTexture );
	void SetupBackBuffer( int width, int height, const char *pDebugName,
			      ID3D11Texture2D *pBackBuffer = NULL, ImageFormat format = IMAGE_FORMAT_RGBA8888 );

	bool Is1D() const;
	bool Is2D() const;
	bool Is3D() const;

	bool Lock( int copy, int level, int cubeFaceID, int xOffset, int yOffset,
		   int width, int height, bool bDiscard, CPixelWriter &writer );
	void Unlock( int copy, int level, int cubeFaceID );

	void LoadTexImage( TextureLoadInfo_t &info, int xOffset = 0, int yOffset = 0, int srcStride = 0 );
	void BlitTextureBits( TextureLoadInfo_t &info, int xOffset, int yOffset, int srcStride );
	void BlitSurfaceBits( TextureLoadInfo_t &info, int xOffset, int yOffset, int srcStride );

	void SetTexture( ID3D11Resource *tex );
	void SetTexture( int n, ID3D11Resource *tex );
	//void SetView( ID3D11ShaderResourceView *view );

	int CalcRamBytes() const;

	static ImageFormat GetImageFormat( DXGI_FORMAT d3dFormat );

	static ImageFormat GetClosestSupportedImageFormatForD3D11( ImageFormat srcFormat );

private:
	ID3D11Resource *CreateD3DTexture( int width, int height, int nDepth,
					  ImageFormat dstFormat, int numLevels, int nCreationFlags );

private:
	// Can either have a single texture or an array of texture copies
	union
	{
		ID3D11Resource *m_pTexture;
		ID3D11Resource **m_ppTexture;
	};

	// One sampler state
	ID3D11SamplerState *m_pSamplerState;

	// Can either have a single view or array of views (for copies)
	union
	{
		ID3D11ShaderResourceView *m_pView;
		ID3D11ShaderResourceView **m_ppView;
	};

	// Can either have a depth stencil view (for depth buffers) or
	// a render target view (for render targets/back buffer)
	union
	{
		ID3D11DepthStencilView *m_pDepthStencilView;
		ID3D11RenderTargetView *m_pRenderTargetView;
	};

	D3D11_TEXTURE_ADDRESS_MODE m_UTexWrap;
	D3D11_TEXTURE_ADDRESS_MODE m_VTexWrap;
	D3D11_TEXTURE_ADDRESS_MODE m_WTexWrap;
	D3D11_FILTER m_Filter;
	D3D11_FILTER_TYPE m_MinFilter;
	D3D11_FILTER_TYPE m_MipFilter;
	D3D11_FILTER_TYPE m_MagFilter;
	int m_Anisotropy;
	bool m_bIsAnisotropic;

public:
	unsigned char m_NumLevels;
	unsigned char m_SwitchNeeded;
	unsigned char m_NumCopies;
	unsigned char m_CurrentCopy;
	short m_Count;
	short m_CountIndex;
	short m_Depth;
	int m_iTextureType;
	int m_iTextureDimensions;
	int m_nWidth;
	int m_nHeight;
	
	ImageFormat m_Format;
	DXGI_FORMAT m_D3DFormat;
	int m_nSizeTexels;
	int m_nSizeBytes;
	int m_LastBoundFrame;
	int m_nTimesBoundMax;
	int m_nTimesBoundThisFrame;
	unsigned short m_nFlags;
	int m_CreationFlags;

	//
	// The stuff below exists to emulate DX9's LockRect function, which
	// allowed you to modify a portion of the texture without discarding the
	// rest of the texture.
	//

	// For locking textures
	ID3D11Resource *m_pLockedTexture;
	D3D11_MAPPED_SUBRESOURCE m_MappedData;
	unsigned char *m_pLockedFace;
	unsigned int m_nLockedFaceSize;
	UINT m_LockedSubresource;
	bool m_bLocked;

	// We store copies of dynamic textures in system memory to
	// allow for quickly modifying regions of dynamic textures.
	// (Used by lightmaps)
	union
	{
		unsigned char *m_pRamImage;
		unsigned char **m_ppRamImage;
	};
	
	
};

FORCEINLINE ID3D11Resource *CTextureDx11::GetTexture() const
{
	return m_pTexture;
}

FORCEINLINE ID3D11Resource *CTextureDx11::GetTexture( int n ) const
{
	return m_ppTexture[n];
}

FORCEINLINE ID3D11Texture1D *CTextureDx11::GetTexture1D() const
{
	return (ID3D11Texture1D *)m_pTexture;
}

FORCEINLINE ID3D11Texture1D *CTextureDx11::GetTexture1D( int n ) const
{
	Assert( Is2D() );
	return (ID3D11Texture1D *)m_ppTexture[n];
}

FORCEINLINE ID3D11Texture2D *CTextureDx11::GetTexture2D() const
{
	Assert( Is2D() );
	return (ID3D11Texture2D *)m_pTexture;
}

FORCEINLINE ID3D11Texture2D *CTextureDx11::GetTexture2D( int n ) const
{
	Assert( Is2D() );
	return (ID3D11Texture2D *)m_ppTexture[n];
}

FORCEINLINE ID3D11Texture3D *CTextureDx11::GetTexture3D() const
{
	Assert( Is3D() );
	return (ID3D11Texture3D *)m_pTexture;
}

FORCEINLINE ID3D11Texture3D *CTextureDx11::GetTexture3D( int n ) const
{
	Assert( Is3D() );
	return (ID3D11Texture3D *)m_ppTexture[n];
}

FORCEINLINE bool CTextureDx11::Is1D() const
{
	return m_iTextureDimensions == TEXTURE_1D;
}

FORCEINLINE bool CTextureDx11::Is2D() const
{
	return m_iTextureDimensions == TEXTURE_2D;
}

FORCEINLINE bool CTextureDx11::Is3D() const
{
	return m_iTextureDimensions == TEXTURE_3D;
}

FORCEINLINE ID3D11ShaderResourceView *CTextureDx11::GetView() const
{
	if ( m_NumCopies > 1 )
	{
		return GetView( m_CurrentCopy );
	}

	return m_pView;
}

FORCEINLINE ID3D11ShaderResourceView *CTextureDx11::GetView( int n ) const
{
	Assert( m_NumCopies > 1 );
	return m_ppView[n];
}

FORCEINLINE ID3D11DepthStencilView *CTextureDx11::GetDepthStencilView() const
{
	Assert( m_iTextureType == TEXTURE_DEPTHSTENCIL );
	return m_pDepthStencilView;
}

FORCEINLINE ID3D11RenderTargetView *CTextureDx11::GetRenderTargetView() const
{
	Assert( m_iTextureType == TEXTURE_RENDERTARGET );
	return m_pRenderTargetView;
}

FORCEINLINE ID3D11SamplerState *CTextureDx11::GetSamplerState() const
{
	return m_pSamplerState;
}

FORCEINLINE void CTextureDx11::SetTexture( ID3D11Resource *tex )
{
	m_pTexture = tex;
}

FORCEINLINE void CTextureDx11::SetTexture( int n, ID3D11Resource *tex )
{
	m_ppTexture[n] = tex;
}