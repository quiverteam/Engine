#pragma once

#include "platform.h"
#include "bitmap/imageformat.h"
#include "Dx11Global.h"

class CTextureDx11
{
public:
	enum
	{
		TEXTURE_1D,
		TEXTURE_2D,
		TEXTURE_3D,
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

	bool Is1D() const;
	bool Is2D() const;
	bool Is3D() const;

	void SetTexture( ID3D11Resource *tex );
	void SetTexture( int n, ID3D11Resource *tex );
	//void SetView( ID3D11ShaderResourceView *view );

public:
	union
	{
		ID3D11Resource *m_pTexture;
		ID3D11Resource **m_ppTexture;
	};

	ID3D11SamplerState *m_pSamplerState;

	union
	{
		ID3D11ShaderResourceView *m_pView;
		ID3D11DepthStencilView *m_pDepthStencilView;
		ID3D11RenderTargetView *m_pRenderTargetView;
	};

public:
	D3D11_TEXTURE_ADDRESS_MODE m_UTexWrap;
	D3D11_TEXTURE_ADDRESS_MODE m_VTexWrap;
	D3D11_TEXTURE_ADDRESS_MODE m_WTexWrap;
	D3D11_FILTER m_Filter;
	unsigned char m_NumLevels;
	unsigned char m_SwitchNeeded;
	unsigned char m_NumCopies;
	unsigned char m_CurrentCopy;
	short m_Count;
	short m_CountIndex;
	short m_Depth;
	int m_iTextureType;
	int m_nWidth;
	int m_nHeight;
	ImageFormat m_Format;
	int m_nSizeTexels;
	int m_nSizeBytes;
	int m_LastBoundFrame;
	int m_nTimesBoundMax;
	int m_nTimesBoundThisFrame;
	unsigned short m_nFlags;
	int m_CreationFlags;
	
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
	return m_iTextureType == TEXTURE_1D;
}

FORCEINLINE bool CTextureDx11::Is2D() const
{
	return m_iTextureType == TEXTURE_2D;
}

FORCEINLINE bool CTextureDx11::Is3D() const
{
	return m_iTextureType == TEXTURE_3D;
}

FORCEINLINE ID3D11ShaderResourceView *CTextureDx11::GetView() const
{
	return m_pView;
}

FORCEINLINE ID3D11DepthStencilView *CTextureDx11::GetDepthStencilView() const
{
	return m_pDepthStencilView;
}

FORCEINLINE ID3D11RenderTargetView *CTextureDx11::GetRenderTargetView() const
{
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