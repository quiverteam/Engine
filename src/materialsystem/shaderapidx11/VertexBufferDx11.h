#pragma once

#include "shaderapidx9/meshbase.h"
#include "shaderapi/ishaderdevice.h"

//-----------------------------------------------------------------------------
// Forward declaration
//-----------------------------------------------------------------------------
struct ID3D11Buffer;

//-----------------------------------------------------------------------------
// Dx11 implementation of a vertex buffer
//-----------------------------------------------------------------------------
class CVertexBufferDx11 : public CVertexBufferBase
{
	typedef CVertexBufferBase BaseClass;

	// Methods of IVertexBuffer
public:
	virtual int VertexCount() const;
	virtual VertexFormat_t GetVertexFormat() const;
	unsigned char *Modify( bool bReadOnly, int nFirstVertex, int nVertexCount );
	virtual bool Lock( int nMaxVertexCount, bool bAppend, VertexDesc_t& desc );
	virtual void Unlock( int nWrittenVertexCount, VertexDesc_t& desc );
	virtual bool IsDynamic() const;
	virtual void BeginCastBuffer( VertexFormat_t format );
	virtual	void EndCastBuffer();
	virtual int GetRoomRemaining() const;
	bool HasEnoughRoom( int nVertCount ) const;

	int NextLockOffset() const
	{
		int nNextOffset = ( m_nFirstUnwrittenOffset + m_VertexSize - 1 ) / m_VertexSize;
		nNextOffset *= m_VertexSize;
		return nNextOffset;
	}

	// used to alter the characteristics after creation
	// allows one dynamic vb to be shared for multiple formats
	void ChangeConfiguration( int vertexSize, int totalSize, VertexFormat_t fmt )
	{
		Assert( m_bIsDynamic && !m_bIsLocked && vertexSize );
		m_VertexSize = vertexSize;
		m_nVertexCount = m_nBufferSize / vertexSize;
		m_VertexFormat = fmt;
	}

	// Other public methods
public:
	// constructor, destructor
	CVertexBufferDx11( ShaderBufferType_t type, VertexFormat_t fmt, int nVertexCount, const char* pBudgetGroupName );
	virtual ~CVertexBufferDx11();

	ID3D11Buffer* GetDx11Buffer() const;
	int VertexSize() const;

	// Only used by dynamic buffers, indicates the next lock should perform a discard.
	void Flush();

protected:
	// Creates, destroys the index buffer
	bool Allocate();
	void Free();

	ID3D11Buffer* m_pVertexBuffer;
	VertexFormat_t m_VertexFormat;
	int m_VertexSize;
	int m_nVertexCount;
	int m_nBufferSize;
	int m_nFirstUnwrittenOffset;
	bool m_bIsLocked : 1;
	bool m_bIsDynamic : 1;
	bool m_bFlush : 1;				// Used only for dynamic buffers, indicates to discard the next time

#ifdef _DEBUG
	static int s_nBufferCount;
#endif
};

//-----------------------------------------------------------------------------
// inline methods for CVertexBufferDx11
//-----------------------------------------------------------------------------
inline ID3D11Buffer* CVertexBufferDx11::GetDx11Buffer() const
{
	return m_pVertexBuffer;
}

inline int CVertexBufferDx11::VertexSize() const
{
	return m_VertexSize;
}