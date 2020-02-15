//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include <d3d11.h>
#undef GetCommandLine

#include "meshdx11.h"
#include "utlvector.h"
#include "materialsystem/imaterialsystem.h"
#include "IHardwareConfigInternal.h"
#include "shaderapi_global.h"
#include "shaderapi/ishaderutil.h"
#include "shaderapi/ishaderapi.h"
#include "shaderdevicedx11.h"
#include "materialsystem/imesh.h"
#include "tier0/vprof.h"
#include "tier0/dbg.h"
#include "materialsystem/idebugtextureinfo.h"
#include "materialsystem/ivballoctracker.h"
#include "tier2/tier2.h"


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
CVertexBufferDx11::CVertexBufferDx11( ShaderBufferType_t type, VertexFormat_t fmt, int nVertexCount, const char *pBudgetGroupName ) :
	BaseClass( pBudgetGroupName )
{
	Assert( nVertexCount != 0 );

	m_pVertexBuffer = NULL;
	m_VertexFormat = fmt;
	m_nVertexCount = ( fmt == VERTEX_FORMAT_UNKNOWN ) ? 0 : nVertexCount;
	m_nBufferSize = ( fmt == VERTEX_FORMAT_UNKNOWN ) ? nVertexCount : nVertexCount * VertexSize();
	m_nFirstUnwrittenOffset = 0;
	m_bIsLocked = false;
	m_bIsDynamic = ( type == SHADER_BUFFER_TYPE_DYNAMIC ) || ( type == SHADER_BUFFER_TYPE_DYNAMIC_TEMP );
	m_bFlush = false;
}

CVertexBufferDx11::~CVertexBufferDx11()
{
	Free();
}


//-----------------------------------------------------------------------------
// Creates, destroys the vertex buffer
//-----------------------------------------------------------------------------
bool CVertexBufferDx11::Allocate( )
{
	Assert( !m_pVertexBuffer );

	m_nFirstUnwrittenOffset = 0;

	D3D11_BUFFER_DESC bd;
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = m_nBufferSize;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;

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
				COUNTER_GROUP_TEXTURE_GLOBAL, - m_nBufferSize );
		}
		else
		{
			VPROF_INCREMENT_GROUP_COUNTER( "TexGroup_global_" TEXTURE_GROUP_DYNAMIC_INDEX_BUFFER, 
				COUNTER_GROUP_TEXTURE_GLOBAL, - m_nBufferSize );
		}
	}
}


//-----------------------------------------------------------------------------
// Vertex Buffer info
//-----------------------------------------------------------------------------
int CVertexBufferDx11::VertexCount() const
{
	Assert( !m_bIsDynamic );
	return m_nVertexCount;
}


//-----------------------------------------------------------------------------
// Returns the buffer format (only valid for static index buffers)
//-----------------------------------------------------------------------------
VertexFormat_t CVertexBufferDx11::GetVertexFormat() const
{
	Assert( !m_bIsDynamic );
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
	Assert( format != MATERIAL_INDEX_FORMAT_UNKNOWN );
	Assert( m_bIsDynamic && ( m_VertexFormat == 0 || m_VertexFormat == format ) );
	if ( !m_bIsDynamic )
		return;

	m_VertexFormat = format;
	m_nVertexCount = m_nBufferSize / VertexSize();
}

void CVertexBufferDx11::EndCastBuffer( )
{
	Assert( m_bIsDynamic && m_VertexFormat != 0 );
	if ( !m_bIsDynamic )
		return;
	m_VertexFormat = 0;
	m_nVertexCount = 0;
}


//-----------------------------------------------------------------------------
// Returns the number of indices that can be written into the buffer
//-----------------------------------------------------------------------------
int CVertexBufferDx11::GetRoomRemaining() const
{
	return ( m_nBufferSize - m_nFirstUnwrittenOffset ) / VertexSize();
}


