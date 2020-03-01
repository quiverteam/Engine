//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include <d3d11.h>
#undef GetCommandLine

#include "materialsystem/imesh.h"
#include "meshdx11.h"
#include "utlvector.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/materialsystem_config.h"
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
#include "shaderapi/ishaderutil.h"

#ifdef _DEBUG
//#define CHECK_INDICES
#endif

//-----------------------------------------------------------------------------
// Important enumerations
//-----------------------------------------------------------------------------
enum
{
	VERTEX_BUFFER_SIZE = 32768,
	MAX_QUAD_INDICES = 16384,
};


// this is hooked into the engines convar
extern ConVar mat_debugalttab;

CShaderAPIDx11 *ShaderAPI()
{
	return g_pShaderAPIDx11;
}

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

//------------------------------------------------------------------
//
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
//------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Helpers with meshdescs...
//-----------------------------------------------------------------------------
// FIXME: add compression-agnostic read-accessors (which decompress and return by value, checking desc.m_CompressionType)
inline DirectX::XMFLOAT3 &Position( MeshDesc_t const &desc, int vert )
{
	return *( DirectX::XMFLOAT3 *)( (unsigned char *)desc.m_pPosition + vert * desc.m_VertexSize_Position );
}

inline float Wrinkle( MeshDesc_t const &desc, int vert )
{
	return *(float *)( (unsigned char *)desc.m_pWrinkle + vert * desc.m_VertexSize_Wrinkle );
}

inline DirectX::XMFLOAT3 &BoneWeight( MeshDesc_t const &desc, int vert )
{
	Assert( desc.m_CompressionType == VERTEX_COMPRESSION_NONE );
	return *( DirectX::XMFLOAT3 *)( (unsigned char *)desc.m_pBoneWeight + vert * desc.m_VertexSize_BoneWeight );
}

inline unsigned char *BoneIndex( MeshDesc_t const &desc, int vert )
{
	return desc.m_pBoneMatrixIndex + vert * desc.m_VertexSize_BoneMatrixIndex;
}

inline DirectX::XMFLOAT3 &Normal( MeshDesc_t const &desc, int vert )
{
	Assert( desc.m_CompressionType == VERTEX_COMPRESSION_NONE );
	return *( DirectX::XMFLOAT3 *)( (unsigned char *)desc.m_pNormal + vert * desc.m_VertexSize_Normal );
}

inline unsigned char *Color( MeshDesc_t const &desc, int vert )
{
	return desc.m_pColor + vert * desc.m_VertexSize_Color;
}

inline DirectX::XMFLOAT2 &TexCoord( MeshDesc_t const &desc, int vert, int stage )
{
	return *( DirectX::XMFLOAT2 *)( (unsigned char *)desc.m_pTexCoord[stage] + vert * desc.m_VertexSize_TexCoord[stage] );
}

inline DirectX::XMFLOAT3 &TangentS( MeshDesc_t const &desc, int vert )
{
	return *( DirectX::XMFLOAT3 *)( (unsigned char *)desc.m_pTangentS + vert * desc.m_VertexSize_TangentS );
}

inline DirectX::XMFLOAT3 &TangentT( MeshDesc_t const &desc, int vert )
{
	return *( DirectX::XMFLOAT3 *)( (unsigned char *)desc.m_pTangentT + vert * desc.m_VertexSize_TangentT );
}

//-----------------------------------------------------------------------------
// Base class for the different mesh types.
//-----------------------------------------------------------------------------
class CBaseMeshDX11 : public CMeshBase
{
public:
	// constructor, destructor
	CBaseMeshDX11();
	virtual ~CBaseMeshDX11();

	// FIXME: Make this work! Unsupported methods of IIndexBuffer + IVertexBuffer
	virtual bool Lock( int nMaxIndexCount, bool bAppend, IndexDesc_t &desc )
	{
		Assert( 0 ); return false;
	}
	virtual void Unlock( int nWrittenIndexCount, IndexDesc_t &desc )
	{
		Assert( 0 );
	}
	virtual void ModifyBegin( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t &desc )
	{
		Assert( 0 );
	}
	virtual void ModifyEnd( IndexDesc_t &desc )
	{
		Assert( 0 );
	}
	virtual void Spew( int nIndexCount, const IndexDesc_t &desc )
	{
		Assert( 0 );
	}
	virtual void ValidateData( int nIndexCount, const IndexDesc_t &desc )
	{
		Assert( 0 );
	}
	virtual bool Lock( int nVertexCount, bool bAppend, VertexDesc_t &desc )
	{
		Assert( 0 ); return false;
	}
	virtual void Unlock( int nVertexCount, VertexDesc_t &desc )
	{
		Assert( 0 );
	}
	virtual void Spew( int nVertexCount, const VertexDesc_t &desc )
	{
		Assert( 0 );
	}
	virtual void ValidateData( int nVertexCount, const VertexDesc_t &desc )
	{
		Assert( 0 );
	}

	// Locks mesh for modifying
	void ModifyBeginEx( bool bReadOnly, int nFirstVertex, int nVertexCount, int nFirstIndex, int nIndexCount, MeshDesc_t &desc );
	void ModifyBegin( int nFirstVertex, int nVertexCount, int nFirstIndex, int nIndexCount, MeshDesc_t &desc );
	void ModifyEnd( MeshDesc_t &desc );

	// Sets/gets the vertex format
	virtual void SetVertexFormat( VertexFormat_t format );
	virtual VertexFormat_t GetVertexFormat() const;

	// Sets/gets the morph format
	virtual void SetMorphFormat( MorphFormat_t format );
	virtual MorphFormat_t GetMorphFormat() const;
	// Am I using morph data?
	virtual bool IsUsingMorphData() const;
	bool IsUsingVertexID() const
	{
		return g_pShaderAPIDx11->GetBoundMaterial()->IsUsingVertexID();
	}

	// Sets the material
	virtual void SetMaterial( IMaterial *pMaterial );

	// returns the # of vertices (static meshes only)
	int VertexCount() const
	{
		return 0;
	}

	void SetColorMesh( IMesh *pColorMesh, int nVertexOffsetInBytes )
	{
		Assert( 0 );
	}

	void SetFlexMesh( IMesh *pMesh, int nVertexOffsetInBytes )
	{
		Assert( pMesh == NULL && nVertexOffsetInBytes == 0 );
	}

	void DisableFlexMesh()
	{
		Assert( 0 );
	}

	void MarkAsDrawn()
	{
	}

	bool HasColorMesh() const
	{
		return false;
	}
	bool HasFlexMesh() const
	{
		return false;
	}

	// Draws the mesh
	void DrawMesh();

	// Begins a pass
	void BeginPass();

	// Spews the mesh data
	virtual void Spew( int nVertexCount, int nIndexCount, const MeshDesc_t &desc );

	// Call this in debug mode to make sure our data is good.
	virtual void ValidateData( int nVertexCount, int nIndexCount, const MeshDesc_t &desc );

	void Draw( CPrimList *pLists, int nLists );

	// Copy verts and/or indices to a mesh builder. This only works for temp meshes!
	virtual void CopyToMeshBuilder(
		int iStartVert,		// Which vertices to copy.
		int nVerts,
		int iStartIndex,	// Which indices to copy.
		int nIndices,
		int indexOffset,	// This is added to each index.
		CMeshBuilder &builder );

	// returns the primitive type
	virtual MaterialPrimitiveType_t GetPrimitiveType() const = 0;

	// Returns the number of indices in a mesh..
	virtual int IndexCount() const = 0;

	// FIXME: Make this work!
	virtual MaterialIndexFormat_t IndexFormat() const
	{
		return MATERIAL_INDEX_FORMAT_16BIT;
	}

	// NOTE: For dynamic index buffers only!
	// Casts the memory of the dynamic index buffer to the appropriate type
	virtual void BeginCastBuffer( MaterialIndexFormat_t format )
	{
		Assert( 0 );
	}
	virtual void BeginCastBuffer( VertexFormat_t format )
	{
		Assert( 0 );
	}
	virtual void EndCastBuffer()
	{
		Assert( 0 );
	}
	virtual int GetRoomRemaining() const
	{
		Assert( 0 ); return 0;
	}

	// returns a static vertex buffer...
	virtual CVertexBufferDx11 *GetVertexBuffer()
	{
		return 0;
	}
	virtual CIndexBufferDx11 *GetIndexBuffer()
	{
		return 0;
	}

	// Do I need to reset the vertex format?
	virtual bool NeedsVertexFormatReset( VertexFormat_t fmt ) const;

	// Do I have enough room?
	virtual bool HasEnoughRoom( int nVertexCount, int nIndexCount ) const;

	// Operation to do pre-lock
	virtual void PreLock()
	{
	}

	bool m_bMeshLocked;

protected:
	bool DebugTrace() const;

	// The vertex format we're using...
	VertexFormat_t m_VertexFormat;

	// The morph format we're using
	MorphFormat_t m_MorphFormat;

#ifdef _DEBUG
	IMaterialInternal *m_pMaterial;
	bool m_IsDrawing;
#endif
};

//-----------------------------------------------------------------------------
// Implementation of the mesh
//-----------------------------------------------------------------------------
class CMeshDX11 : public CBaseMeshDX11
{
public:
	// constructor
	CMeshDX11( const char *pTextureGroupName );
	virtual ~CMeshDX11();

	// Locks/unlocks the mesh
	void LockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc );
	void UnlockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc );

	// Locks mesh for modifying
	void ModifyBeginEx( bool bReadOnly, int nFirstVertex, int nVertexCount, int nFirstIndex, int nIndexCount, MeshDesc_t &desc );
	void ModifyBegin( int nFirstVertex, int nVertexCount, int nFirstIndex, int nIndexCount, MeshDesc_t &desc );
	void ModifyEnd( MeshDesc_t &desc );

	// returns the # of vertices (static meshes only)
	int VertexCount() const;

	// returns the # of indices 
	virtual int IndexCount() const;

	// Sets up the vertex and index buffers
	void UseIndexBuffer( CIndexBufferDx11 *pBuffer );
	void UseVertexBuffer( CVertexBufferDx11 *pBuffer );

	// returns a static vertex buffer...
	CVertexBufferDx11 *GetVertexBuffer()
	{
		return m_pVertexBuffer;
	}
	CIndexBufferDx11 *GetIndexBuffer()
	{
		return m_pIndexBuffer;
	}

	void SetColorMesh( IMesh *pColorMesh, int nVertexOffsetInBytes );
	void SetFlexMesh( IMesh *pMesh, int nVertexOffsetInBytes );
	void DisableFlexMesh();

	bool HasColorMesh() const;
	bool HasFlexMesh() const;

	// Draws the mesh
	void Draw( int nFirstIndex, int nIndexCount );
	void Draw( CPrimList *pLists, int nLists );
	void DrawInternal( CPrimList *pLists, int nLists );

	// Draws a single pass
	void RenderPass();

	// Sets the primitive type
	void SetPrimitiveType( MaterialPrimitiveType_t type );
	MaterialPrimitiveType_t GetPrimitiveType() const;

	bool IsDynamic() const
	{
		return false;
	}

	void CheckIndices( CPrimList *pPrim, int numPrimitives );

protected:
	// Sets the render state.
	bool SetRenderState( int nVertexOffsetInBytes, int nFirstVertexIdx, VertexFormat_t vertexFormat = VERTEX_FORMAT_INVALID );

	// Is the vertex format valid?
	bool IsValidVertexFormat( VertexFormat_t vertexFormat = VERTEX_FORMAT_INVALID );

	// Locks/ unlocks the vertex buffer
	bool Lock( int nVertexCount, bool bAppend, VertexDesc_t &desc );
	void Unlock( int nVertexCount, VertexDesc_t &desc );

	// Locks/unlocks the index buffer
	// Pass in nFirstIndex=-1 to lock wherever the index buffer is. Pass in a value 
	// >= 0 to specify where to lock.
	int  Lock( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t &pIndices );
	void Unlock( int nIndexCount, IndexDesc_t &desc );

	// computes how many primitives we've got
	int NumPrimitives( int nVertexCount, int nIndexCount ) const;

	// The vertex and index buffers
	CVertexBufferDx11 *m_pVertexBuffer;
	CIndexBufferDx11 *m_pIndexBuffer;

	// The current color mesh (to be bound to stream 1)
	// The vertex offset allows use of a global, shared color mesh VB
	CMeshDX11 *m_pColorMesh;
	int			m_nColorMeshVertOffsetInBytes;

	CVertexBufferDx11 *m_pFlexVertexBuffer;

	bool   m_bHasFlexVerts;
	int	   m_nFlexVertOffsetInBytes;
	int m_flexVertCount;

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

#ifdef RECORDING
	int	m_LockVertexBufferSize;
	void *m_LockVertexBuffer;
#endif

#if defined( RECORDING ) || defined( CHECK_INDICES )
	void *m_LockIndexBuffer;
	int	m_LockIndexBufferSize;
#endif
	const char *m_pTextureGroupName;

	friend class CMeshMgr; // MESHFIXME
};

//-----------------------------------------------------------------------------
// A little extra stuff for the dynamic version
//-----------------------------------------------------------------------------
class CDynamicMeshDX11 : public CMeshDX11
{
public:
	// constructor, destructor
	CDynamicMeshDX11();
	virtual ~CDynamicMeshDX11();

	// Initializes the dynamic mesh
	void Init( int nBufferId );

	// Sets the vertex format
	virtual void SetVertexFormat( VertexFormat_t format );

	// Resets the state in case of a task switch
	void Reset();

	// Do I have enough room in the buffer?
	bool HasEnoughRoom( int nVertexCount, int nIndexCount ) const;

	// returns the # of indices
	int IndexCount() const;

	// Locks the mesh
	void LockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc );

	// Unlocks the mesh
	void UnlockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc );

	// Override vertex + index buffer
	void OverrideVertexBuffer( CVertexBufferDx11 *pStaticVertexBuffer );
	void OverrideIndexBuffer( CIndexBufferDx11 *pStaticIndexBuffer );

	// Do I need to reset the vertex format?
	bool NeedsVertexFormatReset( VertexFormat_t fmt ) const;

	// Draws it						   
	void Draw( int nFirstIndex, int nIndexCount );
	void MarkAsDrawn()
	{
		m_HasDrawn = true;
	}
	// Simply draws what's been buffered up immediately, without state change 
	void DrawSinglePassImmediately();

	// Operation to do pre-lock
	void PreLock();

	bool IsDynamic() const
	{
		return true;
	}

private:
	// Resets buffering state
	void ResetVertexAndIndexCounts();

	// Buffer Id
	int m_nBufferId;

	// total queued vertices
	int m_TotalVertices;
	int	m_TotalIndices;

	// the first vertex and index since the last draw
	int m_nFirstVertex;
	int m_FirstIndex;

	// Have we drawn since the last lock?
	bool m_HasDrawn;

	// Any overrides?
	bool m_VertexOverride;
	bool m_IndexOverride;
};


//-----------------------------------------------------------------------------
// A mesh that stores temporary vertex data in the correct format (for modification)
//-----------------------------------------------------------------------------
class CTempMeshDX11 : public CBaseMeshDX11
{
public:
	// constructor, destructor
	CTempMeshDX11( bool isDynamic );
	virtual ~CTempMeshDX11();

	// Sets the material
	virtual void SetVertexFormat( VertexFormat_t format );

	// Locks/unlocks the mesh
	void LockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc );
	void UnlockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc );

	// Locks mesh for modifying
	virtual void ModifyBeginEx( bool bReadOnly, int nFirstVertex, int nVertexCount, int nFirstIndex, int nIndexCount, MeshDesc_t &desc );
	virtual void ModifyBegin( int nFirstVertex, int nVertexCount, int nFirstIndex, int nIndexCount, MeshDesc_t &desc );
	virtual void ModifyEnd( MeshDesc_t &desc );

	// Number of indices + vertices
	int VertexCount() const;
	virtual int IndexCount() const;
	virtual bool IsDynamic() const;

	// Sets the primitive type
	void SetPrimitiveType( MaterialPrimitiveType_t type );
	MaterialPrimitiveType_t GetPrimitiveType() const;

	// Begins a pass
	void BeginPass();

	// Draws a single pass
	void RenderPass();

	// Draws the entire beast
	void Draw( int nFirstIndex, int nIndexCount );

	virtual void CopyToMeshBuilder(
		int iStartVert,		// Which vertices to copy.
		int nVerts,
		int iStartIndex,	// Which indices to copy.
		int nIndices,
		int indexOffset,	// This is added to each index.
		CMeshBuilder &builder );
