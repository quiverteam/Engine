#include "VertexBufferDx11.h"
#include "shaderapidx9/locald3dtypes.h"
#include "shaderapidx11_global.h"
#include "shaderdevicedx11.h"
#include "shaderapi/ishaderutil.h"
#include "tier0/vprof.h"
#include "tier0/dbg.h"
#include "materialsystem/ivballoctracker.h"
#include "tier2/tier2.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
//
// Dx11 implementation of a vertex buffer
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// globals
//-----------------------------------------------------------------------------
#ifdef _DEBUG
int CVertexBufferDx11::s_nBufferCount = 0;
#endif


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CVertexBufferDx11::CVertexBufferDx11( ShaderBufferType_t type, VertexFormat_t fmt, int nVertexCount, const char* pBudgetGroupName ) :
	BaseClass( pBudgetGroupName )
{
	Assert( nVertexCount != 0 );

	m_pVertexMemory = NULL;
	m_nVerticesLocked = 0;
	m_pVertexBuffer = NULL;
	m_VertexFormat = fmt;
	m_nVertexCount = ( fmt == VERTEX_FORMAT_UNKNOWN ) ? 0 : nVertexCount;
	m_VertexSize = VertexFormatSize( m_VertexFormat );
	m_nBufferSize = ( fmt == VERTEX_FORMAT_UNKNOWN ) ? nVertexCount : nVertexCount * VertexSize();
	m_Position = 0;
	m_bIsLocked = false;
	m_bIsDynamic = ( type == SHADER_BUFFER_TYPE_DYNAMIC ) || ( type == SHADER_BUFFER_TYPE_DYNAMIC_TEMP );
	m_bFlush = false;
}

CVertexBufferDx11::~CVertexBufferDx11()
{
	Free();
}

bool CVertexBufferDx11::HasEnoughRoom( int nVertCount ) const
{
	return ( NextLockOffset() + ( nVertCount * m_VertexSize ) ) <= m_nBufferSize;
}

//-----------------------------------------------------------------------------
// Creates, destroys the vertex buffer
//-----------------------------------------------------------------------------
bool CVertexBufferDx11::Allocate()
{
	Assert( !m_pVertexBuffer );

	if ( !m_bIsDynamic )
	{
		m_pVertexMemory = (unsigned char *)malloc( m_nBufferSize );
		memset( m_pVertexMemory, 0, m_nBufferSize );
	}
	else
	{
		m_pVertexMemory = NULL;
	}

	m_Position = 0;

	D3D11_BUFFER_DESC bd;
	if ( m_bIsDynamic )
	{
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.CPUAccessFlags = 0;
	}
		
	bd.ByteWidth = m_nBufferSize;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.MiscFlags = 0;

	//Log( "Creating D3D vertex buffer: size: %i\n", m_nBufferSize );

	//if ( m_nBufferSize <= 0 )
	//{
	//	int *p = 0;
	//	*p = 1;
	//}
		

	HRESULT hr = D3D11Device()->CreateBuffer( &bd, NULL, &m_pVertexBuffer );
	bool bOk = !FAILED( hr ) && ( m_pVertexBuffer != 0 );

	if ( bOk )
	{
		// Track VB allocations
		g_VBAllocTracker->CountVB( m_pVertexBuffer, m_bIsDynamic, m_nBufferSize, VertexSize(), GetVertexFormat() );

		if ( !m_bIsDynamic )
		{
			VPROF_INCREMENT_GROUP_COUNTER( "TexGroup_global_" TEXTURE_GROUP_STATIC_INDEX_BUFFER,
						       COUNTER_GROUP_TEXTURE_GLOBAL, m_nBufferSize );
		}
		else
		{
			VPROF_INCREMENT_GROUP_COUNTER( "TexGroup_global_" TEXTURE_GROUP_DYNAMIC_INDEX_BUFFER,
						       COUNTER_GROUP_TEXTURE_GLOBAL, m_nBufferSize );
					       // Dynamic meshes should never be compressed (slows down writing to them)
			Assert( CompressionType( GetVertexFormat() ) == VERTEX_COMPRESSION_NONE );
		}
#ifdef _DEBUG
		++s_nBufferCount;
#endif
	}

	return bOk;
}

