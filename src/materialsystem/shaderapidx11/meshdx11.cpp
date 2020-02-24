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
#include "shaderapidx9/shaderapi_global.h"
#include "shaderapi/ishaderutil.h"
#include "shaderapi/ishaderapi.h"
#include "shaderdevicedx11.h"
#include "materialsystem/imesh.h"
#include "tier0/vprof.h"
#include "tier0/dbg.h"
#include "materialsystem/idebugtextureinfo.h"
#include "materialsystem/ivballoctracker.h"
#include "tier2/tier2.h"
#include "shaderapidx11.h"
#include "VertexBufferDx11.h"
#include "IndexBufferDx11.h"

//-----------------------------------------------------------------------------
// Dx11 implementation of a mesh
//-----------------------------------------------------------------------------
class CMeshDx11 : public CMeshBase
{
public:
	CMeshDx11( const char *pTextureGroupName = NULL );
	virtual ~CMeshDx11();

	// FIXME: Make this work! Unsupported methods of IIndexBuffer
	virtual bool Lock( int nMaxIndexCount, bool bAppend, IndexDesc_t &desc )
	{
		return false;
	}
	//virtual void Unlock( int nWrittenIndexCount, IndexDesc_t &desc )
	//{
	//}
	virtual int GetRoomRemaining() const;
	virtual void ModifyBegin( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t &desc );
	virtual void ModifyEnd( IndexDesc_t &desc );
	virtual void Spew( int nIndexCount, const IndexDesc_t &desc );
	virtual void ValidateData( int nIndexCount, const IndexDesc_t &desc );
	virtual bool Lock( int nVertexCount, bool bAppend, VertexDesc_t &desc );
	virtual void Unlock( int nVertexCount, VertexDesc_t &desc );
	virtual void Spew( int nVertexCount, const VertexDesc_t &desc );
	virtual void ValidateData( int nVertexCount, const VertexDesc_t &desc );
	virtual bool IsDynamic() const;
	virtual void BeginCastBuffer( MaterialIndexFormat_t format );
	virtual void BeginCastBuffer( VertexFormat_t format );
	virtual void EndCastBuffer();

	// Locks/unlocks the index buffer
	// Pass in nFirstIndex=-1 to lock wherever the index buffer is. Pass in a value 
	// >= 0 to specify where to lock.
	int  Lock( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t &pIndices );
	void Unlock( int nIndexCount, IndexDesc_t &desc );

	void LockMesh( int numVerts, int numIndices, MeshDesc_t &desc );
	void UnlockMesh( int numVerts, int numIndices, MeshDesc_t &desc );

	void ModifyBeginEx( bool bReadOnly, int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t &desc );
	void ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t &desc );
	void ModifyEnd( MeshDesc_t &desc );

	// returns the # of vertices (static meshes only)
	int  VertexCount() const;

	// returns the # of indices 
	virtual int IndexCount() const;

	virtual bool HasColorMesh() const;
	virtual bool IsUsingMorphData() const;
	virtual bool HasFlexMesh() const;

	// Sets up the vertex and index buffers
	void UseIndexBuffer( IIndexBuffer *pBuffer );
	void UseVertexBuffer( IVertexBuffer *pBuffer );

	// Sets the primitive type
	void SetPrimitiveType( MaterialPrimitiveType_t type );
	MaterialPrimitiveType_t GetPrimitiveType() const;

	virtual void BeginPass();
	virtual void RenderPass();
	// Draws the entire mesh
	void Draw( int firstIndex, int numIndices );
	void Draw( CPrimList *pPrims, int nPrims );

	// Copy verts and/or indices to a mesh builder. This only works for temp meshes!
	virtual void CopyToMeshBuilder(
		int iStartVert,		// Which vertices to copy.
		int nVerts,
		int iStartIndex,	// Which indices to copy.
		int nIndices,
		int indexOffset,	// This is added to each index.
		CMeshBuilder &builder );

	// Spews the mesh data
	void Spew( int numVerts, int numIndices, const MeshDesc_t &desc );

	void ValidateData( int numVerts, int numIndices, const MeshDesc_t &desc );

	// gets the associated material
	IMaterial *GetMaterial();

	void SetColorMesh( IMesh *pColorMesh, int nVertexOffset );
	void SetFlexMesh( IMesh *pMesh, int nVertexOffset );

	virtual MaterialIndexFormat_t IndexFormat() const;

	virtual void DisableFlexMesh();

	virtual void MarkAsDrawn();

	virtual VertexFormat_t GetVertexFormat() const;

	virtual IMesh *GetMesh()
	{
		return this;
	}

	CVertexBufferDx11 *GetVertexBuffer() const
	{
		return m_pVertexBuffer;
	}

	CIndexBufferDx11 *GetIndexBuffer() const
	{
		return m_pIndexBuffer;
	}