private:
	// Selection mode 
	void TestSelection();
	void ClipTriangle( DirectX::XMVECTOR **ppVert, float zNear, DirectX::XMMATRIX &proj );

	CDynamicMeshDX11 *GetDynamicMesh();

	CUtlVector< unsigned char, CUtlMemoryAligned< unsigned char, 32 > > m_VertexData;
	CUtlVector< unsigned short > m_IndexData;

	unsigned short m_VertexSize;
	MaterialPrimitiveType_t m_Type;
	int m_LockedVerts;
	int m_LockedIndices;
	bool m_IsDynamic;

	// Used in rendering sub-parts of the mesh
	static unsigned int s_NumIndices;
	static unsigned int s_FirstIndex;

#ifdef _DEBUG
	bool m_Locked;
	bool m_InPass;
#endif
};


//-----------------------------------------------------------------------------
// This is a version that buffers up vertex data so we can blast through it later
//-----------------------------------------------------------------------------
class CBufferedMeshDX11 : public CBaseMeshDX11
{
public:
	// constructor, destructor
	CBufferedMeshDX11();
	virtual ~CBufferedMeshDX11();

	// checks to see if it was rendered..
	void ResetRendered();
	bool WasNotRendered() const;

	// Sets the mesh we're really going to draw into
	void SetMesh( CBaseMeshDX11 *pMesh );
	const CBaseMeshDX11 *GetMesh() const
	{
		return m_pMesh;
	}

// Spews the mesh data
	virtual void Spew( int nVertexCount, int nIndexCount, const MeshDesc_t &spewDesc );

	// Sets the vertex format
	virtual void SetVertexFormat( VertexFormat_t format );
	virtual VertexFormat_t GetVertexFormat() const;

	// Sets the morph format
	virtual void SetMorphFormat( MorphFormat_t format );

	// Sets the material
	void SetMaterial( IMaterial *pMaterial );

	// returns the number of indices (should never be called!)
	virtual int IndexCount() const
	{
		Assert( 0 ); return 0;
	}
	virtual MaterialIndexFormat_t IndexFormat() const
	{
		Assert( 0 ); return MATERIAL_INDEX_FORMAT_16BIT;
	}
	virtual bool IsDynamic() const
	{
		Assert( 0 ); return true;
	}
	virtual void BeginCastBuffer( MaterialIndexFormat_t format )
	{
		Assert( 0 );
	}
	virtual void EndCastBuffer()
	{
		Assert( 0 );
	}
	virtual int GetRoomRemaining() const
	{
		Assert( 0 ); return 0;
	}

	// Locks the mesh
	void LockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc );
	void UnlockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc );

	// Sets the primitive type
	void SetPrimitiveType( MaterialPrimitiveType_t type );
	MaterialPrimitiveType_t GetPrimitiveType() const;

	void ValidateData( int nVertexCount, int nIndexCount, const MeshDesc_t &spewDesc );

	// Draws it
	void Draw( int nFirstIndex, int nIndexCount );

	// Renders a pass
	void RenderPass();

	// Flushes queued data
	void Flush();

	void SetFlexMesh( IMesh *pMesh, int nVertexOffsetInBytes );

private:
	// The actual mesh we need to render....
	CBaseMeshDX11 *m_pMesh;

	// The index of the last vertex (for tristrip fixup)
	unsigned short m_LastIndex;

	// Extra padding indices for tristrips
	unsigned short m_ExtraIndices;

	// Am I currently flushing?
	bool m_IsFlushing;

	// has the dynamic mesh been rendered?
	bool m_WasRendered;

	// Do I need to flush?
	bool m_FlushNeeded;

#ifdef DEBUG_BUFFERED_MESHES
	// for debugging only
	bool m_BufferedStateSet;
	BufferedState_t m_BufferedState;
#endif
};

//-----------------------------------------------------------------------------
// Implementation of the mesh manager
//-----------------------------------------------------------------------------
class CMeshMgr : public IMeshMgrDx11
{
public:
	// constructor, destructor
	CMeshMgr();
	virtual ~CMeshMgr();

	// Initialize, shutdown
	void Init();
	void Shutdown();

	// Task switch...
	void ReleaseBuffers();
	void RestoreBuffers();

	// Releases all dynamic vertex buffers
	void DestroyVertexBuffers();

	// Flushes the dynamic mesh
	void Flush();

	// Flushes the vertex buffers
	void DiscardVertexBuffers();

	// Creates, destroys static meshes
	IMesh *CreateStaticMesh( VertexFormat_t vertexFormat, const char *pTextureBudgetGroup, IMaterial *pMaterial = NULL );
	void DestroyStaticMesh( IMesh *pMesh );

	// Gets at the dynamic mesh	(spoofs it though)
	IMesh *GetDynamicMesh( IMaterial *pMaterial, VertexFormat_t vertexFormat, int nHWSkinBoneCount, bool buffered,
			       IMesh *pVertexOverride, IMesh *pIndexOverride );

	// -----------------------------------------------------------
	// ------------ New Vertex/Index Buffer interface ----------------------------
		// Do we need support for bForceTempMesh and bSoftwareVertexShader?
		// I don't think we use bSoftwareVertexShader anymore. .need to look into bForceTempMesh.
	IVertexBuffer *CreateVertexBuffer( ShaderBufferType_t type, VertexFormat_t fmt, int nVertexCount, const char *pBudgetGroup )
	{
		Assert( 0 );
		return NULL;
	}
	IIndexBuffer *CreateIndexBuffer( ShaderBufferType_t bufferType, MaterialIndexFormat_t fmt, int nIndexCount, const char *pBudgetGroup )
	{
		Assert( 0 );
		return NULL;
	}
	void DestroyVertexBuffer( IVertexBuffer * )
	{
		Assert( 0 );
	}
	void DestroyIndexBuffer( IIndexBuffer * )
	{
		Assert( 0 );
	}
	// Do we need to specify the stream here in the case of locking multiple dynamic VBs on different streams?
	IVertexBuffer *GetDynamicVertexBuffer( int streamID, VertexFormat_t vertexFormat, bool bBuffered = true );
	IIndexBuffer *GetDynamicIndexBuffer( MaterialIndexFormat_t fmt, bool bBuffered = true );
	void BindVertexBuffer( int streamID, IVertexBuffer *pVertexBuffer, int nOffsetInBytes, int nFirstVertex, int nVertexCount, VertexFormat_t fmt, int nRepetitions = 1 )
	{
		Assert( 0 );
	}
	void BindIndexBuffer( IIndexBuffer *pIndexBuffer, int nOffsetInBytes )
	{
		Assert( 0 );
	}
	void Draw( MaterialPrimitiveType_t primitiveType, int nFirstIndex, int nIndexCount )
	{
		Assert( 0 );
	}
	// ------------ End ----------------------------
	void RenderPassWithVertexAndIndexBuffers( void );

	VertexFormat_t GetCurrentVertexFormat( void ) const
	{
		return m_CurrentVertexFormat;
	}

	// Gets at the *actual* dynamic mesh
	IMesh *GetActualDynamicMesh( VertexFormat_t vertexFormat );
	IMesh *GetFlexMesh();

	// Computes vertex format from a list of ingredients
	VertexFormat_t ComputeVertexFormat( unsigned int flags,
					    int numTexCoords, int *pTexCoordDimensions, int numBoneWeights,
					    int userDataSize ) const;

		    // Use fat vertices (for tools)
	virtual void UseFatVertices( bool bUseFat );

	// Returns the number of vertices we can render using the dynamic mesh
	virtual void GetMaxToRender( IMesh *pMesh, bool bMaxUntilFlush, int *pMaxVerts, int *pMaxIndices );
	virtual int GetMaxVerticesToRender( IMaterial *pMaterial );
	virtual int GetMaxIndicesToRender();

	// Returns a vertex buffer appropriate for the flags
	CVertexBufferDx11 *FindOrCreateVertexBuffer( int nDynamicBufferId, VertexFormat_t fmt );
	CIndexBufferDx11 *GetDynamicIndexBuffer();

	// Is the mesh dynamic?
	bool IsDynamicMesh( IMesh *pMesh ) const;
	bool IsBufferedDynamicMesh( IMesh *pMesh ) const;

	// Is the vertex buffer dynamic?
	bool IsDynamicVertexBuffer( IVertexBuffer *pVertexBuffer ) const;

	// Is the index buffer dynamic?
	bool IsDynamicIndexBuffer( IIndexBuffer *pIndexBuffer ) const;

	// Returns the vertex size 
	int VertexFormatSize( VertexFormat_t vertexFormat ) const
	{
		return CVertexBufferBase::VertexFormatSize( vertexFormat );
	}

	// Computes the vertex buffer pointers 
	void ComputeVertexDescription( unsigned char *pBuffer,
				       VertexFormat_t vertexFormat, MeshDesc_t &desc ) const;

	IVertexBuffer *GetDynamicVertexBuffer( IMaterial *pMaterial, bool buffered = true );
	IIndexBuffer *GetDynamicIndexBuffer( IMaterial *pMaterial, bool buffered = true );
	virtual void MarkUnusedVertexFields( unsigned int nFlags, int nTexCoordCount, bool *pUnusedTexCoords );

	int UnusedVertexFields() const
	{
		return m_nUnusedVertexFields;
	}
	int UnusedTextureCoords() const
	{
		return m_nUnusedTextureCoords;
	}

	virtual int BufferCount() const
	{
		return 0;
	}

private:
	bool SetRenderState( int nVertexOffsetInBytes, int nFirstVertexIdx, VertexFormat_t vertexFormat, int nVertexStride );

	struct VertexBufferLookup_t
	{
		CVertexBufferDx11 *m_pBuffer;
		int				m_VertexSize;
	};

	void CopyStaticMeshIndexBufferToTempMeshIndexBuffer( CTempMeshDX11 *pDstIndexMesh, CMeshDX11 *pSrcIndexMesh );

	// Cleans up the class
	void CleanUp();

	// The dynamic index buffer
	CIndexBufferDx11 *m_pDynamicIndexBuffer;

	// A static vertexID buffer
	CVertexBufferDx11 *m_pVertexIDBuffer;

	// The dynamic vertex buffers
	CUtlVector< VertexBufferLookup_t >	m_DynamicVertexBuffers;

	// The buffered mesh
	CBufferedMeshDX11 m_BufferedMesh;

	// The current dynamic mesh
	CDynamicMeshDX11 m_DynamicMesh;
	CDynamicMeshDX11 m_DynamicFlexMesh;

	// The current dynamic vertex buffer
	CVertexBufferDx11 m_DynamicVertexBuffer;

	// The current dynamic index buffer
	CIndexBufferDx11 m_DynamicIndexBuffer;

	// The dynamic mesh temp version (for shaders that modify vertex data)
	CTempMeshDX11 m_DynamicTempMesh;

	// Am I buffering or not?
	bool m_BufferedMode;

	// Using fat vertices?
	bool m_bUseFatVertices;

	CVertexBufferDx11 *m_pCurrentVertexBuffer;
	VertexFormat_t m_CurrentVertexFormat;
	int m_pVertexBufferOffset[MAX_DX11_STREAMS];
	int m_pCurrentVertexStride[MAX_DX11_STREAMS];
	int m_pFirstVertex[MAX_DX11_STREAMS];
	int m_pVertexCount[MAX_DX11_STREAMS];
	CIndexBufferBase *m_pCurrentIndexBuffer;
	int m_nIndexBufferOffset;
	MaterialPrimitiveType_t m_PrimitiveType;
	int m_nFirstIndex;
	int m_nNumIndices;

	unsigned int m_nUnusedVertexFields;
	unsigned int m_nUnusedTextureCoords;
};

//-----------------------------------------------------------------------------
// Singleton Dx11 Mesh manager
//-----------------------------------------------------------------------------
static CMeshMgr g_MeshMgr;
IMeshMgrDx11 *MeshMgr()
{
	return &g_MeshMgr;
}

static CIndexBufferDx11 *g_pLastIndex = NULL;
static CVertexBufferDx11 *g_pLastVertex = NULL;
static CMeshDX11 *g_pLastColorMesh = NULL;

//-----------------------------------------------------------------------------
//
// Base mesh
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CBaseMeshDX11::CBaseMeshDX11() : m_VertexFormat( 0 )
{
	m_bMeshLocked = false;
#ifdef _DEBUG
	m_IsDrawing = false;
	m_pMaterial = 0;
#endif
}

CBaseMeshDX11::~CBaseMeshDX11()
{
}


//-----------------------------------------------------------------------------
// For debugging...
//-----------------------------------------------------------------------------
bool CBaseMeshDX11::DebugTrace() const
{
#ifdef _DEBUG
	if ( m_pMaterial )
		return m_pMaterial->PerformDebugTrace();
#endif

	return false;
}

void CBaseMeshDX11::SetMaterial( IMaterial *pMaterial )
{
#ifdef _DEBUG
	m_pMaterial = static_cast<IMaterialInternal *>( pMaterial );
#endif
}


//-----------------------------------------------------------------------------
// Sets, gets the vertex format
//-----------------------------------------------------------------------------
void CBaseMeshDX11::SetVertexFormat( VertexFormat_t format )
{
	m_VertexFormat = format;
}

VertexFormat_t CBaseMeshDX11::GetVertexFormat() const
{
	return m_VertexFormat;
}


//-----------------------------------------------------------------------------
// Sets/gets the morph format
//-----------------------------------------------------------------------------
void CBaseMeshDX11::SetMorphFormat( MorphFormat_t format )
{
	m_MorphFormat = format;
}

MorphFormat_t CBaseMeshDX11::GetMorphFormat() const
{
	return m_MorphFormat;
}


//-----------------------------------------------------------------------------
// Am I using morph data?
//-----------------------------------------------------------------------------
bool CBaseMeshDX11::IsUsingMorphData() const
{
	LOCK_SHADERAPI();
	// We're not using a morph unless the bound morph is a superset of what the rendermesh needs
	MorphFormat_t morphFormat = GetMorphFormat();
	if ( !morphFormat )
		return false;

	return ( ( morphFormat & ShaderUtil()->GetBoundMorphFormat() ) == morphFormat );
}

//-----------------------------------------------------------------------------
// Do I need to reset the vertex format?
//-----------------------------------------------------------------------------
bool CBaseMeshDX11::NeedsVertexFormatReset( VertexFormat_t fmt ) const
{
	return m_VertexFormat != fmt;
}


//-----------------------------------------------------------------------------
// Do I have enough room?
//-----------------------------------------------------------------------------
bool CBaseMeshDX11::HasEnoughRoom( int nVertexCount, int nIndexCount ) const
{
	// by default, we do
	return true;
}


//-----------------------------------------------------------------------------
// Locks mesh for modifying
//-----------------------------------------------------------------------------
void CBaseMeshDX11::ModifyBeginEx( bool bReadOnly, int nFirstVertex, int nVertexCount, int nFirstIndex, int nIndexCount, MeshDesc_t &desc )
{
	LOCK_SHADERAPI();
	// for the time being, disallow for most cases
	Assert( 0 );
}

void CBaseMeshDX11::ModifyBegin( int nFirstVertex, int nVertexCount, int nFirstIndex, int nIndexCount, MeshDesc_t &desc )
{
	LOCK_SHADERAPI();
	// for the time being, disallow for most cases
	Assert( 0 );
}

void CBaseMeshDX11::ModifyEnd( MeshDesc_t &desc )
{
	LOCK_SHADERAPI();
	// for the time being, disallow for most cases
	Assert( 0 );
}


//-----------------------------------------------------------------------------
// Begins a pass
//-----------------------------------------------------------------------------
void CBaseMeshDX11::BeginPass()
{
	LOCK_SHADERAPI();
}


//-----------------------------------------------------------------------------
// Sets the render state and gets the drawing going
//-----------------------------------------------------------------------------
inline void CBaseMeshDX11::DrawMesh()
{
#ifdef _DEBUG
	// Make sure we're not drawing...
	Assert( !m_IsDrawing );
	m_IsDrawing = true;
#endif

	// This is going to cause RenderPass to get called a bunch
	g_pShaderAPIDx11->DrawMesh( this );

#ifdef _DEBUG
	m_IsDrawing = false;
#endif
}