//-----------------------------------------------------------------------------
// Lock, unlock
//-----------------------------------------------------------------------------
bool CVertexBufferDx11::Lock( int nMaxVertexCount, bool bAppend, VertexDesc_t &desc )
{
	Assert( !m_bIsLocked && ( nMaxVertexCount != 0 ) && ( nMaxVertexCount <= m_nVertexCount ) );
	Assert( m_VertexFormat != 0 );

	// FIXME: Why do we need to sync matrices now?
	ShaderUtil()->SyncMatrices();
	g_ShaderMutex.Lock();

	D3D11_MAPPED_SUBRESOURCE lockedData;
	HRESULT hr;

	// This can happen if the buffer was locked but a type wasn't bound
	if ( m_VertexFormat == 0 )
		goto vertexBufferLockFailed;

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
			goto vertexBufferLockFailed;
	}

	// Check to see if we have enough memory 
	int nMemoryRequired = nMaxVertexCount * VertexSize();
	bool bHasEnoughMemory = ( m_nFirstUnwrittenOffset + nMemoryRequired <= m_nBufferSize );

	D3D11_MAP map;
	if ( bAppend )
	{
		// Can't have the first lock after a flush be an appending lock
		Assert( !m_bFlush );

		// If we're appending and we don't have enough room, then puke!
		if ( !bHasEnoughMemory || m_bFlush )
			goto vertexBufferLockFailed;
		map = ( m_nFirstUnwrittenOffset == 0 ) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
	}
	else
	{
		// If we're not appending, no overwrite unless we don't have enough room
		// If we're a static buffer, always discard if we're not appending
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

	hr = D3D11DeviceContext()->Map( m_pVertexBuffer, 0, map, 0, &lockedData );
	if ( FAILED( hr ) )
	{
		Warning( "Failed to lock vertex buffer in CVertexBufferDx11::Lock\n" );
		goto vertexBufferLockFailed;
	}

	ComputeVertexDescription( (unsigned char*)lockedData.pData + m_nFirstUnwrittenOffset, m_VertexFormat, desc );
	desc.m_nFirstVertex = 0;
	desc.m_nOffset = m_nFirstUnwrittenOffset; 
	m_bIsLocked = true;
	return true;

vertexBufferLockFailed:
	g_ShaderMutex.Unlock();

	// Set up a bogus index descriptor
	ComputeVertexDescription( 0, 0, desc );
	desc.m_nFirstVertex = 0;
	desc.m_nOffset = 0; 
	return false;
}

void CVertexBufferDx11::Unlock( int nWrittenVertexCount, VertexDesc_t &desc )
{
	Assert( nWrittenVertexCount <= m_nVertexCount );

	// NOTE: This can happen if the lock occurs during alt-tab
	// or if another application is initializing
	if ( !m_bIsLocked )
		return;

	if ( m_pVertexBuffer )
	{
		D3D11DeviceContext()->Unmap( m_pVertexBuffer, 0 );
	}

	m_nFirstUnwrittenOffset += nWrittenVertexCount * VertexSize();
	m_bIsLocked = false;
	g_ShaderMutex.Unlock();
}


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
CIndexBufferDx11::CIndexBufferDx11( ShaderBufferType_t type, MaterialIndexFormat_t fmt, int nIndexCount, const char *pBudgetGroupName ) : 
	BaseClass( pBudgetGroupName )
{
	Assert( nIndexCount != 0 );
	Assert( IsDynamicBufferType( type ) || ( fmt != MATERIAL_INDEX_FORMAT_UNKNOWN ) );

	m_pIndexBuffer = NULL;
	m_IndexFormat = fmt;
	m_nIndexCount = ( fmt == MATERIAL_INDEX_FORMAT_UNKNOWN ) ? 0 : nIndexCount;
	m_nBufferSize = ( fmt == MATERIAL_INDEX_FORMAT_UNKNOWN ) ? nIndexCount : nIndexCount * IndexSize();
	m_nFirstUnwrittenOffset = 0;
	m_bIsLocked = false;
	m_bIsDynamic = IsDynamicBufferType( type );
	m_bFlush = false;

	// NOTE: This has to happen at the end since m_IndexFormat must be valid for IndexSize() to work
	if ( m_bIsDynamic )
	{
		m_IndexFormat = MATERIAL_INDEX_FORMAT_UNKNOWN;
		m_nIndexCount = 0;
	}
}

CIndexBufferDx11::~CIndexBufferDx11()
{
	Free();
}