protected:
	const char *m_pTextureGroupName;

	CVertexBufferDx11 *m_pVertexBuffer;
	CIndexBufferDx11 *m_pIndexBuffer;

	CMeshDx11 *m_pColorMesh;
	int m_nColorMeshOffset;

	IVertexBuffer *m_pFlexVertexBuffer;
	bool m_bHasFlexVerts;
	int m_nFlexOffset;
	int m_flexVertCount;

	bool m_bMeshLocked;

	// The vertex format we're using...
	VertexFormat_t m_VertexFormat;

	// The morph format we're using
	MorphFormat_t m_MorphFormat;

	// Primitive type
	MaterialPrimitiveType_t m_Type;

	// Number of primitives
	unsigned short m_NumVertices;
	unsigned short m_NumIndices;

	// Is it locked?
	bool m_IsVBLocked;
	bool m_IsIBLocked;

	// Used in rendering sub-parts of the mesh
	static CPrimList *s_pPrims;
	static int s_nPrims;
	static unsigned int s_FirstVertex; // Gets reset during CMeshDX8::DrawInternal
	static unsigned int s_NumVertices;
	int	m_FirstIndex;
};

class CMeshMgrDx11 : public IMeshMgrDx11
{
public:
	virtual IMesh *GetDynamicMesh( IMaterial *pMaterial, int nHWSkinBoneCount,
				       bool buffered, IMesh *pVertexOverride, IMesh *pIndexOverride );
	virtual IMesh *GetDynamicMeshEx( IMaterial *pMaterial, VertexFormat_t fmt, int nHWSkinBoneCount,
					 bool buffered, IMesh *pVertexOverride, IMesh *pIndexOverride );
	virtual IVertexBuffer *GetDynamicVertexBuffer( IMaterial *pMaterial, bool buffered );
	virtual IIndexBuffer *GetDynamicIndexBuffer( IMaterial *pMaterial, bool buffered );
	virtual IMesh *GetFlexMesh();

	virtual IMesh *CreateStaticMesh( VertexFormat_t fmt, const char *pTextureBudgetGroup, IMaterial *pMaterial );
	virtual void DestroyStaticMesh( IMesh *mesh );

	virtual VertexFormat_t ComputeVertexFormat( int nFlags, int nTexCoords, int *pTexCoordDimensions,
						    int nBoneWeights, int nUserDataSize );

public:
	CMeshDx11 m_DynamicMesh;
	CMeshDx11 m_DynamicFlexMesh;

	

};

//-----------------------------------------------------------------------------
// Singleton Dx11 Mesh manager
//-----------------------------------------------------------------------------
static CMeshMgrDx11 s_MeshMgrDx11;
IMeshMgrDx11 *MeshMgr()
{
	return &s_MeshMgrDx11;
}