//-----------------------------------------------------------------------------
// Spews the mesh data
//-----------------------------------------------------------------------------
void CBaseMeshDX11::Spew( int nVertexCount, int nIndexCount, const MeshDesc_t &spewDesc )
{
	LOCK_SHADERAPI();
	// This has regressed.
	int i;


	// FIXME: just fall back to the base class (CVertexBufferBase) version of this function!


#ifdef _DEBUG
	if ( m_pMaterial )
	{
		Plat_DebugString( (const char *)m_pMaterial->GetName() );
		Plat_DebugString( "\n" );
	}
#endif // _DEBUG

	// This is needed so buffering can just use this
	VertexFormat_t fmt = m_VertexFormat;

	// Set up the vertex descriptor
	MeshDesc_t desc = spewDesc;

	char tempbuf[256];
	char *temp = tempbuf;
	sprintf( tempbuf, "\nVerts: (Vertex Format %x)\n", fmt );
	Plat_DebugString( tempbuf );

	CVertexBufferBase::PrintVertexFormat( fmt );

	int numBoneWeights = NumBoneWeights( fmt );
	for ( i = 0; i < nVertexCount; ++i )
	{
		temp += sprintf( temp, "[%4d] ", i + desc.m_nFirstVertex );
		if ( fmt & VERTEX_POSITION )
		{
			DirectX::XMFLOAT3 &pos = Position( desc, i );
			temp += sprintf( temp, "P %8.2f %8.2f %8.2f ",
					 pos.x, pos.y, pos.z );
		}

		if ( fmt & VERTEX_WRINKLE )
		{
			float flWrinkle = Wrinkle( desc, i );
			temp += sprintf( temp, "Wr %8.2f ", flWrinkle );
		}

		if ( numBoneWeights > 0 )
		{
			temp += sprintf( temp, "BW " );
			DirectX::XMFLOAT3 pWeight = BoneWeight( desc, i );
			for ( int j = 0; j < numBoneWeights; ++j )
			{
				temp += sprintf( temp, "%1.2f ", (&pWeight)[j] );
			}
		}
		if ( fmt & VERTEX_BONE_INDEX )
		{
			unsigned char *pIndex = BoneIndex( desc, i );
			temp += sprintf( temp, "BI %d %d %d %d ", (int)pIndex[0], (int)pIndex[1], (int)pIndex[2], (int)pIndex[3] );
			Assert( pIndex[0] >= 0 && pIndex[0] < 16 );
			Assert( pIndex[1] >= 0 && pIndex[1] < 16 );
			Assert( pIndex[2] >= 0 && pIndex[2] < 16 );
			Assert( pIndex[3] >= 0 && pIndex[3] < 16 );
		}

		if ( fmt & VERTEX_NORMAL )
		{
			DirectX::XMFLOAT3 &normal = Normal( desc, i );
			temp += sprintf( temp, "N %1.2f %1.2f %1.2f ",
					 normal.x, normal.y, normal.z );
		}

		if ( fmt & VERTEX_COLOR )
		{
			unsigned char *pColor = Color( desc, i );
			temp += sprintf( temp, "C b %3d g %3d r %3d a %3d ",
					 pColor[0], pColor[1], pColor[2], pColor[3] );
		}

		for ( int j = 0; j < VERTEX_MAX_TEXTURE_COORDINATES; ++j )
		{
			if ( TexCoordSize( j, fmt ) > 0 )
			{
				DirectX::XMFLOAT2 &texcoord = TexCoord( desc, i, j );
				temp += sprintf( temp, "T%d %.2f %.2f ", j, texcoord.x, texcoord.y );
			}
		}

		if ( fmt & VERTEX_TANGENT_S )
		{
			DirectX::XMFLOAT3 &tangentS = TangentS( desc, i );
			temp += sprintf( temp, "S %1.2f %1.2f %1.2f ",
					 tangentS.x, tangentS.y, tangentS.z );
		}

		if ( fmt & VERTEX_TANGENT_T )
		{
			DirectX::XMFLOAT3 &tangentT = TangentT( desc, i );
			temp += sprintf( temp, "T %1.2f %1.2f %1.2f ",
					 tangentT.x, tangentT.y, tangentT.z );
		}

		sprintf( temp, "\n" );
		Plat_DebugString( tempbuf );
		temp = tempbuf;
	}

	sprintf( tempbuf, "\nIndices: %d\n", nIndexCount );
	Plat_DebugString( tempbuf );
	for ( i = 0; i < nIndexCount; ++i )
	{
		temp += sprintf( temp, "%d ", (int)desc.m_pIndices[i] );
		if ( ( i & 0x0F ) == 0x0F )
		{
			sprintf( temp, "\n" );
			Plat_DebugString( tempbuf );
			tempbuf[0] = '\0';
			temp = tempbuf;
		}
	}
	sprintf( temp, "\n" );
	Plat_DebugString( tempbuf );
}

void CBaseMeshDX11::ValidateData( int nVertexCount, int nIndexCount, const MeshDesc_t &spewDesc )
{
	LOCK_SHADERAPI();
}

void CBaseMeshDX11::Draw( CPrimList *pLists, int nLists )
{
	LOCK_SHADERAPI();
	Assert( !"CBaseMeshDX11::Draw(CPrimList, int): should never get here." );
}


// Copy verts and/or indices to a mesh builder. This only works for temp meshes!
void CBaseMeshDX11::CopyToMeshBuilder(
	int iStartVert,		// Which vertices to copy.
	int nVerts,
	int iStartIndex,	// Which indices to copy.
	int nIndices,
	int indexOffset,	// This is added to each index.
	CMeshBuilder &builder )
{
	LOCK_SHADERAPI();
	Assert( false );
	Warning( "CopyToMeshBuilder called on something other than a temp mesh.\n" );
}

//-----------------------------------------------------------------------------
//
// static mesh
//
//-----------------------------------------------------------------------------

CPrimList *CMeshDX11::s_pPrims;
int CMeshDX11::s_nPrims;
unsigned int CMeshDX11::s_FirstVertex;
unsigned int CMeshDX11::s_NumVertices;

//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------
CMeshDX11::CMeshDX11( const char *pTextureGroupName ) : m_NumVertices( 0 ), m_NumIndices( 0 ), m_pVertexBuffer( 0 ),
m_pColorMesh( 0 ), m_nColorMeshVertOffsetInBytes( 0 ),
m_pIndexBuffer( 0 ), m_Type( MATERIAL_TRIANGLES ), m_IsVBLocked( false ),
m_IsIBLocked( false )
{
	m_pTextureGroupName = pTextureGroupName;

	m_bHasFlexVerts = false;
	m_pFlexVertexBuffer = NULL;
	m_nFlexVertOffsetInBytes = 0;
}

CMeshDX11::~CMeshDX11()
{
	// Don't release the vertex buffer 
	if ( !g_MeshMgr.IsDynamicMesh( this ) )
	{
		if ( m_pVertexBuffer )
		{
			g_pShaderDevice->DestroyVertexBuffer( m_pVertexBuffer );
		}
		if ( m_pIndexBuffer )
		{
			g_pShaderDevice->DestroyIndexBuffer( m_pIndexBuffer );
		}
	}
}

void CMeshDX11::SetFlexMesh( IMesh *pMesh, int nVertexOffsetInBytes )
{
	if ( !ShaderUtil()->OnSetFlexMesh( this, pMesh, nVertexOffsetInBytes ) )
		return;

	LOCK_SHADERAPI();
	m_nFlexVertOffsetInBytes = nVertexOffsetInBytes;	// Offset into dynamic mesh (in bytes)

	if ( pMesh )
	{
		m_flexVertCount = pMesh->VertexCount();
		pMesh->MarkAsDrawn();

		CBaseMeshDX11 *pBaseMesh = static_cast<CBaseMeshDX11 *>( pMesh );
		m_pFlexVertexBuffer = pBaseMesh->GetVertexBuffer();

		m_bHasFlexVerts = true;
	}
	else
	{
		m_flexVertCount = 0;
		m_pFlexVertexBuffer = NULL;
		m_bHasFlexVerts = false;
	}
}

void CMeshDX11::DisableFlexMesh()
{
	CMeshDX11::SetFlexMesh( NULL, 0 );
}

bool CMeshDX11::HasFlexMesh() const
{
	LOCK_SHADERAPI();
	return m_bHasFlexVerts;
}

void CMeshDX11::SetColorMesh( IMesh *pColorMesh, int nVertexOffsetInBytes )
{
	if ( !ShaderUtil()->OnSetColorMesh( this, pColorMesh, nVertexOffsetInBytes ) )
		return;

	LOCK_SHADERAPI();
	m_pColorMesh = (CMeshDX11 *)pColorMesh; // dangerous conversion! garymcthack
	m_nColorMeshVertOffsetInBytes = nVertexOffsetInBytes;
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

bool CMeshDX11::HasColorMesh() const
{
	LOCK_SHADERAPI();
	return ( m_pColorMesh != NULL );
}

bool CMeshDX11::Lock( int nVertexCount, bool bAppend, VertexDesc_t &desc )
{
	Assert( !m_IsVBLocked );

	//Log( "MeshDx11::Lock(): nVertexCount = %i, bAppend = %i\n", nVertexCount, (int)bAppend );

	if ( g_pShaderDeviceDx11->IsDeactivated() || ( nVertexCount == 0 ) )
	{
		//Log( "Deactivated or no verts\n" );
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
	g_MeshMgr.GetMaxToRender( this, false, &nMaxVerts, &nMaxIndices );
	if ( !g_pHardwareConfig->SupportsStreamOffset() )
	{
		// Without stream offset, we can't use VBs greater than 65535 verts (due to our using 16-bit indices)
		Assert( nVertexCount <= nMaxVerts );
	}

	bool ret = m_pVertexBuffer->Lock( nVertexCount, bAppend, desc );
	if ( !ret )
	{
		//Log( "Couldn't lock vertex buffer\n" );
		if ( nVertexCount > nMaxVerts )
		{
			Assert( 0 );
			Error( "Too many verts for a dynamic vertex buffer (%d>%d) Tell a programmer to up VERTEX_BUFFER_SIZE.\n",
				(int)nVertexCount, (int)nMaxVerts );
		}
		return false;
	}

	m_IsVBLocked = true;
	return true;
}

void CMeshDX11::Unlock( int nVertexCount, VertexDesc_t &desc )
{
	// NOTE: This can happen if another application finishes
	// initializing during the construction of a mesh
	if ( !m_IsVBLocked )
		return;

#ifdef CHECK_INDICES
	m_pIndexBuffer->UpdateShadowIndices( (unsigned short *)m_LockIndexBuffer );
#endif // CHECK_INDICES

	Assert( m_pVertexBuffer );
	m_pVertexBuffer->Unlock( nVertexCount, desc );
	m_IsVBLocked = false;
}

//-----------------------------------------------------------------------------
// Locks/unlocks the index buffer
//-----------------------------------------------------------------------------
int CMeshDX11::Lock( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t &desc )
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
		return desc.m_nFirstIndex;
	}

#if defined( RECORDING ) || defined( CHECK_INDICES )
	m_LockIndexBufferSize = nIndexCount * 2;
	m_LockIndexBuffer = desc.m_pIndices;
#endif

	m_IsIBLocked = true;
	return desc.m_nFirstIndex;
}

void CMeshDX11::Unlock( int nIndexCount, IndexDesc_t &desc )
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

void CMeshDX11::LockMesh( int numVerts, int numIndices, MeshDesc_t &desc )
{
	ShaderUtil()->SyncMatrices();

	g_ShaderMutex.Lock();
	VPROF( "CMeshDx11::LockMesh" );
	//Log( "Locking vertex buffer\n" );
	// Lock vertex buffer
	Lock( numVerts, false, *static_cast<VertexDesc_t *>( &desc ) );
	if ( m_Type != MATERIAL_POINTS )
	{
		// Lock index buffer
		//Log( "Locking index buffer\n" );
		Lock( false, -1, numIndices, *static_cast<IndexDesc_t *>( &desc ) );
	}

	m_bMeshLocked = true;
}

void CMeshDX11::UnlockMesh( int numVerts, int numIndices, MeshDesc_t &desc )
{
	VPROF( "CMeshDx11::UnlockMesh" );

	Assert( m_bMeshLocked );

	Unlock( numVerts, *static_cast<VertexDesc_t *>( &desc ) );
	if ( m_Type != MATERIAL_POINTS )
	{
		Unlock( numIndices, *static_cast<IndexDesc_t *>( &desc ) );
	}

	// The actual # we wrote
	m_NumVertices = numVerts;
	m_NumIndices = numIndices;

	m_bMeshLocked = false;
	g_ShaderMutex.Unlock();
}

void CMeshDX11::ModifyBeginEx( bool bReadOnly, int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t &desc )
{
	VPROF( "CMeshDX8::ModifyBegin" );

	// Just give the app crap buffers to fill up while we're suppressed...

	Assert( m_pVertexBuffer );

	// Lock it baby
	unsigned char *pVertexMemory = m_pVertexBuffer->Modify( bReadOnly, firstVertex, numVerts );
	if ( pVertexMemory )
	{
		m_IsVBLocked = true;
		g_MeshMgr.ComputeVertexDescription( pVertexMemory, m_VertexFormat, desc );
	}

	desc.m_nFirstVertex = firstVertex;

	Lock( bReadOnly, firstIndex, numIndices, *static_cast<IndexDesc_t *>( &desc ) );
}

void CMeshDX11::ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t &desc )
{
	ModifyBeginEx( false, firstVertex, numVerts, firstIndex, numIndices, desc );
}

void CMeshDX11::ModifyEnd( MeshDesc_t &desc )
{
	VPROF( "CMeshDx11::ModifyEnd" );
	Unlock( 0, *static_cast<IndexDesc_t *>( &desc ) );
	Unlock( 0, *static_cast<VertexDesc_t *>( &desc ) );
}

//-----------------------------------------------------------------------------
// returns the # of vertices (static meshes only)
//-----------------------------------------------------------------------------
int CMeshDX11::VertexCount() const
{
	return m_pVertexBuffer ? m_pVertexBuffer->VertexCount() : 0;
}

//-----------------------------------------------------------------------------
// returns the # of indices 
//-----------------------------------------------------------------------------
int CMeshDX11::IndexCount() const
{
	return m_pIndexBuffer ? m_pIndexBuffer->IndexCount() : 0;
}

//-----------------------------------------------------------------------------
// Sets up the vertex and index buffers
//-----------------------------------------------------------------------------
void CMeshDX11::UseIndexBuffer( CIndexBufferDx11 *pBuffer )
{
	m_pIndexBuffer = pBuffer;
}

void CMeshDX11::UseVertexBuffer( CVertexBufferDx11 *pBuffer )
{
	m_pVertexBuffer = pBuffer;
}

//-----------------------------------------------------------------------------
// Sets the primitive type
//-----------------------------------------------------------------------------
void CMeshDX11::SetPrimitiveType( MaterialPrimitiveType_t type )
{
	Assert( IsX360() || ( type != MATERIAL_INSTANCED_QUADS ) );
	if ( !ShaderUtil()->OnSetPrimitiveType( this, type ) )
	{
		return;
	}

	LOCK_SHADERAPI();
	m_Type = type;
}

MaterialPrimitiveType_t CMeshDX11::GetPrimitiveType() const
{
	return m_Type;
}

int CMeshDX11::NumPrimitives( int nVertexCount, int nIndexCount ) const
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

//-----------------------------------------------------------------------------
// Helpers to count texture coordinates
//-----------------------------------------------------------------------------
static int NumTextureCoordinates( VertexFormat_t vertexFormat )
{
	int nTexCoordCount = 0;
	for ( int i = 0; i < VERTEX_MAX_TEXTURE_COORDINATES; ++i )
	{
		if ( TexCoordSize( i, vertexFormat ) == 0 )
			continue;
		++nTexCoordCount;
	}
	return nTexCoordCount;
}