//-----------------------------------------------------------------------------
// Creates, destroys the index buffer
//-----------------------------------------------------------------------------
bool CIndexBufferDx11::Allocate( )
{
	Assert( !m_pIndexBuffer );

	m_nFirstUnwrittenOffset = 0;

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
				COUNTER_GROUP_TEXTURE_GLOBAL, - m_nBufferSize );
		}
		else
		{
			VPROF_INCREMENT_GROUP_COUNTER( "TexGroup_global_" TEXTURE_GROUP_DYNAMIC_INDEX_BUFFER, 
				COUNTER_GROUP_TEXTURE_GLOBAL, - m_nBufferSize );
		}
	}
}


//-----------------------------------------------------------------------------
// Returns the buffer size (only valid for static index buffers)
//-----------------------------------------------------------------------------
int CIndexBufferDx11::IndexCount() const
{
	Assert( !m_bIsDynamic );
	return m_nIndexCount;
}


//-----------------------------------------------------------------------------
// Returns the buffer format (only valid for static index buffers)
//-----------------------------------------------------------------------------
MaterialIndexFormat_t CIndexBufferDx11::IndexFormat() const
{
	Assert( !m_bIsDynamic );
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
	m_nIndexCount = m_nBufferSize / IndexSize();
}

void CIndexBufferDx11::EndCastBuffer( )
{
	Assert( m_bIsDynamic && m_IndexFormat != MATERIAL_INDEX_FORMAT_UNKNOWN );
	if ( !m_bIsDynamic )
		return;
	m_IndexFormat = MATERIAL_INDEX_FORMAT_UNKNOWN;
	m_nIndexCount = 0;
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
		goto indexBufferLockFailed;
	}

	// We might not have a buffer owing to alt-tab type stuff
	if ( !m_pIndexBuffer )
	{
		if ( !Allocate() )
			goto indexBufferLockFailed;
	}

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

	hr = D3D11DeviceContext()->Map( m_pIndexBuffer, 0, map, 0, &lockedData );
	if ( FAILED( hr ) )
	{
		Warning( "Failed to lock index buffer in CIndexBufferDx11::Lock\n" );
		goto indexBufferLockFailed;
	}

	desc.m_pIndices = (unsigned short*)( (unsigned char*)lockedData.pData+ m_nFirstUnwrittenOffset );
	desc.m_nIndexSize = IndexSize() >> 1;
	desc.m_nFirstIndex = 0;
	desc.m_nOffset = m_nFirstUnwrittenOffset;
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

void CIndexBufferDx11::Unlock( int nWrittenIndexCount, IndexDesc_t &desc )
{
	Assert( nWrittenIndexCount <= m_nIndexCount );

	// NOTE: This can happen if the lock occurs during alt-tab
	// or if another application is initializing
	if ( !m_bIsLocked )
		return;

	if ( m_pIndexBuffer )
	{
		D3D11DeviceContext()->Unmap( m_pIndexBuffer, 0 );
	}

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


//-----------------------------------------------------------------------------
//
// The empty mesh...
//
//-----------------------------------------------------------------------------
CMeshDx11::CMeshDx11()
{
	m_pVertexMemory = new unsigned char[VERTEX_BUFFER_SIZE];
}

CMeshDx11::~CMeshDx11()
{
	delete[] m_pVertexMemory;
}

void CMeshDx11::LockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
	// Who cares about the data?
	desc.m_pPosition = (float*)m_pVertexMemory;
	desc.m_pNormal = (float*)m_pVertexMemory;
	desc.m_pColor = m_pVertexMemory;
	int i;
	for ( i = 0; i < VERTEX_MAX_TEXTURE_COORDINATES; ++i)
		desc.m_pTexCoord[i] = (float*)m_pVertexMemory;
	desc.m_pIndices = (unsigned short*)m_pVertexMemory;

	desc.m_pBoneWeight = (float*)m_pVertexMemory;
	desc.m_pBoneMatrixIndex = (unsigned char*)m_pVertexMemory;
	desc.m_pTangentS = (float*)m_pVertexMemory;
	desc.m_pTangentT = (float*)m_pVertexMemory;
	desc.m_pUserData = (float*)m_pVertexMemory;
	desc.m_NumBoneWeights = 2;

	desc.m_VertexSize_Position = 0;
	desc.m_VertexSize_BoneWeight = 0;
	desc.m_VertexSize_BoneMatrixIndex = 0;
	desc.m_VertexSize_Normal = 0;
	desc.m_VertexSize_Color = 0;
	for( i=0; i < VERTEX_MAX_TEXTURE_COORDINATES; i++ )
		desc.m_VertexSize_TexCoord[i] = 0;
	desc.m_VertexSize_TangentS = 0;
	desc.m_VertexSize_TangentT = 0;
	desc.m_VertexSize_UserData = 0;
	desc.m_ActualVertexSize = 0;	// Size of the vertices.. Some of the m_VertexSize_ elements above

	desc.m_nFirstVertex = 0;
	desc.m_nIndexSize = 0;
}