IMesh *CMeshMgrDx11::GetDynamicMesh( IMaterial *pMaterial, int nHWSkinBoneCount,
				     bool buffered, IMesh *pVertexOverride, IMesh *pIndexOverride )
{
	// DX11FIXME
	Assert( ( pMaterial == NULL ) || ( (IMaterialInternal *)pMaterial )->IsRealTimeVersion() );
	return &m_DynamicMesh;
}

IMesh *CMeshMgrDx11::GetDynamicMeshEx( IMaterial *pMaterial, VertexFormat_t fmt, int nHWSkinBoneCount, bool buffered, IMesh *pVertexOverride, IMesh *pIndexOverride )
{
	// DX11FIXME
	// UNDONE: support compressed dynamic meshes if needed (pro: less VB memory, con: time spent compressing)
	Assert( CompressionType( pVertexOverride->GetVertexFormat() ) != VERTEX_COMPRESSION_NONE );
	Assert( ( pMaterial == NULL ) || ( (IMaterialInternal *)pMaterial )->IsRealTimeVersion() );
	return &m_DynamicMesh;
}

IVertexBuffer *CMeshMgrDx11::GetDynamicVertexBuffer( IMaterial *pMaterial, bool buffered )
{
	// DX11FIXME
	return m_DynamicMesh.GetVertexBuffer();
}

IIndexBuffer *CMeshMgrDx11::GetDynamicIndexBuffer( IMaterial *pMaterial, bool buffered )
{
	// DX11FIXME
	return m_DynamicMesh.GetIndexBuffer();
}

IMesh *CMeshMgrDx11::GetFlexMesh()
{
	// DX11FIXME
	return &m_DynamicFlexMesh;
}

IMesh *CMeshMgrDx11::CreateStaticMesh( VertexFormat_t fmt, const char *pTextureBudgetGroup, IMaterial *pMaterial )
{
	// DX11FIXME
	CMeshDx11 *mesh = new CMeshDx11( pTextureBudgetGroup );
	return mesh;
}

void CMeshMgrDx11::DestroyStaticMesh( IMesh *mesh )
{
	// DX11FIXME
	delete mesh;
}

VertexFormat_t CMeshMgrDx11::ComputeVertexFormat( int nFlags, int nTexCoords, int *pTexCoordDimensions,
						  int nBoneWeights, int nUserDataSize )
{
	// Construct a bitfield that makes sense and is unique from the standard FVF formats
	VertexFormat_t fmt = nFlags & ~VERTEX_FORMAT_USE_EXACT_FORMAT;

	if ( g_pHardwareConfig->SupportsCompressedVertices() == VERTEX_COMPRESSION_NONE )
	{
		// Vertex compression is disabled - make sure all materials
		// say "No!" to compressed verts ( tested in IsValidVertexFormat() )
		fmt &= ~VERTEX_FORMAT_COMPRESSED;
	}

	// This'll take 3 bits at most
	Assert( nBoneWeights <= 4 );

	// Size is measured in # of floats
	Assert( userDataSize <= 4 );
	fmt |= VERTEX_USERDATA_SIZE( nUserDataSize );

	// NOTE: If pTexCoordDimensions isn't specified, then nTexCoordArraySize
	// is interpreted as meaning that we have n 2D texcoords in the first N texcoord slots
	nTexCoords = min( nTexCoords, VERTEX_MAX_TEXTURE_COORDINATES );
	for ( int i = 0; i < nTexCoords; ++i )
	{
		if ( pTexCoordDimensions )
		{
			Assert( pTexCoordDimensions[i] >= 0 && pTexCoordDimensions[i] <= 4 );
			fmt |= VERTEX_TEXCOORD_SIZE( (TextureStage_t)i, pTexCoordDimensions[i] );
		}
		else
		{
			fmt |= VERTEX_TEXCOORD_SIZE( (TextureStage_t)i, 2 );
		}
	}

	return fmt;
}