void CVertexBufferDx11::Free()
{
	if ( m_pVertexBuffer )
	{
#ifdef _DEBUG
		--s_nBufferCount;
#endif

		// Track VB allocations
		g_VBAllocTracker->UnCountVB( m_pVertexBuffer );

		m_pVertexBuffer->Release();
		m_pVertexBuffer = NULL;

		if ( !m_bIsDynamic )
		{
			VPROF_INCREMENT_GROUP_COUNTER( "TexGroup_global_" TEXTURE_GROUP_STATIC_INDEX_BUFFER,
						       COUNTER_GROUP_TEXTURE_GLOBAL, -m_nBufferSize );
		}
		else
		{
			VPROF_INCREMENT_GROUP_COUNTER( "TexGroup_global_" TEXTURE_GROUP_DYNAMIC_INDEX_BUFFER,
						       COUNTER_GROUP_TEXTURE_GLOBAL, -m_nBufferSize );
		}
	}

	if ( m_pVertexMemory )
	{
		free( m_pVertexMemory );
		m_pVertexMemory = NULL;
	}
}


//-----------------------------------------------------------------------------
// Vertex Buffer info
//-----------------------------------------------------------------------------
int CVertexBufferDx11::VertexCount() const
{
	//Assert( !m_bIsDynamic );
	return m_nVertexCount;
}


//-----------------------------------------------------------------------------
// Returns the buffer format (only valid for static index buffers)
//-----------------------------------------------------------------------------
VertexFormat_t CVertexBufferDx11::GetVertexFormat() const
{
	//Assert( !m_bIsDynamic );
	return m_VertexFormat;
}


//-----------------------------------------------------------------------------
// Returns true if the buffer is dynamic
//-----------------------------------------------------------------------------
bool CVertexBufferDx11::IsDynamic() const
{
	return m_bIsDynamic;
}


//-----------------------------------------------------------------------------
// Only used by dynamic buffers, indicates the next lock should perform a discard.
//-----------------------------------------------------------------------------
void CVertexBufferDx11::Flush()
{
	// This strange-looking line makes a flush only occur if the buffer is dynamic.
	m_bFlush = m_bIsDynamic;
}


//-----------------------------------------------------------------------------
// Casts a dynamic buffer to be a particular vertex type
//-----------------------------------------------------------------------------
void CVertexBufferDx11::BeginCastBuffer( VertexFormat_t format )
{
	Assert( format != VERTEX_FORMAT_UNKNOWN );
	Assert( m_bIsDynamic && ( m_VertexFormat == VERTEX_FORMAT_UNKNOWN || m_VertexFormat == format ) );
	if ( !m_bIsDynamic )
		return;

	m_VertexFormat = format;
	m_VertexSize = VertexFormatSize( m_VertexFormat );
	m_nVertexCount = m_nBufferSize / VertexSize();
}

void CVertexBufferDx11::EndCastBuffer()
{
	Assert( m_bIsDynamic && m_VertexFormat != 0 );
	if ( !m_bIsDynamic )
		return;
	m_VertexFormat = 0;
	m_nVertexCount = 0;
	m_VertexSize = 0;
}


//-----------------------------------------------------------------------------
// Returns the number of indices that can be written into the buffer
//-----------------------------------------------------------------------------
int CVertexBufferDx11::GetRoomRemaining() const
{
	return ( m_nBufferSize - NextLockOffset() ) / VertexSize();
}


//-----------------------------------------------------------------------------
// Lock, unlock
//-----------------------------------------------------------------------------
bool CVertexBufferDx11::Lock( int nMaxVertexCount, bool bAppend, VertexDesc_t& desc )
{
	return LockEx( 0, nMaxVertexCount, desc );
}