void CMeshDx11::UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
}

void CMeshDx11::ModifyBeginEx( bool bReadOnly, int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc )
{
	// Who cares about the data?
	desc.m_pPosition = (float*)m_pVertexMemory;
	desc.m_pNormal = (float*)m_pVertexMemory;
	desc.m_pColor = m_pVertexMemory;
	int i;
	for ( i = 0; i < VERTEX_MAX_TEXTURE_COORDINATES; ++i)
		desc.m_pTexCoord[i] = (float*)m_pVertexMemory;
	desc.m_pIndices = (unsigned short*)m_pVertexMemory;

	desc.m_pBoneWeight = (float*)m_pVertexMemory;
	desc.m_pBoneMatrixIndex = (unsigned char*)m_pVertexMemory;
	desc.m_pTangentS = (float*)m_pVertexMemory;
	desc.m_pTangentT = (float*)m_pVertexMemory;
	desc.m_pUserData = (float*)m_pVertexMemory;
	desc.m_NumBoneWeights = 2;

	desc.m_VertexSize_Position = 0;
	desc.m_VertexSize_BoneWeight = 0;
	desc.m_VertexSize_BoneMatrixIndex = 0;
	desc.m_VertexSize_Normal = 0;
	desc.m_VertexSize_Color = 0;
	for( i=0; i < VERTEX_MAX_TEXTURE_COORDINATES; i++ )
		desc.m_VertexSize_TexCoord[i] = 0;
	desc.m_VertexSize_TangentS = 0;
	desc.m_VertexSize_TangentT = 0;
	desc.m_VertexSize_UserData = 0;
	desc.m_ActualVertexSize = 0;	// Size of the vertices.. Some of the m_VertexSize_ elements above

	desc.m_nFirstVertex = 0;
	desc.m_nIndexSize = 0;
}

void CMeshDx11::ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc )
{
	ModifyBeginEx( false, firstVertex, numVerts, firstIndex, numIndices, desc );
}

void CMeshDx11::ModifyEnd( MeshDesc_t& desc )
{
}

// returns the # of vertices (static meshes only)
int CMeshDx11::VertexCount() const
{
	return 0;
}

// Sets the primitive type
void CMeshDx11::SetPrimitiveType( MaterialPrimitiveType_t type )
{
}

// Draws the entire mesh
void CMeshDx11::Draw( int firstIndex, int numIndices )
{
}

void CMeshDx11::Draw(CPrimList *pPrims, int nPrims)
{
}

// Copy verts and/or indices to a mesh builder. This only works for temp meshes!
void CMeshDx11::CopyToMeshBuilder(
								  int iStartVert,		// Which vertices to copy.
								  int nVerts, 
								  int iStartIndex,	// Which indices to copy.
								  int nIndices, 
								  int indexOffset,	// This is added to each index.
								  CMeshBuilder &builder )
{
}

// Spews the mesh data
void CMeshDx11::Spew( int numVerts, int numIndices, const MeshDesc_t & desc )
{
}

void CMeshDx11::ValidateData( int numVerts, int numIndices, const MeshDesc_t & desc )
{
}

// gets the associated material
IMaterial* CMeshDx11::GetMaterial()
{
	// umm. this don't work none
	Assert(0);
	return 0;
}