//-----------------------------------------------------------------------------
//
// The empty mesh...
//
//-----------------------------------------------------------------------------
CMeshDx11::CMeshDx11( const char *pTextureGroupName ) :
	m_NumVertices( 0 ),
	m_NumIndices( 0 ),
	m_pVertexBuffer( 0 ),
	m_pColorMesh( 0 ),
	m_nColorMeshOffset( 0 ),
	m_pIndexBuffer( 0 ),
	m_Type( MATERIAL_TRIANGLES ),
	m_IsVBLocked( false ),
	m_IsIBLocked( false ),
	m_pTextureGroupName( pTextureGroupName ),
	m_bHasFlexVerts( false ),
	m_pFlexVertexBuffer( NULL ),
	m_nFlexOffset( 0 )
{
}

CMeshDx11::~CMeshDx11()
{
}

void CMeshDx11::SetFlexMesh( IMesh *pMesh, int nVertexOffsetInBytes )
{
	if ( !ShaderUtil()->OnSetFlexMesh( this, pMesh, nVertexOffsetInBytes ) )
		return;

	LOCK_SHADERAPI();
	m_nFlexOffset = nVertexOffsetInBytes;

	if ( pMesh )
	{
		m_flexVertCount = pMesh->VertexCount();
		pMesh->MarkAsDrawn();

		CMeshDx11 *pMeshDx11 = static_cast<CMeshDx11 *>( pMesh );
		m_pFlexVertexBuffer = pMeshDx11->GetVertexBuffer();

		m_bHasFlexVerts = true;
	}
	else
	{
		m_flexVertCount = 0;
		m_pFlexVertexBuffer = NULL;
		m_bHasFlexVerts = false;
	}
}

MaterialIndexFormat_t CMeshDx11::IndexFormat() const
{
	return MaterialIndexFormat_t();
}

void CMeshDx11::DisableFlexMesh()
{
	SetFlexMesh( NULL, 0 );
}

void CMeshDx11::MarkAsDrawn()
{
}

VertexFormat_t CMeshDx11::GetVertexFormat() const
{
	return VertexFormat_t();
}

bool CMeshDx11::HasFlexMesh() const
{
	LOCK_SHADERAPI();
	return m_bHasFlexVerts;
}

void CMeshDx11::SetColorMesh( IMesh *pColorMesh, int nVertexOffsetInBytes )
{
	if ( !ShaderUtil()->OnSetColorMesh( this, pColorMesh, nVertexOffsetInBytes ) )
		return;

	LOCK_SHADERAPI();
	m_pColorMesh = (CMeshDx11 *)pColorMesh; // dangerous conversion! garymcthack
	m_nColorMeshOffset = nVertexOffsetInBytes;
	Assert( m_pColorMesh || ( nVertexOffsetInBytes == 0 ) );

#ifdef _DEBUG
	if ( pColorMesh )
	{
		int nVertexCount = VertexCount();
		int numVertsColorMesh = m_pColorMesh->VertexCount();
		Assert( numVertsColorMesh >= nVertexCount );
	}
#endif
}

bool CMeshDx11::HasColorMesh() const
{
	LOCK_SHADERAPI();
	return ( m_pColorMesh != NULL );
}

bool CMeshDx11::IsUsingMorphData() const
{
	return false;
}

int CMeshDx11::GetRoomRemaining() const
{
	return 0;
}

void CMeshDx11::ModifyBegin( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t &desc )
{
}

void CMeshDx11::ModifyEnd( IndexDesc_t &desc )
{
}

void CMeshDx11::Spew( int nIndexCount, const IndexDesc_t &desc )
{
}

void CMeshDx11::ValidateData( int nIndexCount, const IndexDesc_t &desc )
{
}

