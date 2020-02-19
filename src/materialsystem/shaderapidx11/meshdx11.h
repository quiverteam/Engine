//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//


#ifndef MESHDX11_H
#define MESHDX11_H

#ifdef _WIN32
#pragma once
#endif

#include "shaderapidx9/meshbase.h"
#include "shaderapi/ishaderdevice.h"

//-----------------------------------------------------------------------------
// Dx11 implementation of a mesh
//-----------------------------------------------------------------------------
class CMeshDx11 : public CMeshBase
{
public:
	CMeshDx11();
	virtual ~CMeshDx11();

	// FIXME: Make this work! Unsupported methods of IIndexBuffer
	virtual bool Lock( int nMaxIndexCount, bool bAppend, IndexDesc_t& desc ) { Assert(0); return false; }
	virtual void Unlock( int nWrittenIndexCount, IndexDesc_t& desc ) { Assert(0); }
	virtual int GetRoomRemaining() const { Assert(0); return 0; }
	virtual void ModifyBegin( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t& desc ) { Assert(0); }
	virtual void ModifyEnd( IndexDesc_t& desc ) { Assert(0); }
	virtual void Spew( int nIndexCount, const IndexDesc_t & desc ) { Assert(0); }
	virtual void ValidateData( int nIndexCount, const IndexDesc_t &desc ) { Assert(0); }
	virtual bool Lock( int nVertexCount, bool bAppend, VertexDesc_t &desc ) { Assert(0); return false; }
	virtual void Unlock( int nVertexCount, VertexDesc_t &desc ) { Assert(0); }
	virtual void Spew( int nVertexCount, const VertexDesc_t &desc ) { Assert(0); }
	virtual void ValidateData( int nVertexCount, const VertexDesc_t & desc ) { Assert(0); }
	virtual bool IsDynamic() const { Assert(0); return false; }
	virtual void BeginCastBuffer( MaterialIndexFormat_t format ) { Assert(0); }
	virtual void BeginCastBuffer( VertexFormat_t format ) { Assert(0); }
	virtual void EndCastBuffer( ) { Assert(0); }

	void LockMesh( int numVerts, int numIndices, MeshDesc_t& desc );
	void UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc );

	void ModifyBeginEx( bool bReadOnly, int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc );
	void ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc );
	void ModifyEnd( MeshDesc_t& desc );

	// returns the # of vertices (static meshes only)
	int  VertexCount() const;

	virtual void BeginPass( ) {}
	virtual void RenderPass() {}
	virtual bool HasColorMesh() const { return false; }
	virtual bool IsUsingMorphData() const { return false; }
	virtual bool HasFlexMesh() const { return false; }

	// Sets the primitive type
	void SetPrimitiveType( MaterialPrimitiveType_t type );

	// Draws the entire mesh
	void Draw(int firstIndex, int numIndices);

	void Draw(CPrimList *pPrims, int nPrims);

	// Copy verts and/or indices to a mesh builder. This only works for temp meshes!
	virtual void CopyToMeshBuilder( 
		int iStartVert,		// Which vertices to copy.
		int nVerts, 
		int iStartIndex,	// Which indices to copy.
		int nIndices, 
		int indexOffset,	// This is added to each index.
		CMeshBuilder &builder );

	// Spews the mesh data
	void Spew( int numVerts, int numIndices, const MeshDesc_t & desc );

	void ValidateData( int numVerts, int numIndices, const MeshDesc_t & desc );

	// gets the associated material
	IMaterial* GetMaterial();

	void SetColorMesh( IMesh *pColorMesh, int nVertexOffset )
	{
	}


	virtual int IndexCount() const
	{
		return 0;
	}

	virtual MaterialIndexFormat_t IndexFormat() const
	{
		Assert( 0 );
		return MATERIAL_INDEX_FORMAT_UNKNOWN;
	}

	virtual void SetFlexMesh( IMesh *pMesh, int nVertexOffset ) {}

	virtual void DisableFlexMesh() {}

	virtual void MarkAsDrawn() {}

	virtual VertexFormat_t GetVertexFormat() const { return VERTEX_POSITION; }

	virtual IMesh *GetMesh()
	{
		return this;
	}

public:
	static VertexFormat_t ComputeVertexFormat( int nFlags, int nTexCoords, int* pTexCoordDimensions,
						   int nBoneWeights, int nUserDataSize );

private:
	enum
	{
		VERTEX_BUFFER_SIZE = 1024 * 1024
	};

	unsigned char* m_pVertexMemory;
};

#endif // MESHDX11_H