//-----------------------------------------------------------------------------
// Checks if it's a valid format
//-----------------------------------------------------------------------------
#ifdef _DEBUG
static void OutputVertexFormat( VertexFormat_t format )
{
	// FIXME: this is a duplicate of the function in meshdx8.cpp
	VertexCompressionType_t compressionType = CompressionType( format );

	if ( format & VERTEX_POSITION )
	{
		Warning( "VERTEX_POSITION|" );
	}
	if ( format & VERTEX_NORMAL )
	{
		if ( compressionType == VERTEX_COMPRESSION_ON )
			Warning( "VERTEX_NORMAL[COMPRESSED]|" );
		else
			Warning( "VERTEX_NORMAL|" );
	}
	if ( format & VERTEX_COLOR )
	{
		Warning( "VERTEX_COLOR|" );
	}
	if ( format & VERTEX_SPECULAR )
	{
		Warning( "VERTEX_SPECULAR|" );
	}
	if ( format & VERTEX_TANGENT_S )
	{
		Warning( "VERTEX_TANGENT_S|" );
	}
	if ( format & VERTEX_TANGENT_T )
	{
		Warning( "VERTEX_TANGENT_T|" );
	}
	if ( format & VERTEX_BONE_INDEX )
	{
		Warning( "VERTEX_BONE_INDEX|" );
	}
	if ( format & VERTEX_FORMAT_VERTEX_SHADER )
	{
		Warning( "VERTEX_FORMAT_VERTEX_SHADER|" );
	}
	Warning( "\nBone weights: %d (%s)\n", NumBoneWeights( format ),
		( CompressionType( format ) == VERTEX_COMPRESSION_ON ? "compressed" : "uncompressed" ) );
	Warning( "user data size: %d (%s)\n", UserDataSize( format ),
		( CompressionType( format ) == VERTEX_COMPRESSION_ON ? "compressed" : "uncompressed" ) );
	Warning( "num tex coords: %d\n", NumTextureCoordinates( format ) );
	// NOTE: This doesn't print texcoord sizes.
}
#endif

bool CMeshDX11::IsValidVertexFormat( VertexFormat_t vertexFormat )
{
	// FIXME: Make this a debug-only check on say 6th July 2007 (after a week or so's testing)
	//        (i.e. avoid the 360 release build perf. hit for when we ship)
	bool bCheckCompression = ( m_VertexFormat & VERTEX_FORMAT_COMPRESSED ) &&
		( ( vertexFormat == VERTEX_FORMAT_INVALID ) || ( ( vertexFormat & VERTEX_FORMAT_COMPRESSED ) == 0 ) );

	if ( bCheckCompression || IsPC() || IsDebug() )
	{
		IMaterialInternal *pMaterial = g_pShaderAPIDx11->GetBoundMaterial();
		Assert( pMaterial );

		// the material format should match the vertex usage, unless another format is passed in
		if ( vertexFormat == VERTEX_FORMAT_INVALID )
		{
			vertexFormat = pMaterial->GetVertexUsage() & ~( VERTEX_FORMAT_VERTEX_SHADER | VERTEX_FORMAT_USE_EXACT_FORMAT );

			// Blat out unused fields
			vertexFormat &= ~g_MeshMgr.UnusedVertexFields();
			int nUnusedTextureCoords = g_MeshMgr.UnusedTextureCoords();
			for ( int i = 0; i < VERTEX_MAX_TEXTURE_COORDINATES; ++i )
			{
				if ( nUnusedTextureCoords & ( 1 << i ) )
				{
					vertexFormat &= ~VERTEX_TEXCOORD_MASK( i );
				}
			}
		}
		else
		{
			vertexFormat &= ~( VERTEX_FORMAT_VERTEX_SHADER | VERTEX_FORMAT_USE_EXACT_FORMAT );
		}

		bool bIsValid = ( ( VERTEX_FORMAT_FIELD_MASK & vertexFormat ) & ( VERTEX_FORMAT_FIELD_MASK & ~m_VertexFormat ) ) == 0;

		if ( m_VertexFormat & VERTEX_FORMAT_COMPRESSED )
		{
			// We shouldn't get compressed verts if this material doesn't support them!
			if ( ( vertexFormat & VERTEX_FORMAT_COMPRESSED ) == 0 )
			{
				static int numWarnings = 0;
				if ( numWarnings++ == 0 )
				{
					// NOTE: ComputeVertexFormat() will make sure no materials support VERTEX_FORMAT_COMPRESSED
					//       if vertex compression is disabled in the config
					if ( g_pHardwareConfig->SupportsCompressedVertices() == VERTEX_COMPRESSION_NONE )
						Warning( "ERROR: Compressed vertices in use but vertex compression is disabled (or not supported on this hardware)!\n" );
					else
						Warning( "ERROR: Compressed vertices in use but material does not support them!\n" );
				}
				Assert( 0 );
				bIsValid = false;
			}
		}

		bIsValid = bIsValid && UserDataSize( m_VertexFormat ) >= UserDataSize( vertexFormat );

		for ( int i = 0; i < VERTEX_MAX_TEXTURE_COORDINATES; i++ )
		{
			if ( TexCoordSize( i, m_VertexFormat ) < TexCoordSize( i, vertexFormat ) )
			{
				bIsValid = false;
			}
		}

		// NOTE: It can totally be valid to have more weights than the current number of bones.
		// The -1 here is because if we have N bones, we can have only (N-1) weights,
		// since the Nth is implied (the weights sum to 1).
		int nWeightCount = NumBoneWeights( m_VertexFormat );
		bIsValid = bIsValid && ( nWeightCount >= ( g_pShaderAPI->GetCurrentNumBones() - 1 ) );

#ifdef _DEBUG
		if ( !bIsValid )
		{
			Warning( "Material Format:" );
			if ( g_pShaderAPI->GetCurrentNumBones() > 0 )
			{
				vertexFormat |= VERTEX_BONE_INDEX;
				vertexFormat &= ~VERTEX_BONE_WEIGHT_MASK;
				vertexFormat |= VERTEX_BONEWEIGHT( 2 );
			}

			OutputVertexFormat( vertexFormat );
			Warning( "Mesh Format:" );
			OutputVertexFormat( m_VertexFormat );
		}
#endif
		return bIsValid;
	}

	return true;
}

bool CMeshDX11::SetRenderState( int nVertexOffsetInBytes, int nFirstVertexIdx, VertexFormat_t vertexFormat )
{
	// Can't set the state if we're deactivated
	if ( g_pShaderDeviceDx11->IsDeactivated() )
	{
		//ResetMeshRenderState();
		return false;
	}

	//g_LastVertexFormat = vertexFormat;

	// Bind the mesh's buffers
	g_pShaderAPIDx11->SetTopology( m_Type );

	if ( HasFlexMesh() )
	{
		g_pShaderAPIDx11->BindVertexBuffer( 2, GetVertexBuffer(), m_nFlexVertOffsetInBytes, nFirstVertexIdx, m_flexVertCount, GetVertexFormat() );
	}
	else
	{
		//Assert( nVertexOffsetInBytes == 0 );
		//Assert( m_pVertexBuffer );
		
		// HACK...point stream 2 at the same VB which is bound to stream 0...
		// NOTE: D3D debug DLLs will RIP if stream 0 has a smaller stride than the largest
		//       offset in the stream 2 vertex decl elements (which are position(12)+wrinkle(4)+normal(12))
		// If this fires, go find the material/shader which is requesting a really 'thin'
		// stream 0 vertex, and fatten it up slightly (e.g. add a D3DCOLOR element)
		//int minimumStreamZeroStride = 4 * sizeof( float );
		//Assert( m_pVertexBuffer->VertexSize() >= minimumStreamZeroStride );
		//if ( m_pVertexBuffer->VertexSize() < minimumStreamZeroStride )
		//{
		//	static bool bWarned = false;
		//	if ( !bWarned )
		//	{
		//		Warning( "Shader specifying too-thin vertex format, should be at least %d bytes! (Supressing further warnings)\n", minimumStreamZeroStride );
		//		bWarned = true;
		//	}
		//}
		//g_pShaderAPIDx11->BindVertexBuffer( 2, GetVertexBuffer(), nVertexOffsetInBytes, nFirstVertexIdx, m_NumVertices, GetVertexFormat() );
	}

	Assert( m_pVertexBuffer );
	g_pShaderAPIDx11->BindVertexBuffer( 0, GetVertexBuffer(), nVertexOffsetInBytes, nFirstVertexIdx, m_NumVertices, GetVertexFormat() );
	
	Assert( m_pIndexBuffer );
	g_pShaderAPIDx11->BindIndexBuffer( GetIndexBuffer(), nVertexOffsetInBytes );

	g_pLastIndex = GetIndexBuffer();
	g_pLastVertex = GetVertexBuffer();
	

	return true;
}

//-----------------------------------------------------------------------------
// Draws the static mesh
//-----------------------------------------------------------------------------
void CMeshDX11::Draw( int nFirstIndex, int nIndexCount )
{
	if ( !ShaderUtil()->OnDrawMesh( this, nFirstIndex, nIndexCount ) )
	{
		MarkAsDrawn();
		return;
	}

	CPrimList primList;
	if ( nFirstIndex == -1 || nIndexCount == 0 )
	{
		primList.m_FirstIndex = 0;
		primList.m_NumIndices = m_NumIndices;
	}
	else
	{
		primList.m_FirstIndex = nFirstIndex;
		primList.m_NumIndices = nIndexCount;
	}
	DrawInternal( &primList, 1 );
}

void CMeshDX11::Draw( CPrimList *pLists, int nLists )
{
	if ( !ShaderUtil()->OnDrawMesh( this, pLists, nLists ) )
	{
		MarkAsDrawn();
		return;
	}

	DrawInternal( pLists, nLists );
}


void CMeshDX11::DrawInternal( CPrimList *pLists, int nLists )
{
	// Make sure there's something to draw..
	int i;
	for ( i = 0; i < nLists; i++ )
	{
		if ( pLists[i].m_NumIndices > 0 )
			break;
	}
	if ( i == nLists )
		return;

	// can't do these in selection mode!
	Assert( !ShaderAPI()->IsInSelectionMode() );

	if ( !SetRenderState( 0, 0 ) )
		return;

	s_pPrims = pLists;
	s_nPrims = nLists;

#ifdef _DEBUG
	for ( i = 0; i < nLists; ++i )
	{
		Assert( pLists[i].m_NumIndices > 0 );
	}
#endif

	s_FirstVertex = 0;
	s_NumVertices = m_pVertexBuffer->VertexCount();

	DrawMesh();
}

#ifdef CHECK_INDICES
void CMeshDX11::CheckIndices( CPrimList *pPrim, int numPrimitives )
{
	// g_pLastVertex - this is the current vertex buffer
	// g_pLastColorMesh - this is the current color mesh, if there is one.
	// g_pLastIndex - this is the current index buffer.
	// vertoffset : m_FirstIndex
	if ( m_Type == MATERIAL_TRIANGLES || m_Type == MATERIAL_TRIANGLE_STRIP )
	{
		Assert( pPrim->m_FirstIndex >= 0 && pPrim->m_FirstIndex < g_pLastIndex->IndexCount() );
		int i;
		for ( i = 0; i < 2; i++ )
		{
			CVertexBufferDx11 *pMesh;
			if ( i == 0 )
			{
				pMesh = g_pLastVertex;
				Assert( pMesh );
			}
			else
			{
				if ( !g_pLastColorMesh )
				{
					continue;
				}
				pMesh = g_pLastColorMesh->m_pVertexBuffer;
				if ( !pMesh )
				{
					continue;
				}
			}
			Assert( s_FirstVertex >= 0 &&
				(int)( s_FirstVertex + m_FirstIndex ) < pMesh->VertexCount() );
			int nIndexCount = 0;
			if ( m_Type == MATERIAL_TRIANGLES )
			{
				nIndexCount = numPrimitives * 3;
			}
			else if ( m_Type == MATERIAL_TRIANGLE_STRIP )
			{
				nIndexCount = numPrimitives + 2;
			}
			else
			{
				Assert( 0 );
			}
			int j;
			for ( j = 0; j < nIndexCount; j++ )
			{
				int index = g_pLastIndex->GetShadowIndex( j + pPrim->m_FirstIndex );
				Assert( index >= (int)s_FirstVertex );
				Assert( index < (int)( s_FirstVertex + s_NumVertices ) );
			}
		}
	}
}
#endif // CHECK_INDICES

// Draws the specified primitives on the mesh
void CMeshDX11::RenderPass()
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
			int numPrimitives = NumPrimitives( s_NumVertices, pPrim->m_NumIndices );
#ifdef CHECK_INDICES
			CheckIndices( pPrim, numPrimitives );
#endif // CHECK_INDICES
			g_pShaderAPIDx11->DrawIndexed( pPrim->m_FirstIndex, pPrim->m_NumIndices, 0 );
		}
	}
}

//-----------------------------------------------------------------------------
//
// Dynamic mesh implementation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CDynamicMeshDX11::CDynamicMeshDX11() : CMeshDX11( "CDynamicMeshDX8" )
{
	m_nBufferId = 0;
	ResetVertexAndIndexCounts();
}

CDynamicMeshDX11::~CDynamicMeshDX11()
{
}


//-----------------------------------------------------------------------------
// Initializes the dynamic mesh
//-----------------------------------------------------------------------------
void CDynamicMeshDX11::Init( int nBufferId )
{
	m_nBufferId = nBufferId;
}

//-----------------------------------------------------------------------------
// Resets buffering state
//-----------------------------------------------------------------------------
void CDynamicMeshDX11::ResetVertexAndIndexCounts()
{
	m_TotalVertices = m_TotalIndices = 0;
	m_FirstIndex = m_nFirstVertex = -1;
	m_HasDrawn = false;
}

//-----------------------------------------------------------------------------
// Resets the state in case of a task switch
//-----------------------------------------------------------------------------
void CDynamicMeshDX11::Reset()
{
	m_VertexFormat = 0;
	m_pVertexBuffer = 0;
	m_pIndexBuffer = 0;
	ResetVertexAndIndexCounts();

	// Force the render state to be updated next time
	//ResetMeshRenderState();
}

//-----------------------------------------------------------------------------
// Sets the vertex format associated with the dynamic mesh
//-----------------------------------------------------------------------------
void CDynamicMeshDX11::SetVertexFormat( VertexFormat_t format )
{
	if ( g_pShaderDeviceDx11->IsDeactivated() )
		return;

	if ( CompressionType( format ) != VERTEX_COMPRESSION_NONE )
	{
		// UNDONE: support compressed dynamic meshes if needed (pro: less VB memory, con: CMeshBuilder gets slower)
		Warning( "ERROR: dynamic meshes cannot use compressed vertices!\n" );
		Assert( 0 );
		format &= ~VERTEX_FORMAT_COMPRESSED;
	}

	if ( ( format != m_VertexFormat ) || m_VertexOverride || m_IndexOverride )
	{
		m_VertexFormat = format;
		UseVertexBuffer( g_MeshMgr.FindOrCreateVertexBuffer( m_nBufferId, format ) );

		if ( m_nBufferId == 0 )
		{
			UseIndexBuffer( g_MeshMgr.GetDynamicIndexBuffer() );
		}

		m_VertexOverride = m_IndexOverride = false;
	}
}

void CDynamicMeshDX11::OverrideVertexBuffer( CVertexBufferDx11 *pVertexBuffer )
{
	UseVertexBuffer( pVertexBuffer );
	m_VertexOverride = true;
}

void CDynamicMeshDX11::OverrideIndexBuffer( CIndexBufferDx11 *pIndexBuffer )
{
	UseIndexBuffer( pIndexBuffer );
	m_IndexOverride = true;
}


//-----------------------------------------------------------------------------
// Do I need to reset the vertex format?
//-----------------------------------------------------------------------------
bool CDynamicMeshDX11::NeedsVertexFormatReset( VertexFormat_t fmt ) const
{
	//Log( "Does CDynamicMeshDx11 need format reset? %i %i %i != %i\n", m_VertexOverride, m_IndexOverride, m_VertexFormat, fmt );
	return m_VertexOverride || m_IndexOverride || ( m_VertexFormat != fmt ) || m_pVertexBuffer == 0 || m_pIndexBuffer == 0;
}

//-----------------------------------------------------------------------------
// Locks/unlocks the entire mesh
//-----------------------------------------------------------------------------
bool CDynamicMeshDX11::HasEnoughRoom( int nVertexCount, int nIndexCount ) const
{
	if ( g_pShaderDeviceDx11->IsDeactivated() )
		return false;

	// We need space in both the vertex and index buffer
	return m_pVertexBuffer->HasEnoughRoom( nVertexCount ) &&
		m_pIndexBuffer->HasEnoughRoom( nIndexCount );
}

//-----------------------------------------------------------------------------
// returns the number of indices in the mesh
//-----------------------------------------------------------------------------
int CDynamicMeshDX11::IndexCount() const
{
	return m_TotalIndices;
}