bool CMeshDx11::Lock( int nVertexCount, bool bAppend, VertexDesc_t &desc )
{
	Assert( !m_IsVBLocked );

	if ( g_pShaderDeviceDx11->IsDeactivated() || ( nVertexCount == 0 ) )
	{
		// Set up the vertex descriptor
		CVertexBufferBase::ComputeVertexDescription( 0, 0, desc );
		desc.m_nFirstVertex = 0;
		return false;
	}

	// Static vertex buffer case
	if ( !m_pVertexBuffer )
	{
		int size = 0;//VertexFormatSize(m_VertexFormat);
		m_pVertexBuffer = static_cast<CVertexBufferDx11 *>(
			g_pShaderDeviceDx11->CreateVertexBuffer( ShaderBufferType_t::SHADER_BUFFER_TYPE_STATIC,
								 m_VertexFormat, size, m_pTextureGroupName ) );
	}

	// Lock it baby
	int nMaxVerts, nMaxIndices;
	//GetMaxToRender(this, false &nMaxVerts, &nMaxIndices);
	if ( !g_pHardwareConfig->SupportsStreamOffset() )
	{
		// Without stream offset, we can't use VBs greater than 65535 verts (due to our using 16-bit indices)
		Assert( nVertexCount <= nMaxVerts );
	}

	bool ret = m_pVertexBuffer->Lock( nMaxVerts, bAppend, desc );
	if ( !ret )
	{
		if ( nVertexCount > nMaxVerts )
		{
			Assert( 0 );
			Error( "Too many verts for a dynamic vertex buffer (%d>%d) Tell a programmer to up VERTEX_BUFFER_SIZE.\n",
				(int)nVertexCount, (int)nMaxVerts );
		}
		return false;
	}

	return true;
}

void CMeshDx11::Unlock( int nVertexCount, VertexDesc_t &desc )
{
	// NOTE: This can happen if another application finishes
	// initializing during the construction of a mesh
	if ( !m_IsVBLocked )
		return;

	Assert( m_pVertexBuffer );
	m_pVertexBuffer->Unlock( nVertexCount, desc );
	m_IsVBLocked = false;
}

void CMeshDx11::Spew( int nVertexCount, const VertexDesc_t &desc )
{
}

void CMeshDx11::ValidateData( int nVertexCount, const VertexDesc_t &desc )
{
}

bool CMeshDx11::IsDynamic() const
{
	return false;
}

void CMeshDx11::BeginCastBuffer( MaterialIndexFormat_t format )
{
}

void CMeshDx11::BeginCastBuffer( VertexFormat_t format )
{
}

void CMeshDx11::EndCastBuffer()
{
}

//-----------------------------------------------------------------------------
// Locks/unlocks the index buffer
//-----------------------------------------------------------------------------
int CMeshDx11::Lock( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t &desc )
{
	Assert( !m_IsIBLocked );

	if ( !m_pIndexBuffer )
	{
		m_pIndexBuffer = static_cast<CIndexBufferDx11 *>(
			g_pShaderDeviceDx11->CreateIndexBuffer( SHADER_BUFFER_TYPE_DYNAMIC,
								MATERIAL_INDEX_FORMAT_16BIT,
								nIndexCount, m_pTextureGroupName ) );
	}

	bool ret = m_pIndexBuffer->Lock( nIndexCount, false/*bAppend*/, desc );
	if ( !ret )
	{
		return false;
	}
	m_IsIBLocked = true;
	return true;
}

void CMeshDx11::Unlock( int nIndexCount, IndexDesc_t &desc )
{
	// NOTE: This can happen if another application finishes
	// initializing during the construction of a mesh
	if ( !m_IsIBLocked )
		return;

	Assert( m_pIndexBuffer );

	// Unlock, and indicate how many vertices we actually used
	m_pIndexBuffer->Unlock( nIndexCount, desc );
	m_IsIBLocked = false;
}

void CMeshDx11::LockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
	ShaderUtil()->SyncMatrices();

	g_ShaderMutex.Lock();
	VPROF( "CMeshDx11::LockMesh" );
	// Lock vertex buffer
	Lock( numVerts, false, *static_cast<VertexDesc_t *>( &desc ) );
	// Lock index buffer
	Lock( false, -1, numIndices, *static_cast<IndexDesc_t *>( &desc ) );
	m_bMeshLocked = true;
}

