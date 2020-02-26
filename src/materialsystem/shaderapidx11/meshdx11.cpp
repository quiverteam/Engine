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

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

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

	void SetVertexFormat( VertexFormat_t fmt );
	VertexFormat_t GetVertexFormat() const;

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

	int NumPrimitives( int nVertexCount, int nIndexCount ) const;

	// gets the associated material
	IMaterial *GetMaterial();

	void SetColorMesh( IMesh *pColorMesh, int nVertexOffset );
	void SetFlexMesh( IMesh *pMesh, int nVertexOffset );

	virtual MaterialIndexFormat_t IndexFormat() const;

	virtual void DisableFlexMesh();

	virtual void MarkAsDrawn();

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
	virtual void DrawInternal( CPrimList *pPrims, int nPrims );
	virtual void DrawMesh();

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

	// Used to keep track of the current set of primitives we are rendering.
	static CPrimList *s_pPrims;
	static int s_nPrims;
	static unsigned int s_FirstVertex; // Gets reset during CMeshDx11::DrawInternal
	static unsigned int s_NumVertices;

	int	m_FirstIndex;
};

class CMeshMgrDx11 : public IMeshMgrDx11
{
public:
	CMeshMgrDx11();
	virtual ~CMeshMgrDx11();

	virtual IMesh *GetDynamicMesh( IMaterial *pMaterial, VertexFormat_t fmt, int nHWSkinBoneCount,
					 bool buffered, IMesh *pVertexOverride, IMesh *pIndexOverride );
	virtual IVertexBuffer *GetDynamicVertexBuffer( IMaterial *pMaterial, bool buffered );
	virtual IIndexBuffer *GetDynamicIndexBuffer( IMaterial *pMaterial, bool buffered );
	virtual IMesh *GetFlexMesh();

	virtual IMesh *CreateStaticMesh( VertexFormat_t fmt, const char *pTextureBudgetGroup, IMaterial *pMaterial );
	virtual void DestroyStaticMesh( IMesh *mesh );

	virtual VertexFormat_t ComputeVertexFormat( int nFlags, int nTexCoords, int *pTexCoordDimensions,
						    int nBoneWeights, int nUserDataSize );

	virtual int VertexFormatSize( VertexFormat_t fmt );

	virtual void RenderPass( IMesh *pMesh );

public:
	CMeshDx11 m_DynamicMesh;
	CMeshDx11 m_DynamicFlexMesh;

	CVertexBufferDx11 m_DynamicVertexBuffer;
	CIndexBufferDx11 m_DynamicIndexBuffer;
};

//-----------------------------------------------------------------------------
// Singleton Dx11 Mesh manager
//-----------------------------------------------------------------------------
static CMeshMgrDx11 s_MeshMgrDx11;
IMeshMgrDx11 *MeshMgr()
{
	return &s_MeshMgrDx11;
}

CMeshMgrDx11::CMeshMgrDx11() :
	m_DynamicVertexBuffer( SHADER_BUFFER_TYPE_DYNAMIC, VERTEX_FORMAT_UNKNOWN, DYNAMIC_VERTEX_BUFFER_MEMORY, "dynamic" ),
	m_DynamicIndexBuffer( SHADER_BUFFER_TYPE_DYNAMIC, MATERIAL_INDEX_FORMAT_16BIT, INDEX_BUFFER_SIZE, "dynamic" )
{
}

CMeshMgrDx11::~CMeshMgrDx11()
{
}

void CMeshMgrDx11::RenderPass( IMesh *pMesh )
{
	static_cast<CMeshDx11 *>( pMesh )->RenderPass();
}

int CMeshMgrDx11::VertexFormatSize( VertexFormat_t fmt )
{
	return CVertexBufferBase::VertexFormatSize( fmt );
}

