#include "IndexBufferDx11.h"
#include "shaderapidx9/locald3dtypes.h"
#include "shaderapidx11_global.h"
#include "shaderdevicedx11.h"
#include "shaderapi/ishaderutil.h"
#include "tier0/vprof.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
//
// Dx11 implementation of an index buffer
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// globals
//-----------------------------------------------------------------------------

// shove indices into this if you don't actually want indices
static unsigned int s_nScratchIndexBuffer = 0;

#ifdef _DEBUG
int CIndexBufferDx11::s_nBufferCount = 0;
#endif


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CIndexBufferDx11::CIndexBufferDx11( ShaderBufferType_t type, MaterialIndexFormat_t fmt, int nIndexCount, const char* pBudgetGroupName ) :
	BaseClass( pBudgetGroupName )
{
	// NOTE: MATERIAL_INDEX_FORMAT_UNKNOWN can't be dealt with under dx9
	// because format is bound at buffer creation time. What we'll do 
	// is just arbitrarily choose to use a 16-bit index buffer of the same size	
	if ( fmt == MATERIAL_INDEX_FORMAT_UNKNOWN )
	{
		fmt = MATERIAL_INDEX_FORMAT_16BIT;
		nIndexCount /= 2;
	}

	//Assert( nIndexCount != 0 );
	Assert( IsDynamicBufferType( type ) || ( fmt != MATERIAL_INDEX_FORMAT_UNKNOWN ) );

	m_pIndexBuffer = NULL;
	m_IndexFormat = fmt;
	m_nIndexSize = SizeForIndex( fmt );
	m_nIndexCount =  nIndexCount;
	m_nBufferSize = nIndexCount * m_nIndexSize;
	m_nFirstUnwrittenOffset = 0;
	m_bIsLocked = false;
	m_bIsDynamic = IsDynamicBufferType( type );
	m_bFlush = false;

#ifdef _DEBUG
	m_pShadowIndices = new unsigned short[m_nIndexCount];
	m_NumIndices = nIndexCount;
#endif

	// NOTE: This has to happen at the end since m_IndexFormat must be valid for IndexSize() to work
	//if ( m_bIsDynamic )
	//{
	//	m_IndexFormat = MATERIAL_INDEX_FORMAT_UNKNOWN;
	//	m_nIndexCount = 0;
	//}
}

CIndexBufferDx11::~CIndexBufferDx11()
{
#ifdef _DEBUG
	if ( m_pShadowIndices )
	{
		delete[] m_pShadowIndices;
		m_pShadowIndices = NULL;
	}
#endif
	Free();
}

bool CIndexBufferDx11::HasEnoughRoom( int nIndexCount ) const
{
	return ( GetRoomRemaining() - nIndexCount ) >= 0;
}


//-----------------------------------------------------------------------------
// Creates, destroys the index buffer
//-----------------------------------------------------------------------------
bool CIndexBufferDx11::Allocate()
{
	Assert( !m_pIndexBuffer );

	m_nFirstUnwrittenOffset = 0;

	//Log( "Creating D3D index buffer: size: %i\n", m_nBufferSize );

	D3D11_BUFFER_DESC bd;
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = m_nBufferSize;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;

	HRESULT hr = D3D11Device()->CreateBuffer( &bd, NULL, &m_pIndexBuffer );
	bool bOk = !FAILED( hr ) && ( m_pIndexBuffer != NULL );

	if ( bOk )
	{
		if ( !m_bIsDynamic )
		{
			VPROF_INCREMENT_GROUP_COUNTER( "TexGroup_global_" TEXTURE_GROUP_STATIC_INDEX_BUFFER,
						       COUNTER_GROUP_TEXTURE_GLOBAL, m_nBufferSize );
		}
		else
		{
			VPROF_INCREMENT_GROUP_COUNTER( "TexGroup_global_" TEXTURE_GROUP_DYNAMIC_INDEX_BUFFER,
						       COUNTER_GROUP_TEXTURE_GLOBAL, m_nBufferSize );
		}
#ifdef _DEBUG
		++s_nBufferCount;
#endif
	}

	return bOk;
}