//-----------------------------------------------------------------------------
// Operation to do pre-lock	(only called for buffered meshes)
//-----------------------------------------------------------------------------
void CDynamicMeshDX11::PreLock()
{
	if ( m_HasDrawn )
	{
		// Start again then
		ResetVertexAndIndexCounts();
	}
}


//-----------------------------------------------------------------------------
// Locks/unlocks the entire mesh
//-----------------------------------------------------------------------------
void CDynamicMeshDX11::LockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc )
{
	ShaderUtil()->SyncMatrices();

	g_ShaderMutex.Lock();

	// Yes, this may well also be called from BufferedMesh but that's ok
	PreLock();

	if ( m_VertexOverride )
	{
		nVertexCount = 0;
	}

	if ( m_IndexOverride )
	{
		nIndexCount = 0;
	}

	Lock( nVertexCount, false, *static_cast<VertexDesc_t *>( &desc ) );
	if ( m_nFirstVertex < 0 )
	{
		m_nFirstVertex = desc.m_nFirstVertex;
	}

	// When we're using a static index buffer or a flex mesh, the indices assume vertices start at 0
	if ( m_IndexOverride || HasFlexMesh() )
	{
		desc.m_nFirstVertex -= m_nFirstVertex;
	}

	// Don't add indices for points; DrawIndexedPrimitive not supported for them.
	if ( m_Type != MATERIAL_POINTS )
	{
		int nFirstIndex = Lock( false, -1, nIndexCount, *static_cast<IndexDesc_t *>( &desc ) );
		if ( m_FirstIndex < 0 )
		{
			m_FirstIndex = nFirstIndex;
		}
	}
	//else
	//{
	//	desc.m_pIndices = (unsigned short *)( &g_nScratchIndexBuffer );
	//	desc.m_nIndexSize = 0;
	//}

	m_bMeshLocked = true;
}

//-----------------------------------------------------------------------------
// Unlocks the mesh
//-----------------------------------------------------------------------------
void CDynamicMeshDX11::UnlockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc )
{
	m_TotalVertices += nVertexCount;
	m_TotalIndices += nIndexCount;

	if ( DebugTrace() )
	{
		Spew( nVertexCount, nIndexCount, desc );
	}

	CMeshDX11::UnlockMesh( nVertexCount, nIndexCount, desc );

	// This is handled in the CMeshDX8::UnlockMesh above.
	//CBaseMeshDX8::m_bMeshLocked = false;
}

//-----------------------------------------------------------------------------
// Draws it
//-----------------------------------------------------------------------------
void CDynamicMeshDX11::Draw( int nFirstIndex, int nIndexCount )
{
	if ( !ShaderUtil()->OnDrawMesh( this, nFirstIndex, nIndexCount ) )
	{
		MarkAsDrawn();
		return;
	}

	VPROF( "CDynamicMeshDX8::Draw" );

	m_HasDrawn = true;

	if ( m_IndexOverride || m_VertexOverride ||
		( ( m_TotalVertices > 0 ) && ( m_TotalIndices > 0 || m_Type == MATERIAL_POINTS || m_Type == MATERIAL_INSTANCED_QUADS ) ) )
	{
		Assert( !m_IsDrawing );

		// only have a non-zero first vertex when we are using static indices
		int nFirstVertex = m_VertexOverride ? 0 : m_nFirstVertex;
		int actualFirstVertex = m_IndexOverride ? nFirstVertex : 0;
		int nVertexOffsetInBytes = HasFlexMesh() ? nFirstVertex * g_MeshMgr.VertexFormatSize( GetVertexFormat() ) : 0;
		int baseIndex = m_IndexOverride ? 0 : m_FirstIndex;

		// Overriding with the dynamic index buffer, preserve state!
		if ( m_IndexOverride && m_pIndexBuffer == g_MeshMgr.GetDynamicIndexBuffer() )
		{
			baseIndex = m_FirstIndex;
		}

		VertexFormat_t fmt = m_VertexOverride ? GetVertexFormat() : VERTEX_FORMAT_INVALID;
		if ( !SetRenderState( nVertexOffsetInBytes, actualFirstVertex, fmt ) )
			return;

		// Draws a portion of the mesh
		int numVertices = m_VertexOverride ? m_pVertexBuffer->VertexCount() : m_TotalVertices;
		if ( ( nFirstIndex != -1 ) && ( nIndexCount != 0 ) )
		{
			nFirstIndex += baseIndex;
		}
		else
		{
			// by default we draw the whole thing
			nFirstIndex = baseIndex;
			if ( m_IndexOverride )
			{
				nIndexCount = m_pIndexBuffer->IndexCount();
				Assert( nIndexCount != 0 );
			}
			else
			{
				nIndexCount = m_TotalIndices;
				// Fake out the index count	if we're drawing points/instanced-quads
				if ( ( m_Type == MATERIAL_POINTS ) || ( m_Type == MATERIAL_INSTANCED_QUADS ) )
				{
					nIndexCount = m_TotalVertices;
				}
				Assert( nIndexCount != 0 );
			}
		}

		// Fix up nFirstVertex to indicate the first vertex used in the data
		if ( !HasFlexMesh() )
		{
			actualFirstVertex = nFirstVertex - actualFirstVertex;
		}

		s_FirstVertex = actualFirstVertex;
		s_NumVertices = numVertices;

		// Build a primlist with 1 element..
		CPrimList prim;
		prim.m_FirstIndex = nFirstIndex;
		prim.m_NumIndices = nIndexCount;
		Assert( nIndexCount != 0 );
		s_pPrims = &prim;
		s_nPrims = 1;

		DrawMesh();

		s_pPrims = NULL;
	}
}

//-----------------------------------------------------------------------------
// This is useful when we need to dynamically modify data; just set the
// render state and draw the pass immediately
//-----------------------------------------------------------------------------
void CDynamicMeshDX11::DrawSinglePassImmediately()
{
	if ( ( m_TotalVertices > 0 ) || ( m_TotalIndices > 0 ) )
	{
		Assert( !m_IsDrawing );

		// Set the render state
		if ( SetRenderState( 0, 0 ) )
		{
			s_FirstVertex = m_nFirstVertex;
			s_NumVertices = m_TotalVertices;

			// Make a temporary PrimList to hold the indices.
			CPrimList prim( m_FirstIndex, m_TotalIndices );
			Assert( m_TotalIndices != 0 );
			s_pPrims = &prim;
			s_nPrims = 1;

			// Render it
			RenderPass();
		}

		// We're done with our data
		ResetVertexAndIndexCounts();
	}
}

//-----------------------------------------------------------------------------
//
// A mesh that stores temporary vertex data in the correct format (for modification)
//
//-----------------------------------------------------------------------------
// Used in rendering sub-parts of the mesh
unsigned int CTempMeshDX11::s_NumIndices;
unsigned int CTempMeshDX11::s_FirstIndex;

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CTempMeshDX11::CTempMeshDX11( bool isDynamic ) : m_VertexSize( 0xFFFF ), m_IsDynamic( isDynamic )
{
#ifdef _DEBUG
	m_Locked = false;
	m_InPass = false;
#endif
}

CTempMeshDX11::~CTempMeshDX11()
{
}

//-----------------------------------------------------------------------------
// Is the temp mesh dynamic?
//-----------------------------------------------------------------------------
bool CTempMeshDX11::IsDynamic() const
{
	return m_IsDynamic;
}


//-----------------------------------------------------------------------------
// Sets the vertex format
//-----------------------------------------------------------------------------
void CTempMeshDX11::SetVertexFormat( VertexFormat_t format )
{
	CBaseMeshDX11::SetVertexFormat( format );
	m_VertexSize = g_MeshMgr.VertexFormatSize( format );
}

//-----------------------------------------------------------------------------
// returns the # of vertices (static meshes only)
//-----------------------------------------------------------------------------
int CTempMeshDX11::VertexCount() const
{
	return m_VertexSize ? m_VertexData.Count() / m_VertexSize : 0;
}

//-----------------------------------------------------------------------------
// returns the # of indices 
//-----------------------------------------------------------------------------
int CTempMeshDX11::IndexCount() const
{
	return m_IndexData.Count();
}

void CTempMeshDX11::ModifyBeginEx( bool bReadOnly, int nFirstVertex, int nVertexCount, int nFirstIndex, int nIndexCount, MeshDesc_t &desc )
{
	Assert( !m_Locked );

	m_LockedVerts = nVertexCount;
	m_LockedIndices = nIndexCount;

	if ( nVertexCount > 0 )
	{
		int vertexByteOffset = m_VertexSize * nFirstVertex;

		// Lock it baby
		unsigned char *pVertexMemory = &m_VertexData[vertexByteOffset];

		// Compute the vertex index..
		desc.m_nFirstVertex = vertexByteOffset / m_VertexSize;

		// Set up the mesh descriptor
		g_MeshMgr.ComputeVertexDescription( pVertexMemory, m_VertexFormat, desc );
	}
	else
	{
		desc.m_nFirstVertex = 0;
		// Set up the mesh descriptor
		g_MeshMgr.ComputeVertexDescription( 0, 0, desc );
	}

	if ( m_Type != MATERIAL_POINTS && nIndexCount > 0 )
	{
		desc.m_pIndices = &m_IndexData[nFirstIndex];
		desc.m_nIndexSize = 1;
	}
	//else
	//{
	//	desc.m_pIndices = (unsigned short *)( &g_nScratchIndexBuffer );
	//	desc.m_nIndexSize = 0;
	//}

#ifdef _DEBUG
	m_Locked = true;
#endif
}

void CTempMeshDX11::ModifyBegin( int nFirstVertex, int nVertexCount, int nFirstIndex, int nIndexCount, MeshDesc_t &desc )
{
	ModifyBeginEx( false, nFirstVertex, nVertexCount, nFirstIndex, nIndexCount, desc );
}

void CTempMeshDX11::ModifyEnd( MeshDesc_t &desc )
{
#ifdef _DEBUG
	Assert( m_Locked );
	m_Locked = false;
#endif
}

//-----------------------------------------------------------------------------
// Locks/unlocks the mesh
//-----------------------------------------------------------------------------
void CTempMeshDX11::LockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc )
{
	ShaderUtil()->SyncMatrices();

	g_ShaderMutex.Lock();

	Assert( !m_Locked );

	m_LockedVerts = nVertexCount;
	m_LockedIndices = nIndexCount;

	if ( nVertexCount > 0 )
	{
		int vertexByteOffset = m_VertexData.AddMultipleToTail( m_VertexSize * nVertexCount );

		// Lock it baby
		unsigned char *pVertexMemory = &m_VertexData[vertexByteOffset];

		// Compute the vertex index..
		desc.m_nFirstVertex = vertexByteOffset / m_VertexSize;

		// Set up the mesh descriptor
		g_MeshMgr.ComputeVertexDescription( pVertexMemory, m_VertexFormat, desc );
	}
	else
	{
		desc.m_nFirstVertex = 0;
		// Set up the mesh descriptor
		g_MeshMgr.ComputeVertexDescription( 0, 0, desc );
	}

	if ( m_Type != MATERIAL_POINTS && nIndexCount > 0 )
	{
		int nFirstIndex = m_IndexData.AddMultipleToTail( nIndexCount );
		desc.m_pIndices = &m_IndexData[nFirstIndex];
		desc.m_nIndexSize = 1;
	}
	//else
	//{
	//	desc.m_pIndices = (unsigned short *)( &g_nScratchIndexBuffer );
	//	desc.m_nIndexSize = 0;
	//}

#ifdef _DEBUG
	m_Locked = true;
#endif

	m_bMeshLocked = true;
}

void CTempMeshDX11::UnlockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc )
{
	Assert( m_Locked );

	// Remove unused vertices and indices
	int verticesToRemove = m_LockedVerts - nVertexCount;
	if ( verticesToRemove != 0 )
	{
		m_VertexData.RemoveMultiple( m_VertexData.Count() - verticesToRemove, verticesToRemove );
	}

	int indicesToRemove = m_LockedIndices - nIndexCount;
	if ( indicesToRemove != 0 )
	{
		m_IndexData.RemoveMultiple( m_IndexData.Count() - indicesToRemove, indicesToRemove );
	}

#ifdef _DEBUG
	m_Locked = false;
#endif

	m_bMeshLocked = false;

	g_ShaderMutex.Unlock();
}

//-----------------------------------------------------------------------------
// Sets the primitive type
//-----------------------------------------------------------------------------
void CTempMeshDX11::SetPrimitiveType( MaterialPrimitiveType_t type )
{
	// FIXME: Support MATERIAL_INSTANCED_QUADS for CTempMeshDX8 (X360 only)
	Assert( ( type != MATERIAL_INSTANCED_QUADS ) /* || IsX360() */ );
	m_Type = type;
}

MaterialPrimitiveType_t CTempMeshDX11::GetPrimitiveType() const
{
	return m_Type;
}

//-----------------------------------------------------------------------------
// Gets the dynamic mesh
//-----------------------------------------------------------------------------
CDynamicMeshDX11 *CTempMeshDX11::GetDynamicMesh()
{
	return static_cast<CDynamicMeshDX11 *>( g_MeshMgr.GetActualDynamicMesh( m_VertexFormat ) );
}

//-----------------------------------------------------------------------------
// Draws the entire mesh
//-----------------------------------------------------------------------------
void CTempMeshDX11::Draw( int nFirstIndex, int nIndexCount )
{
	if ( !ShaderUtil()->OnDrawMesh( this, nFirstIndex, nIndexCount ) )
	{
		MarkAsDrawn();
		return;
	}

	if ( m_VertexData.Count() > 0 )
	{
		if ( !g_pShaderDeviceDx11->IsDeactivated() )
		{
#ifdef DRAW_SELECTION
			if ( !g_bDrawSelection && !ShaderAPI()->IsInSelectionMode() )
#else
			if ( !ShaderAPI()->IsInSelectionMode() )
#endif
			{
				s_FirstIndex = nFirstIndex;
				s_NumIndices = nIndexCount;

				DrawMesh();

				// This assertion fails if a BeginPass() call was not matched by 
				// a RenderPass() call
				Assert( !m_InPass );
			}
			else
			{
				TestSelection();
			}
		}

		// Clear out the data if this temp mesh is a dynamic one...
		if ( m_IsDynamic )
		{
			m_VertexData.RemoveAll();
			m_IndexData.RemoveAll();
		}
	}
}

void CTempMeshDX11::CopyToMeshBuilder(
	int iStartVert,		// Which vertices to copy.
	int nVerts,
	int iStartIndex,	// Which indices to copy.
	int nIndices,
	int indexOffset,	// This is added to each index.
	CMeshBuilder &builder )
{
	int startOffset = iStartVert * m_VertexSize;
	int endOffset = ( iStartVert + nVerts ) * m_VertexSize;
	Assert( startOffset >= 0 && startOffset <= m_VertexData.Count() );
	Assert( endOffset >= 0 && endOffset <= m_VertexData.Count() && endOffset >= startOffset );
	if ( endOffset > startOffset )
	{
		// FIXME: make this a method of CMeshBuilder (so the 'Position' pointer accessor can be removed)
		//        make sure it takes a VertexFormat_t parameter for src/dest match validation
		memcpy( (void *)builder.Position(), &m_VertexData[startOffset], endOffset - startOffset );
		builder.AdvanceVertices( nVerts );
	}

	for ( int i = 0; i < nIndices; ++i )
	{
		builder.Index( m_IndexData[iStartIndex + i] + indexOffset );
		builder.AdvanceIndex();
	}
}

//-----------------------------------------------------------------------------
// Selection mode helper functions
//-----------------------------------------------------------------------------
static void ComputeModelToView( DirectX::XMMATRIX &modelToView )
{
	// Get the modelview matrix...
	DirectX::XMMATRIX world, view;
	ShaderAPI()->GetMatrix( MATERIAL_MODEL, world );
	ShaderAPI()->GetMatrix( MATERIAL_VIEW, view );
	modelToView = DirectX::XMMatrixMultiply( world, view );
}

static float ComputeCullFactor()
{
	D3D11_CULL_MODE cullMode = ShaderAPI()->GetCullMode();

	float cullFactor;
	switch ( cullMode )
	{
	case D3D11_CULL_BACK:
		cullFactor = -1.0f;
		break;

	case D3D11_CULL_FRONT:
		cullFactor = 1.0f;
		break;

	default:
		cullFactor = 0.0f;
		break;
	};

	return cullFactor;
}