IMesh *CMeshMgrDx11::GetDynamicMesh( IMaterial *pMaterial, VertexFormat_t vertexFormat, int nHWSkinBoneCount,
				     bool buffered, IMesh *pVertexOverride, IMesh *pIndexOverride )
{
	// DX11FIXME
	Assert( ( pMaterial == NULL ) || ( (IMaterialInternal *)pMaterial )->IsRealTimeVersion() );

	IMaterialInternal *pMatInternal = static_cast<IMaterialInternal *>( pMaterial );

	CMeshDx11 *pMesh = NULL;
	pMesh = &m_DynamicMesh;

	if ( pMesh )
	{
		if ( !pVertexOverride )
		{
			// Remove VERTEX_FORMAT_COMPRESSED from the material's format (dynamic meshes don't
			// support compression, and all materials should support uncompressed verts too)
			VertexFormat_t materialFormat = pMatInternal->GetVertexFormat() & ~VERTEX_FORMAT_COMPRESSED;
			VertexFormat_t fmt = ( vertexFormat != 0 ) ? vertexFormat : materialFormat;
			if ( vertexFormat != 0 )
			{
				int nVertexFormatBoneWeights = NumBoneWeights( vertexFormat );
				if ( nHWSkinBoneCount < nVertexFormatBoneWeights )
				{
					nHWSkinBoneCount = nVertexFormatBoneWeights;
				}
			}

			// Force the requested number of bone weights
			fmt &= ~VERTEX_BONE_WEIGHT_MASK;
			if ( nHWSkinBoneCount > 0 )
			{
				fmt |= VERTEX_BONEWEIGHT( 2 );
				fmt |= VERTEX_BONE_INDEX;
			}

			pMesh->SetVertexFormat( fmt );
		}
		else
		{
			CMeshDx11 *pMeshVertOverride = static_cast<CMeshDx11 *>( pVertexOverride );
			pMesh->SetVertexFormat( pMeshVertOverride->GetVertexFormat() );
		}

		//pMesh->SetMorphFormat( pMatInternal->GetMorphFormat() );
		//pMesh->SetMaterial( pMatInternal );

		// Note this works because we're guaranteed to not be using a buffered mesh
		// when we have overrides on
		// FIXME: Make work for temp meshes
		//if ( pMesh == &m_DynamicMesh )
		//{
		//	CMeshDx11 *pBaseVertex = static_cast<CMeshDx11 *>( pVertexOverride );
		//	if ( pBaseVertex )
		//	{
		//		m_DynamicMesh.OverrideVertexBuffer( pBaseVertex->GetVertexBuffer() );
		//	}
		//
		//	CMeshDx11 *pBaseIndex = static_cast<CMeshDx11 *>( pIndexOverride );
		//	if ( pBaseIndex )
		//	{
		//		m_DynamicMesh.OverrideIndexBuffer( pBaseIndex->GetIndexBuffer() );
		//	}
		//}
	}
	return pMesh;
}

IVertexBuffer *CMeshMgrDx11::GetDynamicVertexBuffer( IMaterial *pMaterial, bool buffered )
{
	// DX11FIXME
	return &m_DynamicVertexBuffer;
}