void CIndexBufferDx11::Free()
{
	if ( m_pIndexBuffer )
	{
#ifdef _DEBUG
		--s_nBufferCount;
#endif

		m_pIndexBuffer->Release();
		m_pIndexBuffer = NULL;

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
}


//-----------------------------------------------------------------------------
// Returns the buffer size (only valid for static index buffers)
//-----------------------------------------------------------------------------
int CIndexBufferDx11::IndexCount() const
{
	//Assert( !m_bIsDynamic );
	return m_nIndexCount;
}


//-----------------------------------------------------------------------------
// Returns the buffer format (only valid for static index buffers)
//-----------------------------------------------------------------------------
MaterialIndexFormat_t CIndexBufferDx11::IndexFormat() const
{
	//Assert( !m_bIsDynamic );
	return m_IndexFormat;
}


//-----------------------------------------------------------------------------
// Returns true if the buffer is dynamic
//-----------------------------------------------------------------------------
bool CIndexBufferDx11::IsDynamic() const
{
	return m_bIsDynamic;
}


//-----------------------------------------------------------------------------
// Only used by dynamic buffers, indicates the next lock should perform a discard.
//-----------------------------------------------------------------------------
void CIndexBufferDx11::Flush()
{
	// This strange-looking line makes a flush only occur if the buffer is dynamic.
	m_bFlush = m_bIsDynamic;
}

int CIndexBufferDx11::SizeForIndex( MaterialIndexFormat_t fmt ) const
{
	switch ( fmt )
	{
	default:
	case MATERIAL_INDEX_FORMAT_UNKNOWN:
		return 0;
	case MATERIAL_INDEX_FORMAT_16BIT:
		return 2;
	case MATERIAL_INDEX_FORMAT_32BIT:
		return 4;
	}
}

//-----------------------------------------------------------------------------
// Casts a dynamic buffer to be a particular index type
//-----------------------------------------------------------------------------
void CIndexBufferDx11::BeginCastBuffer( MaterialIndexFormat_t format )
{
	Assert( format != MATERIAL_INDEX_FORMAT_UNKNOWN );
	Assert( m_bIsDynamic && ( m_IndexFormat == MATERIAL_INDEX_FORMAT_UNKNOWN || m_IndexFormat == format ) );
	if ( !m_bIsDynamic )
		return;

	m_IndexFormat = format;
	m_nIndexSize = SizeForIndex( m_IndexFormat );
	m_nIndexCount = m_nBufferSize / IndexSize();
}

void CIndexBufferDx11::EndCastBuffer()
{
	Assert( m_bIsDynamic && m_IndexFormat != MATERIAL_INDEX_FORMAT_UNKNOWN );
	if ( !m_bIsDynamic )
		return;
	m_IndexFormat = MATERIAL_INDEX_FORMAT_UNKNOWN;
	m_nIndexCount = 0;
	m_nIndexSize = 0;
}


//-----------------------------------------------------------------------------
// Returns the number of indices that can be written into the buffer
//-----------------------------------------------------------------------------
int CIndexBufferDx11::GetRoomRemaining() const
{
	return ( m_nBufferSize - m_nFirstUnwrittenOffset ) / IndexSize();
}


//-----------------------------------------------------------------------------
// Locks, unlocks the mesh
//-----------------------------------------------------------------------------
bool CIndexBufferDx11::Lock( int nMaxIndexCount, bool bAppend, IndexDesc_t &desc )
{
	Assert( !m_bIsLocked && ( nMaxIndexCount != 0 ) && ( nMaxIndexCount <= m_nIndexCount ) );
	if ( m_bIsLocked || ( nMaxIndexCount == 0 ) || ( nMaxIndexCount > m_nIndexCount ) )
	{
		//DebuggerBreak();
	}
	Assert( m_IndexFormat != MATERIAL_INDEX_FORMAT_UNKNOWN );

	// FIXME: Why do we need to sync matrices now?
	ShaderUtil()->SyncMatrices();
	g_ShaderMutex.Lock();

	D3D11_MAPPED_SUBRESOURCE lockedData;
	HRESULT hr;

	// This can happen if the buffer was locked but a type wasn't bound
	if ( m_IndexFormat == MATERIAL_INDEX_FORMAT_UNKNOWN )
		goto indexBufferLockFailed;

	// Just give the app crap buffers to fill up while we're suppressed...
	if ( g_pShaderDevice->IsDeactivated() || ( nMaxIndexCount == 0 ) )
		goto indexBufferLockFailed;

	// Did we ask for something too large?
	if ( nMaxIndexCount > m_nIndexCount )
	{
		Warning( "Too many indices for index buffer. . tell a programmer (%d>%d)\n", nMaxIndexCount, m_nIndexCount );
		DebuggerBreak();
		goto indexBufferLockFailed;
	}

	// We might not have a buffer owing to alt-tab type stuff
	if ( !m_pIndexBuffer )
	{
		if ( !Allocate() )
			goto indexBufferLockFailed;
	}

	//Log( "Locking index buffer %p\n", m_pIndexBuffer );

	// Check to see if we have enough memory 
	int nMemoryRequired = nMaxIndexCount * IndexSize();
	bool bHasEnoughMemory = ( m_nFirstUnwrittenOffset + nMemoryRequired <= m_nBufferSize );

	D3D11_MAP map;
	if ( bAppend )
	{
		// Can't have the first lock after a flush be an appending lock
		Assert( !m_bFlush );

		// If we're appending and we don't have enough room, then puke!
		if ( !bHasEnoughMemory || m_bFlush )
			goto indexBufferLockFailed;
		map = ( m_nFirstUnwrittenOffset == 0 ) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
	}
	else
	{
		// If we're not appending, no overwrite unless we don't have enough room
		if ( !m_bFlush && bHasEnoughMemory && m_bIsDynamic )
		{
			map = ( m_nFirstUnwrittenOffset == 0 ) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
		}
		else
		{
			map = D3D11_MAP_WRITE_DISCARD;
			m_nFirstUnwrittenOffset = 0;
			m_bFlush = false;
		}
	}
	//goto indexBufferLockFailed;
	hr = D3D11DeviceContext()->Map( m_pIndexBuffer, 0, map, 0, &lockedData );
	if ( FAILED( hr ) )
	{
		Warning( "Failed to lock index buffer in CIndexBufferDx11::Lock\n" );
		goto indexBufferLockFailed;
	}
	//Log( "LOcking index buffer at %i\n", m_nFirstUnwrittenOffset );
	desc.m_pIndices = (unsigned short*)( (unsigned char*)lockedData.pData + m_nFirstUnwrittenOffset );
	desc.m_nIndexSize = IndexSize() >> 1;
	desc.m_nOffset = m_nFirstUnwrittenOffset;
	desc.m_nFirstIndex = desc.m_nOffset / IndexSize();
#ifdef _DEBUG
	m_LockedStartIndex = desc.m_nFirstIndex;
	m_LockedNumIndices = nMaxIndexCount;
#endif
	m_bIsLocked = true;
	return true;

indexBufferLockFailed:
	g_ShaderMutex.Unlock();

	// Set up a bogus index descriptor
	desc.m_pIndices = (unsigned short*)( &s_nScratchIndexBuffer );
	desc.m_nFirstIndex = 0;
	desc.m_nIndexSize = 0;
	desc.m_nOffset = 0;
	return false;
}

void CIndexBufferDx11::Unlock( int nWrittenIndexCount, IndexDesc_t& desc )
{
	//Log( "Unlocking index buffer %p\n", m_pIndexBuffer );
	Assert( nWrittenIndexCount <= m_nIndexCount );

	// For write-combining, ensure we always have locked memory aligned to 4-byte boundaries
	if ( m_bIsDynamic )
		nWrittenIndexCount = nWrittenIndexCount;

	//Log( "Wrote %i indices\n", nWrittenIndexCount );

	// NOTE: This can happen if the lock occurs during alt-tab
	// or if another application is initializing
	if ( !m_bIsLocked )
		return;

	//Spew( nWrittenIndexCount, desc );

	if ( m_pIndexBuffer )
	{
		D3D11DeviceContext()->Unmap( m_pIndexBuffer, 0 );
	}

#ifdef _DEBUG
	m_LockedStartIndex = 0;
	m_LockedNumIndices = 0;
#endif

	m_nFirstUnwrittenOffset += nWrittenIndexCount * IndexSize();
	m_bIsLocked = false;
	g_ShaderMutex.Unlock();
}


//-----------------------------------------------------------------------------
// Locks, unlocks an existing mesh
//-----------------------------------------------------------------------------
void CIndexBufferDx11::ModifyBegin( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t& desc )
{
	Assert( 0 );
}

void CIndexBufferDx11::ModifyEnd( IndexDesc_t& desc )
{

}