//-----------------------------------------------------------------------------
// Clip to viewport
//-----------------------------------------------------------------------------
static int g_NumClipVerts;
static DirectX::XMVECTOR g_ClipVerts[16];

static float XMVec3Component( const DirectX::XMVECTOR &flt3, int cmp )
{
	switch ( cmp )
	{
	case 0:
		return DirectX::XMVectorGetX( flt3 );
	case 1:
		return DirectX::XMVectorGetY( flt3 );
	case 2:
		return DirectX::XMVectorGetZ( flt3 );
	default:
		return DirectX::XMVectorGetX( flt3 );
	}
}

static void XMVec3SetComponent( DirectX::XMVECTOR &flt3, int cmp, float val )
{
	switch ( cmp )
	{
	case 0:
		flt3 = DirectX::XMVectorSetX( flt3, val );
	case 1:
		flt3 = DirectX::XMVectorSetY( flt3, val );
	case 2:
		flt3 = DirectX::XMVectorSetZ( flt3, val );
	default:
		Assert( 0 );
	}
}

static bool PointInsidePlane( DirectX::XMVECTOR *pVert, int normalInd, float val, bool nearClip )
{
	if ( ( val > 0 ) || nearClip )
		return ( val - XMVec3Component( *pVert, normalInd ) >= 0 );
	else
		return ( XMVec3Component( *pVert, normalInd ) - val >= 0 );
}

static void IntersectPlane( DirectX::XMVECTOR *pStart, DirectX::XMVECTOR *pEnd,
			    int normalInd, float val, DirectX::XMVECTOR *pOutVert )
{
	DirectX::XMVECTOR dir;
	dir = DirectX::XMVectorSubtract( *pEnd, *pStart );
	Assert( XMVec3Component( dir, normalInd ) != 0.0f );
	float t = ( val - XMVec3Component( *pStart, normalInd ) ) / XMVec3Component( dir, normalInd );
	XMVec3SetComponent( *pOutVert, 0, XMVec3Component( *pStart, 0 ) + XMVec3Component( dir, 0 ) * t );
	XMVec3SetComponent( *pOutVert, 1, XMVec3Component( *pStart, 1 ) + XMVec3Component( dir, 1 ) * t );
	XMVec3SetComponent( *pOutVert, 2, XMVec3Component( *pStart, 2 ) + XMVec3Component( dir, 2 ) * t );

	// Avoid any precision problems.
	XMVec3SetComponent( *pOutVert, normalInd, val );
}

static int ClipTriangleAgainstPlane( DirectX::XMVECTOR **ppVert, int nVertexCount,
				     DirectX::XMVECTOR **ppOutVert, int normalInd, float val, bool nearClip = false )
{
	// Ye Olde Sutherland-Hodgman clipping algorithm
	int numOutVerts = 0;
	DirectX::XMVECTOR *pStart = ppVert[nVertexCount - 1];
	bool startInside = PointInsidePlane( pStart, normalInd, val, nearClip );
	for ( int i = 0; i < nVertexCount; ++i )
	{
		DirectX::XMVECTOR *pEnd = ppVert[i];
		bool endInside = PointInsidePlane( pEnd, normalInd, val, nearClip );
		if ( endInside )
		{
			if ( !startInside )
			{
				IntersectPlane( pStart, pEnd, normalInd, val, &g_ClipVerts[g_NumClipVerts] );
				ppOutVert[numOutVerts++] = &g_ClipVerts[g_NumClipVerts++];
			}
			ppOutVert[numOutVerts++] = pEnd;
		}
		else
		{
			if ( startInside )
			{
				IntersectPlane( pStart, pEnd, normalInd, val, &g_ClipVerts[g_NumClipVerts] );
				ppOutVert[numOutVerts++] = &g_ClipVerts[g_NumClipVerts++];
			}
		}
		pStart = pEnd;
		startInside = endInside;
	}

	return numOutVerts;
}

void CTempMeshDX11::ClipTriangle( DirectX::XMVECTOR **ppVert, float zNear, DirectX::XMMATRIX &projection )
{
	int i;
	int nVertexCount = 3;
	DirectX::XMVECTOR *ppClipVert1[10];
	DirectX::XMVECTOR *ppClipVert2[10];

	g_NumClipVerts = 0;

	// Clip against the near plane in view space to prevent negative w.
	// Clip against each plane
	nVertexCount = ClipTriangleAgainstPlane( ppVert, nVertexCount, ppClipVert1, 2, zNear, true );
	if ( nVertexCount < 3 )
		return;

	// Sucks that I have to do this, but I have to clip near plane in view space 
	// Clipping in projection space is screwy when w < 0
	// Transform the clipped points into projection space
	Assert( g_NumClipVerts <= 2 );
	for ( i = 0; i < nVertexCount; ++i )
	{
		if ( ppClipVert1[i] == &g_ClipVerts[0] )
		{
			g_ClipVerts[0] = DirectX::XMVector3TransformCoord( *ppClipVert1[i], projection );
		}
		else if ( ppClipVert1[i] == &g_ClipVerts[1] )
		{
			g_ClipVerts[1] = DirectX::XMVector3TransformCoord( *ppClipVert1[i], projection );
		}
		else
		{
			g_ClipVerts[g_NumClipVerts] = DirectX::XMVector3TransformCoord( *ppClipVert1[i], projection );
			ppClipVert1[i] = &g_ClipVerts[g_NumClipVerts];
			++g_NumClipVerts;
		}
	}

	nVertexCount = ClipTriangleAgainstPlane( ppClipVert1, nVertexCount, ppClipVert2, 2, 1.0f );
	if ( nVertexCount < 3 )
		return;

	nVertexCount = ClipTriangleAgainstPlane( ppClipVert2, nVertexCount, ppClipVert1, 0, 1.0f );
	if ( nVertexCount < 3 )
		return;

	nVertexCount = ClipTriangleAgainstPlane( ppClipVert1, nVertexCount, ppClipVert2, 0, -1.0f );
	if ( nVertexCount < 3 )
		return;

	nVertexCount = ClipTriangleAgainstPlane( ppClipVert2, nVertexCount, ppClipVert1, 1, 1.0f );
	if ( nVertexCount < 3 )
		return;

	nVertexCount = ClipTriangleAgainstPlane( ppClipVert1, nVertexCount, ppClipVert2, 1, -1.0f );
	if ( nVertexCount < 3 )
		return;

#ifdef DRAW_SELECTION
	if ( 1 || g_bDrawSelection )
	{
		srand( *(int *)( &ppClipVert2[0]->x ) );
		unsigned char r = (unsigned char)( rand() * 191.0f / RAND_MAX ) + 64;
		unsigned char g = (unsigned char)( rand() * 191.0f / RAND_MAX ) + 64;
		unsigned char b = (unsigned char)( rand() * 191.0f / RAND_MAX ) + 64;

		ShaderAPI()->SetupSelectionModeVisualizationState();

		CMeshBuilder *pMeshBuilder = ShaderAPI()->GetVertexModifyBuilder();
		IMesh *pMesh = GetDynamicMesh();
		pMeshBuilder->Begin( pMesh, MATERIAL_POLYGON, nVertexCount );

		for ( i = 0; i < nVertexCount; ++i )
		{
			pMeshBuilder->Position3fv( *ppClipVert2[i] );
			pMeshBuilder->Color3ub( r, g, b );
			pMeshBuilder->AdvanceVertex();
		}

		pMeshBuilder->End();
		pMesh->Draw();

		pMeshBuilder->Begin( pMesh, MATERIAL_LINE_LOOP, nVertexCount );

		for ( i = 0; i < nVertexCount; ++i )
		{
			pMeshBuilder->Position3fv( *ppClipVert2[i] );
			pMeshBuilder->Color3ub( 255, 255, 255 );
			pMeshBuilder->AdvanceVertex();
		}

		pMeshBuilder->End();
		pMesh->Draw();
	}
#endif

	// Compute closest and furthest verts
	float minz = XMVec3Component( *ppClipVert2[0], 2 );
	float maxz = minz;
	for ( i = 1; i < nVertexCount; ++i )
	{
		if ( XMVec3Component( *ppClipVert2[i], 2 ) < minz )
			minz = XMVec3Component( *ppClipVert2[i], 2 );
		else if ( XMVec3Component( *ppClipVert2[i], 2 ) > maxz )
			maxz = XMVec3Component( *ppClipVert2[i], 2 );
	}

	ShaderAPI()->RegisterSelectionHit( minz, maxz );
}

//-----------------------------------------------------------------------------
// Selection mode 
//-----------------------------------------------------------------------------
void CTempMeshDX11::TestSelection()
{
	// Note that this doesn't take into account any vertex modification
	// done in a vertex shader. Also it doesn't take into account any clipping
	// done in hardware

	// Blow off points and lines; they don't matter
	if ( ( m_Type != MATERIAL_TRIANGLES ) && ( m_Type != MATERIAL_TRIANGLE_STRIP ) )
		return;

	DirectX::XMMATRIX modelToView, projection;
	ComputeModelToView( modelToView );
	ShaderAPI()->GetMatrix( MATERIAL_PROJECTION, projection );
	float zNear = 0.1f;// -projection.m[3][2] / projection.m[2][2]; // FIXME

	//DirectX::XMMatrix

	DirectX::XMVECTOR *pPos[3];
	DirectX::XMVECTOR edge[2];
	DirectX::XMVECTOR normal;

	int numTriangles;
	if ( m_Type == MATERIAL_TRIANGLES )
		numTriangles = m_IndexData.Count() / 3;
	else
		numTriangles = m_IndexData.Count() - 2;

	float cullFactor = ComputeCullFactor();

	// Makes the lovely loop simpler
	if ( m_Type == MATERIAL_TRIANGLE_STRIP )
		cullFactor *= -1.0f;

	// We'll need some temporary memory to tell us if we're transformed the vert
	int nVertexCount = m_VertexData.Count() / m_VertexSize;
	static CUtlVector< unsigned char > transformedVert;
	int transformedVertSize = ( nVertexCount + 7 ) >> 3;
	transformedVert.RemoveAll();
	transformedVert.EnsureCapacity( transformedVertSize );
	transformedVert.AddMultipleToTail( transformedVertSize );
	memset( transformedVert.Base(), 0, transformedVertSize );

	int indexPos;
	for ( int i = 0; i < numTriangles; ++i )
	{
		// Get the three indices
		if ( m_Type == MATERIAL_TRIANGLES )
		{
			indexPos = i * 3;
		}
		else
		{
			Assert( m_Type == MATERIAL_TRIANGLE_STRIP );
			cullFactor *= -1.0f;
			indexPos = i;
		}

		// BAH. Gotta clip to the near clip plane in view space to prevent
		// negative w coords; negative coords throw off the projection-space clipper.

		// Get the three positions in view space
		int inFrontIdx = -1;
		for ( int j = 0; j < 3; ++j )
		{
			int index = m_IndexData[indexPos];
			DirectX::XMVECTOR *pPosition = ( DirectX::XMVECTOR *)&m_VertexData[index * m_VertexSize];
			if ( ( transformedVert[index >> 3] & ( 1 << ( index & 0x7 ) ) ) == 0 )
			{
				*pPosition = DirectX::XMVector3TransformCoord( *pPosition, modelToView );
				//D3DXVec3TransformCoord( pPosition, pPosition, &modelToView );
				transformedVert[index >> 3] |= ( 1 << ( index & 0x7 ) );
			}

			pPos[j] = pPosition;
			if ( XMVec3Component( *pPos[j], 2 ) < 0.0f )
				inFrontIdx = j;
			++indexPos;
		}

		// all points are behind the camera
		if ( inFrontIdx < 0 )
			continue;

		// backface cull....
		edge[0] = DirectX::XMVectorSubtract( *pPos[1], *pPos[0] );
		//D3DXVec3Subtract( &edge[0], pPos[1], pPos[0] );
		edge[1] = DirectX::XMVectorSubtract( *pPos[2], *pPos[0] );
		//D3DXVec3Subtract( &edge[1], pPos[2], pPos[0] );
		normal = DirectX::XMVector3Cross( edge[0], edge[1] );
		//D3DXVec3Cross( &normal, &edge[0], &edge[1] );
		float dot = DirectX::XMVectorGetX( DirectX::XMVector3Dot( normal, *pPos[inFrontIdx] ) );
		//float dot = D3DXVec3Dot( &normal, pPos[inFrontIdx] );
		if ( dot * cullFactor > 0.0f )
			continue;

		// Clip to viewport
		ClipTriangle( pPos, zNear, projection );
	}
}

//-----------------------------------------------------------------------------
// Begins a render pass
//-----------------------------------------------------------------------------
void CTempMeshDX11::BeginPass()
{
	Assert( !m_InPass );

#ifdef _DEBUG
	m_InPass = true;
#endif

	CMeshBuilder *pMeshBuilder = ShaderAPI()->GetVertexModifyBuilder();

	CDynamicMeshDX11 *pMesh = GetDynamicMesh();

	int nIndexCount;
	int nFirstIndex;
	if ( ( s_FirstIndex == -1 ) && ( s_NumIndices == 0 ) )
	{
		nIndexCount = m_IndexData.Count();
		nFirstIndex = 0;
	}
	else
	{
		nIndexCount = s_NumIndices;
		nFirstIndex = s_FirstIndex;
	}

	int i;
	int nVertexCount = m_VertexData.Count() / m_VertexSize;
	pMeshBuilder->Begin( pMesh, m_Type, nVertexCount, nIndexCount );

	// Copy in the vertex data...
	// Note that since we pad the vertices, it's faster for us to simply
	// copy the fields we're using...
	Assert( pMeshBuilder->BaseVertexData() );
	memcpy( pMeshBuilder->BaseVertexData(), m_VertexData.Base(), m_VertexData.Count() );
	pMeshBuilder->AdvanceVertices( m_VertexData.Count() / m_VertexSize );

	for ( i = 0; i < nIndexCount; ++i )
	{
		pMeshBuilder->Index( m_IndexData[nFirstIndex + i] );
		pMeshBuilder->AdvanceIndex();
	}

	// NOTE: The client is expected to modify the data after this call is made
	pMeshBuilder->Reset();
}

//-----------------------------------------------------------------------------
// Draws a single pass
//-----------------------------------------------------------------------------
void CTempMeshDX11::RenderPass()
{
	Assert( m_InPass );

#ifdef _DEBUG
	m_InPass = false;
#endif

	// Have the shader API modify the vertex data as it needs
	// This vertex data is modified based on state set by the material
	//ShaderAPI()->ModifyVertexData();

	// Done building the mesh
	ShaderAPI()->GetVertexModifyBuilder()->End();

	// Have the dynamic mesh render a single pass...
	GetDynamicMesh()->DrawSinglePassImmediately();
}

//-----------------------------------------------------------------------------
//
// Buffered mesh implementation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CBufferedMeshDX11::CBufferedMeshDX11() : m_IsFlushing( false ), m_WasRendered( true )
{
	m_pMesh = NULL;
#ifdef DEBUG_BUFFERED_STATE
	m_BufferedStateSet = false;
#endif
}

CBufferedMeshDX11::~CBufferedMeshDX11()
{
}


//-----------------------------------------------------------------------------
// Sets the mesh
//-----------------------------------------------------------------------------
void CBufferedMeshDX11::SetMesh( CBaseMeshDX11 *pMesh )
{
	if ( m_pMesh != pMesh )
	{
		ShaderAPI()->FlushBufferedPrimitives();
		m_pMesh = pMesh;
	}
}


//-----------------------------------------------------------------------------
// Spews the mesh data
//-----------------------------------------------------------------------------
void CBufferedMeshDX11::Spew( int nVertexCount, int nIndexCount, const MeshDesc_t &spewDesc )
{
	if ( m_pMesh )
	{
		m_pMesh->Spew( nVertexCount, nIndexCount, spewDesc );
	}
}


//-----------------------------------------------------------------------------
// Sets the material
//-----------------------------------------------------------------------------
void CBufferedMeshDX11::SetVertexFormat( VertexFormat_t format )
{
	Assert( m_pMesh );
	bool bReset = m_pMesh->NeedsVertexFormatReset( format );
	if ( bReset )
	{
		ShaderAPI()->FlushBufferedPrimitives();
		m_pMesh->SetVertexFormat( format );
	}
}

void CBufferedMeshDX11::SetMorphFormat( MorphFormat_t format )
{
	Assert( m_pMesh );
	m_pMesh->SetMorphFormat( format );
}