void CMeshDx11::UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc )
{
	VPROF( "CMeshDx11::UnlockMesh" );

	Assert( m_bMeshLocked );

	Unlock( numVerts, *static_cast<VertexDesc_t *>( &desc ) );
	//if ( m_Type != MATERIAL_POINTS )
	//{
		Unlock( numIndices, *static_cast<IndexDesc_t *>( &desc ) );
	//}

	// The actual # we wrote
	m_NumVertices = numVerts;
	m_NumIndices = numIndices;

	m_bMeshLocked = false;
	g_ShaderMutex.Unlock();
}

void CMeshDx11::ModifyBeginEx( bool bReadOnly, int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc )
{
	VPROF( "CMeshDX8::ModifyBegin" );

	// Just give the app crap buffers to fill up while we're suppressed...

	Assert( m_pVertexBuffer );

	// Lock it baby
	bool pVertexMemory = Lock( bReadOnly, firstIndex, numIndices, *static_cast<IndexDesc_t *>( &desc ) );
	if ( pVertexMemory )
	{
		m_IsVBLocked = true;
	}

	desc.m_nFirstVertex = firstVertex;

	Lock( bReadOnly, firstIndex, numIndices, *static_cast<IndexDesc_t *>( &desc ) );
}

void CMeshDx11::ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc )
{
	ModifyBeginEx( false, firstVertex, numVerts, firstIndex, numIndices, desc );
}

void CMeshDx11::ModifyEnd( MeshDesc_t& desc )
{
	VPROF( "CMeshDx11::ModifyEnd" );
	Unlock( 0, *static_cast<IndexDesc_t *>( &desc ) );
	Unlock( 0, *static_cast<VertexDesc_t *>( &desc ) );
}

// returns the # of vertices (static meshes only)
int CMeshDx11::VertexCount() const
{
	return m_pVertexBuffer ? m_pVertexBuffer->VertexCount() : 0;
}

//-----------------------------------------------------------------------------
// returns the # of indices 
//-----------------------------------------------------------------------------
int CMeshDx11::IndexCount() const
{
	return m_pIndexBuffer ? m_pIndexBuffer->IndexCount() : 0;
}

//-----------------------------------------------------------------------------
// Sets up the vertex and index buffers
//-----------------------------------------------------------------------------
void CMeshDx11::UseIndexBuffer( IIndexBuffer *pBuffer )
{
	m_pIndexBuffer = static_cast<CIndexBufferDx11 *>( pBuffer );
}

void CMeshDx11::UseVertexBuffer( IVertexBuffer *pBuffer )
{
	m_pVertexBuffer = static_cast<CVertexBufferDx11 *>( pBuffer );;
}

// Sets the primitive type
void CMeshDx11::SetPrimitiveType( MaterialPrimitiveType_t type )
{
	Assert( IsX360() || ( type != MATERIAL_INSTANCED_QUADS ) );
	if ( !ShaderUtil()->OnSetPrimitiveType( this, type ) )
	{
		return;
	}
	m_Type = type;
}

MaterialPrimitiveType_t CMeshDx11::GetPrimitiveType() const
{
	return m_Type;
}

void CMeshDx11::BeginPass()
{
}

void CMeshDx11::RenderPass()
{
}

// Draws the entire mesh
void CMeshDx11::Draw( int firstIndex, int numIndices )
{
	g_pShaderAPI->Draw( m_Type, firstIndex, numIndices );
}

void CMeshDx11::Draw(CPrimList *pPrims, int nPrims)
{
	for ( int i = 0; i < nPrims; i++ )
	{
		CPrimList *prim = &pPrims[i];
		g_pShaderAPI->Draw( m_Type, prim->m_FirstIndex, prim->m_NumIndices );
	}
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