IIndexBuffer *CMeshMgrDx11::GetDynamicIndexBuffer( IMaterial *pMaterial, bool buffered )
{
	// DX11FIXME
	return &m_DynamicIndexBuffer;
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
	Assert( nUserDataSize <= 4 );
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
CPrimList *CMeshDx11::s_pPrims = NULL;
int CMeshDx11::s_nPrims = 0;
unsigned int CMeshDx11::s_NumVertices = 0;
unsigned int CMeshDx11::s_FirstVertex = 0;

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

void CMeshDx11::SetVertexFormat( VertexFormat_t fmt )
{
	m_VertexFormat = fmt;
}

VertexFormat_t CMeshDx11::GetVertexFormat() const
{
	return m_VertexFormat;
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

	Log( "MeshDx11::Lock(): nVertexCount = %i, bAppend = %i\n", nVertexCount, (int)bAppend );

	if ( g_pShaderDeviceDx11->IsDeactivated() || ( nVertexCount == 0 ) )
	{
		Log( "Deactivated or no verts\n" );
		// Set up the vertex descriptor
		CVertexBufferBase::ComputeVertexDescription( 0, 0, desc );
		desc.m_nFirstVertex = 0;
		return false;
	}

	// Static vertex buffer case
	if ( !m_pVertexBuffer )
	{
		//int size = MeshMgr()->VertexFormatSize(m_VertexFormat);
		m_pVertexBuffer = static_cast<CVertexBufferDx11 *>(
			g_pShaderDeviceDx11->CreateVertexBuffer( ShaderBufferType_t::SHADER_BUFFER_TYPE_STATIC,
								 m_VertexFormat, nVertexCount, m_pTextureGroupName ) );
	}

	// Lock it baby
	int nMaxVerts, nMaxIndices;
	// DX11FIXME
	//GetMaxToRender(this, false &nMaxVerts, &nMaxIndices);
	nMaxVerts = 65535;
	nMaxIndices = 65535;
	if ( !g_pHardwareConfig->SupportsStreamOffset() )
	{
		// Without stream offset, we can't use VBs greater than 65535 verts (due to our using 16-bit indices)
		Assert( nVertexCount <= nMaxVerts );
	}

	bool ret = m_pVertexBuffer->Lock( nMaxVerts, bAppend, desc );
	if ( !ret )
	{
		Log( "Couldn't lock vertex buffer\n" );
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
	if ( m_pIndexBuffer )
	{
		m_pIndexBuffer->BeginCastBuffer( format );
	}
}

void CMeshDx11::BeginCastBuffer( VertexFormat_t format )
{
	m_VertexFormat = format;
	if ( m_pVertexBuffer )
	{
		m_pVertexBuffer->BeginCastBuffer( format );
	}
}

void CMeshDx11::EndCastBuffer()
{
	if ( m_pVertexBuffer )
	{
		m_pVertexBuffer->EndCastBuffer();
	}
	if ( m_pIndexBuffer )
	{
		m_pIndexBuffer->EndCastBuffer();
	}
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
	Log( "Locking vertex buffer\n" );
	// Lock vertex buffer
	Lock( numVerts, false, *static_cast<VertexDesc_t *>( &desc ) );
	// Lock index buffer
	Log( "Locking index buffer\n" );
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

// Draws the specified primitives on the mesh
void CMeshDx11::RenderPass()
{
	if ( !s_pPrims )
	{
		Warning( "CMeshDx11::RenderPass(): s_pPrims is NULL!\n" );
		return;
	}

	if ( m_Type == MATERIAL_HETEROGENOUS )
	{
		Warning( "CMeshDx11::RenderPass() m_Type is MATERIAL_HETEROGENOUS\n" );
		return;
	}

	// make sure the vertex format is a superset of the current material's
	// vertex format...
	//if ( !IsValidVertexFormat( g_LastVertexFormat ) )
	//{
	//	Warning( "Material %s does not support vertex format used by the mesh (maybe missing fields or mismatched vertex compression?), mesh will not be rendered. Grab a programmer!\n",
	//		 ShaderAPI()->GetBoundMaterial()->GetName() );
	//	return;
	//}

	// Set the state and transform
	g_pShaderAPIDx11->IssueStateChanges();

	// Now draw our primitives
	for ( int i = 0; i < s_nPrims; i++ )
	{
		CPrimList *pPrim = &s_pPrims[i];
		if ( pPrim->m_NumIndices == 0 )
			return;

		if ( ( m_Type == MATERIAL_POINTS ) || ( m_Type == MATERIAL_INSTANCED_QUADS ) )
		{
			// (For point/instanced-quad lists, we don't actually fill in indices, but we treat it as
			// though there are indices for the list up until here).
			g_pShaderAPIDx11->DrawNotIndexed( s_FirstVertex, pPrim->m_NumIndices );
		}
		else
		{
			//int numPrimitives = NumPrimitives( s_NumVertices, pPrim->m_NumIndices );
			g_pShaderAPIDx11->DrawIndexed( pPrim->m_FirstIndex, pPrim->m_NumIndices, 0 );
		}
	}
}

//------------------------------------------------------------------
// Mesh rendering begins here. Here's the annoyingly complex
// process:
//
// Before the Draw() function below is called, the user should've
// binded an IMaterial to the Shader API using g_pShaderAPI->Bind().
//
// So, the Shader API currently has a material to use.
//
// The user then calls the Draw() function below on the mesh they
// want to render. By default, firstIndex = -1 and numIndices = 0,
// causing the entire mesh to draw. The user can specify which
// which vertex indices on the mesh to draw.
//
// DrawInternal() is then called from Draw(), passing the list of
// primitives on the mesh to draw. Each primitive in the list is
// simply a first index and an index count. The primitive list is
// then stored statically on CMeshDx11, so we only have one list
// of primitives to render floating around.
//
// DrawInternal() then calls DrawMesh(), which binds the mesh's
// vertex buffer, index buffer, and primitive topology type to the.
// Shader API. Finally, we call DrawMesh() on the Shader API,
// passing our mesh pointer.
//
// Shader API's DrawMesh() will then store our mesh pointer, and
// call DrawMesh() on the currently bound material, which will do
// the work of setting the snapshot state, binding the constant
// buffers and shaders, and setting other dynamic states.
//
// Each pass of a material will call Shader API's RenderPass()
// function. In this function, Shader API calls RenderPass() on the
// mesh pointer that we gave it, which tells the Shader API to
// actually issue the queued up state changes to D3D, and actually
// draw the individual primitives.
//
// Done!
//------------------------------------------------------------------

// Draws a part of or the entire mesh
void CMeshDx11::Draw( int firstIndex, int numIndices )
{
	if ( !ShaderUtil()->OnDrawMesh( this, firstIndex, numIndices ) )
	{
		MarkAsDrawn();
		return;
	}

	CPrimList primList;
	if ( firstIndex == -1 || numIndices == 0 )
	{
		primList.m_FirstIndex = 0;
		primList.m_NumIndices = m_NumIndices;
	}
	else
	{
		primList.m_FirstIndex = firstIndex;
		primList.m_NumIndices = numIndices;
	}

	DrawInternal( &primList, 1 );
}

void CMeshDx11::Draw(CPrimList *pPrims, int nPrims)
{
	if ( !ShaderUtil()->OnDrawMesh( this, pPrims, nPrims ) )
	{
		MarkAsDrawn();
		return;
	}

	DrawInternal( pPrims, nPrims );
}

void CMeshDx11::DrawInternal( CPrimList *pPrims, int nPrims )
{
	Log( "CMeshDx11: DrawInternal(%p, %i)\n", pPrims, nPrims );
	// Make sure there's something to draw..
	int i;
	for ( i = 0; i < nPrims; i++ )
	{
		if ( pPrims[i].m_NumIndices > 0 )
			break;
	}
	if ( i == nPrims )
		return;

	// can't do these in selection mode!
	//Assert( !g_pShaderAPI->IsInSelectionMode() );

	s_pPrims = pPrims;
	s_nPrims = nPrims;
	s_NumVertices = m_pVertexBuffer->VertexCount();
	s_FirstVertex = 0;

	DrawMesh();
}

void CMeshDx11::DrawMesh()
{
	// Bind the mesh's buffers
	g_pShaderAPIDx11->SetTopology( m_Type );
	g_pShaderAPIDx11->BindVertexBuffer( 0, GetVertexBuffer(), 0, 0, m_NumVertices, GetVertexFormat() );
	g_pShaderAPIDx11->BindIndexBuffer( GetIndexBuffer(), 0 );

	// Draw it
	g_pShaderAPIDx11->DrawMesh( this );
}

int CMeshDx11::NumPrimitives( int nVertexCount, int nIndexCount ) const
{
	switch ( m_Type )
	{
	case MATERIAL_POINTS:
		// D3D11_PRIMITIVE_TOPOLOGY_POINTLIST
		return nVertexCount;

	case MATERIAL_LINES:
		// D3D11_PRIMITIVE_TOPOLOGY_LINELIST
		return nIndexCount / 2;

	case MATERIAL_TRIANGLES:
		// D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
		return nIndexCount / 3;

	case MATERIAL_TRIANGLE_STRIP:
		// D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
		return nIndexCount - 2;

	default:
		// Invalid type!
		Assert( 0 );
	}

	return 0;
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