VertexFormat_t CBufferedMeshDX11::GetVertexFormat() const
{
	Assert( m_pMesh );
	return m_pMesh->GetVertexFormat();
}

void CBufferedMeshDX11::SetMaterial( IMaterial *pMaterial )
{
#if _DEBUG
	Assert( m_pMesh );
	m_pMesh->SetMaterial( pMaterial );
#endif
}

void CBufferedMeshDX11::ValidateData( int nVertexCount, int nIndexCount, const MeshDesc_t &spewDesc )
{
#if _DEBUG
	Assert( m_pMesh );
	m_pMesh->ValidateData( nVertexCount, nIndexCount, spewDesc );
#endif
}


//-----------------------------------------------------------------------------
// Sets the flex mesh to render with this mesh
//-----------------------------------------------------------------------------
void CBufferedMeshDX11::SetFlexMesh( IMesh *pMesh, int nVertexOffsetInBytes )
{
	// FIXME: Probably are situations where we don't need to flush,
	// but this is going to look different in a very short while, so I'm not going to bother
	ShaderAPI()->FlushBufferedPrimitives();
	m_pMesh->SetFlexMesh( pMesh, nVertexOffsetInBytes );
}


//-----------------------------------------------------------------------------
// checks to see if it was rendered..
//-----------------------------------------------------------------------------
void CBufferedMeshDX11::ResetRendered()
{
	m_WasRendered = false;
}

bool CBufferedMeshDX11::WasNotRendered() const
{
	return !m_WasRendered;
}

//-----------------------------------------------------------------------------
// "Draws" it
//-----------------------------------------------------------------------------
void CBufferedMeshDX11::Draw( int nFirstIndex, int nIndexCount )
{
	if ( !ShaderUtil()->OnDrawMesh( this, nFirstIndex, nIndexCount ) )
	{
		m_WasRendered = true;
		MarkAsDrawn();
		return;
	}

	Assert( !m_IsFlushing && !m_WasRendered );

	// Gotta draw all of the buffered mesh
	Assert( ( nFirstIndex == -1 ) && ( nIndexCount == 0 ) );

	// No need to draw it more than once...
	m_WasRendered = true;

	// We've got something to flush
	m_FlushNeeded = true;

	// Less than 0 indices indicates we were using a standard buffer
	if ( m_pMesh->HasFlexMesh() || !ShaderUtil()->GetConfig().bBufferPrimitives )
	{
		ShaderAPI()->FlushBufferedPrimitives();
	}
}


//-----------------------------------------------------------------------------
// Sets the primitive mode
//-----------------------------------------------------------------------------

void CBufferedMeshDX11::SetPrimitiveType( MaterialPrimitiveType_t type )
{
	Assert( IsX360() || ( type != MATERIAL_INSTANCED_QUADS ) );
	Assert( type != MATERIAL_HETEROGENOUS );

	if ( type != GetPrimitiveType() )
	{
		ShaderAPI()->FlushBufferedPrimitives();
		m_pMesh->SetPrimitiveType( type );
	}
}

MaterialPrimitiveType_t CBufferedMeshDX11::GetPrimitiveType() const
{
	return m_pMesh->GetPrimitiveType();
}

//-----------------------------------------------------------------------------
// Locks/unlocks the entire mesh
//-----------------------------------------------------------------------------
void CBufferedMeshDX11::LockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc )
{
	ShaderUtil()->SyncMatrices();

	Assert( m_pMesh );
	Assert( m_WasRendered );

	// Do some pre-lock processing
	m_pMesh->PreLock();

	// for tristrips, gotta make degenerate ones...
	m_ExtraIndices = 0;
	bool tristripFixup = ( m_pMesh->IndexCount() != 0 ) &&
		( m_pMesh->GetPrimitiveType() == MATERIAL_TRIANGLE_STRIP );
	if ( tristripFixup )
	{
		m_ExtraIndices = ( m_pMesh->IndexCount() & 0x1 ) != 0 ? 3 : 2;
		nIndexCount += m_ExtraIndices;
	}

	// Flush if we gotta
	if ( !m_pMesh->HasEnoughRoom( nVertexCount, nIndexCount ) )
	{
		ShaderAPI()->FlushBufferedPrimitives();
	}

	m_pMesh->LockMesh( nVertexCount, nIndexCount, desc );

//	This is taken care of in the function above.
//	CBaseMeshDX8::m_bMeshLocked = true;

	// Deal with fixing up the tristrip..
	if ( tristripFixup && desc.m_nIndexSize )
	{
		char buf[32];
		if ( DebugTrace() )
		{
			if ( m_ExtraIndices == 3 )
				sprintf( buf, "Link Index: %d %d\n", m_LastIndex, m_LastIndex );
			else
				sprintf( buf, "Link Index: %d\n", m_LastIndex );
			Plat_DebugString( buf );
		}
		*desc.m_pIndices++ = m_LastIndex;
		if ( m_ExtraIndices == 3 )
		{
			*desc.m_pIndices++ = m_LastIndex;
		}

		// Leave room for the last padding index
		++desc.m_pIndices;
	}

	m_WasRendered = false;

#ifdef DEBUG_BUFFERED_MESHES
	if ( m_BufferedStateSet )
	{
		BufferedState_t compare;
		ShaderAPI()->GetBufferedState( compare );
		Assert( !memcmp( &compare, &m_BufferedState, sizeof( compare ) ) );
	}
	else
	{
		ShaderAPI()->GetBufferedState( m_BufferedState );
		m_BufferedStateSet = true;
	}
#endif
}


//-----------------------------------------------------------------------------
// Locks/unlocks the entire mesh
//-----------------------------------------------------------------------------
void CBufferedMeshDX11::UnlockMesh( int nVertexCount, int nIndexCount, MeshDesc_t &desc )
{
	Assert( m_pMesh );

	// Gotta fix up the first index to batch strips reasonably
	if ( ( m_pMesh->GetPrimitiveType() == MATERIAL_TRIANGLE_STRIP ) && desc.m_nIndexSize )
	{
		if ( m_ExtraIndices > 0 )
		{
			*( desc.m_pIndices - 1 ) = *desc.m_pIndices;

			if ( DebugTrace() )
			{
				char buf[32];
				sprintf( buf, "Link Index: %d\n", *desc.m_pIndices );
				Plat_DebugString( buf );
			}
		}

		// Remember the last index for next time 
		m_LastIndex = desc.m_pIndices[nIndexCount - 1];

		nIndexCount += m_ExtraIndices;
	}

	m_pMesh->UnlockMesh( nVertexCount, nIndexCount, desc );

//	This is taken care of in the function above.
//	CBaseMeshDX8::m_bMeshLocked = false;
}

//-----------------------------------------------------------------------------
// Renders a pass
//-----------------------------------------------------------------------------

void CBufferedMeshDX11::RenderPass()
{
	// this should never be called!
	Assert( 0 );
}

//-----------------------------------------------------------------------------
// Flushes queued data
//-----------------------------------------------------------------------------

void CBufferedMeshDX11::Flush()
{
	// If you are hitting this assert you are causing a flush between a 
	// meshbuilder begin/end and you are more than likely losing rendering data.
	AssertOnce( !m_bMeshLocked );

	if ( m_pMesh && !m_IsFlushing && m_FlushNeeded )
	{
		VPROF( "CBufferedMeshDX11::Flush" );

#ifdef DEBUG_BUFFERED_MESHES
		if ( m_BufferedStateSet )
		{
			BufferedState_t compare;
			ShaderAPI()->GetBufferedState( compare );
			Assert( !memcmp( &compare, &m_BufferedState, sizeof( compare ) ) );
			m_BufferedStateSet = false;
		}
#endif

		m_IsFlushing = true;

		// Actually draws the data using the mesh's material
		static_cast<IMesh *>( m_pMesh )->Draw();

		m_IsFlushing = false;
		m_FlushNeeded = false;

		m_pMesh->SetFlexMesh( NULL, 0 );
	}
}

//-----------------------------------------------------------------------------
//
// Mesh manager implementation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CMeshMgr::CMeshMgr() :
	m_pDynamicIndexBuffer( 0 ),
	m_DynamicTempMesh( true ),
	m_pVertexIDBuffer( 0 ),
	m_pCurrentVertexBuffer( NULL ),
	m_CurrentVertexFormat( 0 ),
	m_pCurrentIndexBuffer( NULL ),
	m_DynamicIndexBuffer( SHADER_BUFFER_TYPE_DYNAMIC, MATERIAL_INDEX_FORMAT_16BIT, INDEX_BUFFER_SIZE, "dynamic" ),
	m_DynamicVertexBuffer( SHADER_BUFFER_TYPE_DYNAMIC, VERTEX_FORMAT_UNKNOWN, DYNAMIC_VERTEX_BUFFER_MEMORY, "dynamic" )
{
	m_bUseFatVertices = false;
	m_nIndexBufferOffset = 0;
	memset( m_pVertexBufferOffset, 0, sizeof( m_pVertexBufferOffset ) );
	memset( m_pCurrentVertexStride, 0, sizeof( m_pCurrentVertexStride ) );
	memset( m_pFirstVertex, 0, sizeof( m_pFirstVertex ) );
	memset( m_pVertexCount, 0, sizeof( m_pVertexCount ) );
	m_nUnusedVertexFields = 0;
	m_nUnusedTextureCoords = 0;
}

CMeshMgr::~CMeshMgr()
{
}

//-----------------------------------------------------------------------------
// Initialize, shutdown
//-----------------------------------------------------------------------------
void CMeshMgr::Init()
{
	m_DynamicMesh.Init( 0 );
	m_DynamicFlexMesh.Init( 1 );

	// The dynamic index buffer
	//m_pDynamicIndexBuffer = new CIndexBuffer( Dx9Device(), INDEX_BUFFER_SIZE, ShaderAPI()->UsingSoftwareVertexProcessing(), true );
	m_pDynamicIndexBuffer = static_cast<CIndexBufferDx11 *>(
		g_pShaderDeviceDx11->CreateIndexBuffer( SHADER_BUFFER_TYPE_DYNAMIC, MATERIAL_INDEX_FORMAT_16BIT, INDEX_BUFFER_SIZE, "dynamic" ) );
	//Log( "Dynamic ib index size: %i\n", m_pDynamicIndexBuffer->IndexSize() );

	// If we're running in vs3.0, allocate a vertexID buffer
	//CreateVertexIDBuffer();

	m_BufferedMode = !IsX360();
}

void CMeshMgr::Shutdown()
{
	CleanUp();
}

//-----------------------------------------------------------------------------
// Task switch...
//-----------------------------------------------------------------------------
void CMeshMgr::ReleaseBuffers()
{
	if ( IsPC() && mat_debugalttab.GetBool() )
	{
		Warning( "mat_debugalttab: CMeshMgr::ReleaseBuffers\n" );
	}

	CleanUp();
	m_DynamicMesh.Reset();
	m_DynamicFlexMesh.Reset();
}

void CMeshMgr::RestoreBuffers()
{
	if ( IsPC() && mat_debugalttab.GetBool() )
	{
		Warning( "mat_debugalttab: CMeshMgr::RestoreBuffers\n" );
	}
	Init();
}

//-----------------------------------------------------------------------------
// Cleans up vertex and index buffers
//-----------------------------------------------------------------------------
void CMeshMgr::CleanUp()
{
	if ( m_pDynamicIndexBuffer )
	{
		delete m_pDynamicIndexBuffer;
		m_pDynamicIndexBuffer = 0;
	}

	DestroyVertexBuffers();

	// If we're running in vs3.0, allocate a vertexID buffer
	//DestroyVertexIDBuffer();
}

//-----------------------------------------------------------------------------
// Unused vertex fields
//-----------------------------------------------------------------------------
void CMeshMgr::MarkUnusedVertexFields( unsigned int nFlags, int nTexCoordCount, bool *pUnusedTexCoords )
{
	m_nUnusedVertexFields = nFlags;
	m_nUnusedTextureCoords = 0;
	for ( int i = 0; i < nTexCoordCount; ++i )
	{
		if ( pUnusedTexCoords[i] )
		{
			m_nUnusedTextureCoords |= ( 1 << i );
		}
	}
}

//-----------------------------------------------------------------------------
// Is the mesh dynamic?
//-----------------------------------------------------------------------------
bool CMeshMgr::IsDynamicMesh( IMesh *pMesh ) const
{
	return ( pMesh == &m_DynamicMesh ) || ( pMesh == &m_DynamicFlexMesh );
}

bool CMeshMgr::IsBufferedDynamicMesh( IMesh *pMesh ) const
{
	return ( pMesh == &m_BufferedMesh );
}

bool CMeshMgr::IsDynamicVertexBuffer( IVertexBuffer *pVertexBuffer ) const
{
	return ( pVertexBuffer == &m_DynamicVertexBuffer );
}

bool CMeshMgr::IsDynamicIndexBuffer( IIndexBuffer *pIndexBuffer ) const
{
	return ( pIndexBuffer == &m_DynamicIndexBuffer );
}

//-----------------------------------------------------------------------------
// Discards the dynamic vertex and index buffer
//-----------------------------------------------------------------------------
void CMeshMgr::DiscardVertexBuffers()
{
	VPROF_BUDGET( "CMeshMgr::DiscardVertexBuffers", VPROF_BUDGETGROUP_SWAP_BUFFERS );
	// This shouldn't be necessary, but it seems to be on GeForce 2
	// It helps when running WC and the engine simultaneously.
	//ResetMeshRenderState();

	// Flush all dynamic vertex and index buffers the next time
	// Lock() is called on them.
	if ( !g_pShaderDeviceDx11->IsDeactivated() )
	{
		for ( int i = m_DynamicVertexBuffers.Count(); --i >= 0; )
		{
			m_DynamicVertexBuffers[i].m_pBuffer->Flush();
		}
		m_pDynamicIndexBuffer->Flush();
	}
}

//-----------------------------------------------------------------------------
// Releases all dynamic vertex buffers
//-----------------------------------------------------------------------------
void CMeshMgr::DestroyVertexBuffers()
{

	for ( int i = m_DynamicVertexBuffers.Count(); --i >= 0; )
	{
		if ( m_DynamicVertexBuffers[i].m_pBuffer )
		{
			// This will also unbind it
			g_pShaderDeviceDx11->DestroyVertexBuffer( m_DynamicVertexBuffers[i].m_pBuffer );
		}
	}

	m_DynamicVertexBuffers.RemoveAll();
	m_DynamicMesh.Reset();
	m_DynamicFlexMesh.Reset();
}


//-----------------------------------------------------------------------------
// Flushes the dynamic mesh
//-----------------------------------------------------------------------------
void CMeshMgr::Flush()
{
	if ( IsPC() )
	{
		m_BufferedMesh.Flush();
	}
}

//-----------------------------------------------------------------------------
// Creates, destroys static meshes
//-----------------------------------------------------------------------------
IMesh *CMeshMgr::CreateStaticMesh( VertexFormat_t format, const char *pTextureBudgetGroup, IMaterial *pMaterial )
{
	// FIXME: Use a fixed-size allocator
	CMeshDX11 *pNewMesh = new CMeshDX11( pTextureBudgetGroup );
	pNewMesh->SetVertexFormat( format );
	if ( pMaterial != NULL )
	{
		pNewMesh->SetMorphFormat( pMaterial->GetMorphFormat() );
		pNewMesh->SetMaterial( pMaterial );
	}
	return pNewMesh;
}

void CMeshMgr::DestroyStaticMesh( IMesh *pMesh )
{
	// Don't destroy the dynamic mesh!
	Assert( !IsDynamicMesh( pMesh ) );
	CBaseMeshDX11 *pMeshImp = static_cast<CBaseMeshDX11 *>( pMesh );
	if ( pMeshImp )
	{
		delete pMeshImp;
	}
}

//-----------------------------------------------------------------------------
// Copy a static mesh index buffer to a dynamic mesh index buffer
//-----------------------------------------------------------------------------
void CMeshMgr::CopyStaticMeshIndexBufferToTempMeshIndexBuffer( CTempMeshDX11 *pDstIndexMesh,
							       CMeshDX11 *pSrcIndexMesh )
{
	Assert( !pSrcIndexMesh->IsDynamic() );
	int nIndexCount = pSrcIndexMesh->IndexCount();

	CMeshBuilder dstMeshBuilder;
	dstMeshBuilder.Begin( pDstIndexMesh, pSrcIndexMesh->GetPrimitiveType(), 0, nIndexCount );
	CIndexBufferDx11 *srcIndexBuffer = pSrcIndexMesh->GetIndexBuffer();
	IndexDesc_t desc;
	bool ret = srcIndexBuffer->Lock( nIndexCount, false, desc );
	if ( ret )
	{
		int i;
		for ( i = 0; i < nIndexCount; i++ )
		{
			dstMeshBuilder.Index( desc.m_pIndices[i] );
			dstMeshBuilder.AdvanceIndex();
		}
	}
	
	srcIndexBuffer->Unlock( 0, desc );
	dstMeshBuilder.End();
}