bool CVertexBufferDx11::LockEx( int nFirstVertex, int nMaxVertexCount, VertexDesc_t &desc )
{
	Assert( !m_bIsLocked && ( nMaxVertexCount != 0 ) && ( nMaxVertexCount <= m_nVertexCount ) );
	Assert( m_VertexFormat != 0 );

	// FIXME: Why do we need to sync matrices now?
	ShaderUtil()->SyncMatrices();
	//g_ShaderMutex.Lock();

	// This can happen if the buffer was locked but a type wasn't bound
	if ( m_VertexFormat == 0 )
	{
		//Log( "No vertex format!\n" );
		goto vertexBufferLockFailed;
	}


	// Just give the app crap buffers to fill up while we're suppressed...
	if ( g_pShaderDevice->IsDeactivated() || ( nMaxVertexCount == 0 ) )
		goto vertexBufferLockFailed;

	// Did we ask for something too large?
	if ( nMaxVertexCount > m_nVertexCount )
	{
		Warning( "Too many vertices for vertex buffer. . tell a programmer (%d>%d)\n", nMaxVertexCount, m_nVertexCount );
		goto vertexBufferLockFailed;
	}

	// We might not have a buffer owing to alt-tab type stuff
	if ( !m_pVertexBuffer )
	{
		if ( !Allocate() )
		{
			//Log( "Couldn't allocate vertex buffer\n" );
			goto vertexBufferLockFailed;
		}

	}

	//Log( "Locking vertex buffer %p\n", m_pVertexBuffer );

	// Check to see if we have enough memory 
	//int nMemoryRequired = nMaxVertexCount * VertexSize();

	if ( m_bIsDynamic )
	{
		D3D11_MAPPED_SUBRESOURCE lockedData;
		HRESULT hr;

		D3D11_MAP map;

		bool bHasEnoughMemory = HasEnoughRoom( nMaxVertexCount );

		// If we're not appending, no overwrite unless we don't have enough room
		// If we're a static buffer, always discard if we're not appending
		if ( !m_bFlush && bHasEnoughMemory )
		{
			map = ( m_Position == 0 ) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
		}
		else
		{
			map = D3D11_MAP_WRITE_DISCARD;
			m_Position = 0;
			m_bFlush = false;
		}

		int nLockOffset = NextLockOffset();

		//goto vertexBufferLockFailed;
		hr = D3D11DeviceContext()->Map( m_pVertexBuffer, 0, map, 0, &lockedData );
		if ( FAILED( hr ) )
		{
			Warning( "Failed to lock vertex buffer in CVertexBufferDx11::Lock\n" );
			goto vertexBufferLockFailed;
		}

		ComputeVertexDescription( (unsigned char *)lockedData.pData + nLockOffset, m_VertexFormat, desc );
		desc.m_nFirstVertex = nLockOffset / VertexSize();
		desc.m_nOffset = nLockOffset;
		m_bIsLocked = true;
		return true;
	}
	else
	{
		// Static vertex buffers

		bool bHasEnoughMemory = ( nFirstVertex + nMaxVertexCount ) <= m_nVertexCount;
		if ( !bHasEnoughMemory )
		{
			goto vertexBufferLockFailed;
		}
		int nLockOffset;
		if ( nFirstVertex >= 0 )
			nLockOffset = nFirstVertex * VertexSize();
		else
			nLockOffset = m_Position;
		m_Position = nLockOffset;
		// Static vertex buffer case
		ComputeVertexDescription( m_pVertexMemory + nLockOffset, m_VertexFormat, desc );
		desc.m_nFirstVertex = nFirstVertex;
		desc.m_nOffset = nLockOffset;
		m_nVerticesLocked = nMaxVertexCount;
		m_bIsLocked = true;
		return true;
	}


vertexBufferLockFailed:
	g_ShaderMutex.Unlock();

	// Set up a bogus index descriptor
	ComputeVertexDescription( 0, 0, desc );
	desc.m_nFirstVertex = 0;
	desc.m_nOffset = 0;
	return false;
}

void CVertexBufferDx11::Unlock( int nWrittenVertexCount, VertexDesc_t& desc )
{
	//Log( "Unlocking vertex buffer %p\n", m_pVertexBuffer );
	Assert( nWrittenVertexCount <= m_nVertexCount );

	// NOTE: This can happen if the lock occurs during alt-tab
	// or if another application is initializing
	if ( !m_bIsLocked )
		return;

	if ( m_pVertexBuffer )
	{
		if ( m_bIsDynamic )
		{
			D3D11DeviceContext()->Unmap( m_pVertexBuffer, 0 );

			int nLockOffset = NextLockOffset();
			int nBufferSize = nWrittenVertexCount * m_VertexSize;

			m_Position = nLockOffset + nBufferSize;
		}
		else
		{
			D3D11_BOX box;
			box.back = 1;
			box.front = 0;
			box.bottom = 1;
			box.top = 0;
			box.left = m_Position;
			box.right = box.left + ( m_nVerticesLocked * VertexSize() );
			D3D11DeviceContext()->UpdateSubresource( m_pVertexBuffer, 0, &box, m_pVertexMemory + m_Position, 0, 0 );

			m_Position += m_nVerticesLocked * VertexSize();
		}
		
	}

	//Spew( nWrittenVertexCount, desc );

	m_bIsLocked = false;
	//g_ShaderMutex.Unlock();
}

unsigned char *CVertexBufferDx11::Modify( bool bReadOnly, int nFirstVertex, int nVertexCount )
{
	if ( nVertexCount == 0 )
		return NULL;

	Assert( m_pVertexBuffer && !m_bIsDynamic );

	if ( nFirstVertex + nVertexCount > m_nVertexCount )
	{
		Assert( 0 );
		return NULL;
	}

	unsigned char *pData = m_pVertexMemory + ( nFirstVertex * m_VertexSize );
	m_nVerticesLocked = nVertexCount;

	m_Position = nFirstVertex * m_VertexSize;
	m_bIsLocked = true;

	return pData;
}