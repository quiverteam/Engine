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

VertexFormat_t CMeshDx11::ComputeVertexFormat( int nFlags, int nTexCoords, int* pTexCoordDimensions,
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