//-----------------------------------------------------------------------------
// Gets at the *real* dynamic mesh
//-----------------------------------------------------------------------------
IMesh *CMeshMgr::GetActualDynamicMesh( VertexFormat_t format )
{
	m_DynamicMesh.SetVertexFormat( format );
	return &m_DynamicMesh;
}

IMesh *CMeshMgr::GetFlexMesh()
{
	if ( g_pMaterialSystemHardwareConfig->SupportsPixelShaders_2_b() )
	{
		// FIXME: Kinda ugly size.. 28 bytes
		m_DynamicFlexMesh.SetVertexFormat( VERTEX_POSITION | VERTEX_NORMAL | VERTEX_WRINKLE | VERTEX_FORMAT_USE_EXACT_FORMAT );
	}
	else
	{
		// Same size as a pair of float3s (24 bytes)
		m_DynamicFlexMesh.SetVertexFormat( VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_USE_EXACT_FORMAT );
	}
	return &m_DynamicFlexMesh;
}

//-----------------------------------------------------------------------------
// Gets at the dynamic mesh
//-----------------------------------------------------------------------------
IMesh *CMeshMgr::GetDynamicMesh( IMaterial *pMaterial, VertexFormat_t vertexFormat, int nHWSkinBoneCount,
				 bool buffered, IMesh *pVertexOverride, IMesh *pIndexOverride )
{
	Assert( ( pMaterial == NULL ) || ( (IMaterialInternal *)pMaterial )->IsRealTimeVersion() );

	if ( IsX360() )
	{
		buffered = false;
	}

	// Can't be buffered if we're overriding the buffers
	if ( pVertexOverride || pIndexOverride )
	{
		buffered = false;
	}

	// When going from buffered to unbuffered mode, need to flush..
	if ( ( m_BufferedMode != buffered ) && m_BufferedMode )
	{
		m_BufferedMesh.SetMesh( 0 );
	}
	m_BufferedMode = buffered;

	IMaterialInternal *pMatInternal = static_cast<IMaterialInternal *>( pMaterial );

	bool needTempMesh = ShaderAPI()->IsInSelectionMode();

#ifdef DRAW_SELECTION
	if ( g_bDrawSelection )
	{
		needTempMesh = true;
	}
#endif

	CBaseMeshDX11 *pMesh;

	if ( needTempMesh )
	{
		// These haven't been implemented yet for temp meshes!
		// I'm not a hundred percent sure how to implement them; it would
		// involve a lock and a copy at least, which would stall the entire
		// rendering pipeline.
		Assert( !pVertexOverride );

		if ( pIndexOverride )
		{
			CopyStaticMeshIndexBufferToTempMeshIndexBuffer( &m_DynamicTempMesh,
				(CMeshDX11 *)pIndexOverride );
		}
		pMesh = &m_DynamicTempMesh;
	}
	else
	{
		pMesh = &m_DynamicMesh;
	}

	if ( m_BufferedMode )
	{
		Assert( !m_BufferedMesh.WasNotRendered() );
		m_BufferedMesh.SetMesh( pMesh );
		pMesh = &m_BufferedMesh;
	}

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
		CBaseMeshDX11 *pDX8Mesh = static_cast<CBaseMeshDX11 *>( pVertexOverride );
		pMesh->SetVertexFormat( pDX8Mesh->GetVertexFormat() );
	}
	pMesh->SetMorphFormat( pMatInternal->GetMorphFormat() );
	pMesh->SetMaterial( pMatInternal );

	// Note this works because we're guaranteed to not be using a buffered mesh
	// when we have overrides on
	// FIXME: Make work for temp meshes
	if ( pMesh == &m_DynamicMesh )
	{
		CBaseMeshDX11 *pBaseVertex = static_cast<CBaseMeshDX11 *>( pVertexOverride );
		if ( pBaseVertex )
		{
			m_DynamicMesh.OverrideVertexBuffer( pBaseVertex->GetVertexBuffer() );
		}

		CBaseMeshDX11 *pBaseIndex = static_cast<CBaseMeshDX11 *>( pIndexOverride );
		if ( pBaseIndex )
		{
			m_DynamicMesh.OverrideIndexBuffer( pBaseIndex->GetIndexBuffer() );
		}
	}

	return pMesh;
}


//-----------------------------------------------------------------------------
// Used to construct vertex data
//-----------------------------------------------------------------------------
void CMeshMgr::ComputeVertexDescription( unsigned char *pBuffer,
					 VertexFormat_t vertexFormat, MeshDesc_t &desc ) const
{
	ComputeVertexDesc( pBuffer, vertexFormat, (VertexDesc_t &)desc );
}

//-----------------------------------------------------------------------------
// Computes the vertex format
//-----------------------------------------------------------------------------
VertexFormat_t CMeshMgr::ComputeVertexFormat( unsigned int flags,
					      int nTexCoordArraySize, int *pTexCoordDimensions, int numBoneWeights,
					      int userDataSize ) const
{
	// Construct a bitfield that makes sense and is unique from the standard FVF formats
	VertexFormat_t fmt = flags & ~VERTEX_FORMAT_USE_EXACT_FORMAT;

	if ( g_pHardwareConfig->SupportsCompressedVertices() == VERTEX_COMPRESSION_NONE )
	{
		// Vertex compression is disabled - make sure all materials
		// say "No!" to compressed verts ( tested in IsValidVertexFormat() )
		fmt &= ~VERTEX_FORMAT_COMPRESSED;
	}

	// This'll take 3 bits at most
	Assert( numBoneWeights <= 4 );

	if ( numBoneWeights > 0 )
	{
		fmt |= VERTEX_BONEWEIGHT( 2 ); // Always exactly two weights
	}

	// Size is measured in # of floats
	Assert( userDataSize <= 4 );
	fmt |= VERTEX_USERDATA_SIZE( userDataSize );

	// NOTE: If pTexCoordDimensions isn't specified, then nTexCoordArraySize
	// is interpreted as meaning that we have n 2D texcoords in the first N texcoord slots
	nTexCoordArraySize = min( nTexCoordArraySize, VERTEX_MAX_TEXTURE_COORDINATES );
	for ( int i = 0; i < nTexCoordArraySize; ++i )
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
// Use fat vertices (for tools)
//-----------------------------------------------------------------------------
void CMeshMgr::UseFatVertices( bool bUseFat )
{
	m_bUseFatVertices = bUseFat;
}

//-----------------------------------------------------------------------------
// Returns the number of vertices we can render using the dynamic mesh
//-----------------------------------------------------------------------------
void CMeshMgr::GetMaxToRender( IMesh *pMesh, bool bMaxUntilFlush, int *pMaxVerts, int *pMaxIndices )
{
	CBaseMeshDX11 *pBaseMesh = static_cast<CBaseMeshDX11 *>( pMesh );
	if ( !pBaseMesh )
	{
		*pMaxVerts = 0;
		*pMaxIndices = m_pDynamicIndexBuffer->IndexCount();
		return;
	}

	if ( IsBufferedDynamicMesh( pMesh ) )
	{
		pBaseMesh = ( CBaseMeshDX11 * )static_cast<CBufferedMeshDX11 *>( pBaseMesh )->GetMesh();
		pMesh = pBaseMesh;
	}

	// Static mesh? Max you can use is 65535
	if ( !IsDynamicMesh( pMesh ) )
	{
		*pMaxVerts = 65535;
		*pMaxIndices = 65535;
		return;
	}

	CVertexBufferDx11 *pVertexBuffer = pBaseMesh->GetVertexBuffer();
	CIndexBufferDx11 *pIndexBuffer = pBaseMesh->GetIndexBuffer();

	if ( !pVertexBuffer )
	{
		*pMaxVerts = 0;
		*pMaxIndices = 0;
		return;
	}

	if ( !bMaxUntilFlush )
	{
		*pMaxVerts = ShaderAPI()->GetCurrentDynamicVBSize() / pVertexBuffer->VertexSize();
		if ( *pMaxVerts > 65535 )
		{
			*pMaxVerts = 65535;
		}
		*pMaxIndices = pIndexBuffer ? pIndexBuffer->IndexCount() : 0;
		return;
	}

	*pMaxVerts = pVertexBuffer->GetRoomRemaining();
	*pMaxIndices = pIndexBuffer ? pIndexBuffer->IndexCount() - pIndexBuffer->IndexPosition() : 0;
	if ( *pMaxVerts == 0 )
	{
		*pMaxVerts = ShaderAPI()->GetCurrentDynamicVBSize() / pVertexBuffer->VertexSize();
	}
	if ( *pMaxVerts > 65535 )
	{
		*pMaxVerts = 65535;
	}
	if ( *pMaxIndices == 0 )
	{
		*pMaxIndices = pIndexBuffer ? pIndexBuffer->IndexCount() : 0;
	}
}

int CMeshMgr::GetMaxVerticesToRender( IMaterial *pMaterial )
{
	Assert( ( pMaterial == NULL ) || ( (IMaterialInternal *)pMaterial )->IsRealTimeVersion() );

	// Be conservative, assume no compression (in here, we don't know if the caller will used a compressed VB or not)
	// FIXME: allow the caller to specify which compression type should be used to compute size from the vertex format
	//        (this can vary between multiple VBs/Meshes using the same material)
	VertexFormat_t fmt = pMaterial->GetVertexFormat() & ~VERTEX_FORMAT_COMPRESSED;
	int nMaxVerts = ShaderAPI()->GetCurrentDynamicVBSize() / VertexFormatSize( fmt );
	if ( nMaxVerts > 65535 )
	{
		nMaxVerts = 65535;
	}
	return nMaxVerts;
}

int CMeshMgr::GetMaxIndicesToRender()
{
	return INDEX_BUFFER_SIZE;
}

//-----------------------------------------------------------------------------
// Returns a vertex buffer appropriate for the flags
//-----------------------------------------------------------------------------
CVertexBufferDx11 *CMeshMgr::FindOrCreateVertexBuffer( int nDynamicBufferId, VertexFormat_t vertexFormat )
{
	int vertexSize = VertexFormatSize( vertexFormat );

	while ( m_DynamicVertexBuffers.Count() <= nDynamicBufferId )
	{
		// Track VB allocations (override any prior allocator string set higher up on the callstack)
		g_VBAllocTracker->TrackMeshAllocations( NULL );
		g_VBAllocTracker->TrackMeshAllocations( "CMeshMgr::FindOrCreateVertexBuffer (dynamic VB)" );

		// create the single 1MB dynamic vb that will be shared amongst all consumers
		// the correct thing is to use the largest expected vertex format size of max elements, but this
		// creates an undesirably large buffer - instead create the buffer we want, and fix consumers that bork
		// NOTE: GetCurrentDynamicVBSize returns a smaller value during level transitions
		int nBufferMemory = ShaderAPI()->GetCurrentDynamicVBSize();
		int nIndex = m_DynamicVertexBuffers.AddToTail();
		m_DynamicVertexBuffers[nIndex].m_VertexSize = 0;
		//m_DynamicVertexBuffers[nIndex].m_pBuffer = new CVertexBuffer( Dx9Device(), 0, 0,
		//							      nBufferMemory / VERTEX_BUFFER_SIZE, VERTEX_BUFFER_SIZE, TEXTURE_GROUP_STATIC_VERTEX_BUFFER_OTHER, ShaderAPI()->UsingSoftwareVertexProcessing(), true );
		m_DynamicVertexBuffers[nIndex].m_pBuffer = static_cast<CVertexBufferDx11 *>(
			g_pShaderDeviceDx11->CreateVertexBuffer( SHADER_BUFFER_TYPE_DYNAMIC, vertexFormat, nBufferMemory / vertexSize, "dynamicvb" ) );

		g_VBAllocTracker->TrackMeshAllocations( NULL );
	}

	if ( m_DynamicVertexBuffers[nDynamicBufferId].m_VertexSize != vertexSize )
	{
		// provide caller with dynamic vb in expected format
		// NOTE: GetCurrentDynamicVBSize returns a smaller value during level transitions
		int nBufferMemory = ShaderAPI()->GetCurrentDynamicVBSize();
		m_DynamicVertexBuffers[nDynamicBufferId].m_VertexSize = vertexSize;
		m_DynamicVertexBuffers[nDynamicBufferId].m_pBuffer->ChangeConfiguration( vertexSize, nBufferMemory );

		// size changed means stream stride needs update
		// mark cached stream state as invalid to reset stream
		//if ( nDynamicBufferId == 0 )
		//{
		//	g_pLastVertex = NULL;
		//}
	}

	return m_DynamicVertexBuffers[nDynamicBufferId].m_pBuffer;
}

CIndexBufferDx11 *CMeshMgr::GetDynamicIndexBuffer()
{
	return m_pDynamicIndexBuffer;
}

IVertexBuffer *CMeshMgr::GetDynamicVertexBuffer( IMaterial *pMaterial, bool buffered )
{
	Assert( 0 );
	return NULL;
//	return ( IMeshDX8 * )GetDynamicMesh( pMaterial, buffered, NULL, NULL );
}

IIndexBuffer *CMeshMgr::GetDynamicIndexBuffer( IMaterial *pMaterial, bool buffered )
{
	Assert( 0 );
	return NULL;
//	return ( IMeshDX8 * )GetDynamicMesh( pMaterial, buffered, NULL, NULL );
}

// Do we need to specify the stream here in the case of locking multiple dynamic VBs on different streams?
IVertexBuffer *CMeshMgr::GetDynamicVertexBuffer( int streamID, VertexFormat_t vertexFormat, bool bBuffered )
{
	if ( IsX360() )
	{
		bBuffered = false;
	}

	if ( CompressionType( vertexFormat ) != VERTEX_COMPRESSION_NONE )
	{
		// UNDONE: support compressed dynamic meshes if needed (pro: less VB memory, con: time spent compressing)
		DebuggerBreak();
		return NULL;
	}

	// MESHFIXME
#if 0
	if ( ( m_BufferedMode != bBuffered ) && m_BufferedMode )
	{
		m_BufferedIndexBuffer.SetIndexBuffer( NULL );
	}
#endif

	m_BufferedMode = bBuffered;
	Assert( !m_BufferedMode ); // MESHFIXME: don't deal with buffered VBs yet.

	bool needTempMesh = ShaderAPI()->IsInSelectionMode();

#ifdef DRAW_SELECTION
	if ( g_bDrawSelection )
	{
		needTempMesh = true;
	}
#endif

	Assert( !needTempMesh ); // MESHFIXME: don't support temp meshes here yet.

	CVertexBufferDx11 *pVertexBuffer;

	if ( needTempMesh )
	{
		Assert( 0 ); // MESHFIXME: don't do this yet.
//		pVertexBuffer = &m_DynamicTempVertexBuffer;
		pVertexBuffer = NULL;
	}
	else
	{
		pVertexBuffer = &m_DynamicVertexBuffer;
	}

	if ( m_BufferedMode )
	{
		Assert( 0 ); // don't support this yet.
#if 0
		Assert( !m_BufferedMesh.WasNotRendered() );
		m_BufferedMesh.SetMesh( pMesh );
		pMesh = &m_BufferedMesh;
#endif
	}

	return pVertexBuffer;
}

IIndexBuffer *CMeshMgr::GetDynamicIndexBuffer( MaterialIndexFormat_t fmt, bool bBuffered )
{
	if ( IsX360() )
	{
		bBuffered = false;
	}

	m_BufferedMode = bBuffered;

	Assert( !m_BufferedMode );

#ifdef _DEBUG
	bool needTempMesh =
#endif
		ShaderAPI()->IsInSelectionMode();

#ifdef DRAW_SELECTION
	if ( g_bDrawSelection )
	{
		needTempMesh = true;
	}
#endif

	Assert( !needTempMesh ); // don't handle this yet. MESHFIXME

	CIndexBufferBase *pIndexBuffer = &m_DynamicIndexBuffer;
	return pIndexBuffer;
}

void CMeshMgr::RenderPassWithVertexAndIndexBuffers()
{

}