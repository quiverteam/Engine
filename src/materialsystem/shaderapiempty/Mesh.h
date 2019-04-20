//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines basic mesh classes
//
// $NoKeywords: $
//
//===========================================================================//
#pragma once

#include "materialsystem/imesh.h"


/*

Defines a basic mesh class

Meshes have a vertex buffer and an index buffer.
The vertex buffer simply contains a list of verticies used in the mesh.
Index buffers contain indicies of verticies in the index buffer, which can then
be used to generate triangles from the verticies.

Mesh modification is a multithreaded operation, hence the Lock and Unlock functions
scattered throughout the class.

From reading this, it appears that dynamic meshes can be modified during runtime, and
static meshes cannot be modified. As for the technical reasoning behind this, I am not
completely sure, but OpenGL/DX provide different buffer types, shared and non-shared.
The shared buffers exist both in GPU and host memory and are required to be
coherent (like the caches in the CPU). Thus, data written to a shared buffer in host
memory must be duplicated into GPU memory (big performance no-no). Non-shared (static)
buffers exist only in the GPU, thus they don't need to be coherent with anything
in host memory. And that also means they cannot be written to from the host.
However, a GPU program might(?) be able to modify it.

*/
class CMesh : public IMesh
{
public:
	CMesh( bool bIsDynamic );
	virtual ~CMesh();

    /*

    */
	virtual bool Lock( int nMaxIndexCount, bool bAppend, IndexDesc_t& desc );

    /*

    */
	virtual void Unlock( int nWrittenIndexCount, IndexDesc_t& desc );

    /*

    */
	virtual void ModifyBegin( bool bReadOnly, int nFirstIndex, int nIndexCount, IndexDesc_t& desc );

    /*

    */
	virtual void ModifyEnd( IndexDesc_t& desc );

    /*
    Spews the data inside of an index buffer to some type of output stream
    nIndexCount is the number of vectors in the index buffer described by desc
    */
	virtual void Spew( int nIndexCount, const IndexDesc_t & desc );

    /*
    Validates data in the index buffer, used in debug mode
    */
	virtual void ValidateData( int nIndexCount, const IndexDesc_t &desc );

    /*

    */
	virtual bool Lock( int nVertexCount, bool bAppend, VertexDesc_t &desc );

    /*

    */
	virtual void Unlock( int nVertexCount, VertexDesc_t &desc );

    /*
    Spews data to an output stream about data in the vertex buffer
    */
	virtual void Spew( int nVertexCount, const VertexDesc_t &desc );

    /*
    Validates data in the vertex buffer, used in debug mode
    */
	virtual void ValidateData( int nVertexCount, const VertexDesc_t & desc );

    /*

    */
	virtual bool IsDynamic() const { return m_bIsDynamic; }

    /*

    */
	virtual void BeginCastBuffer( VertexFormat_t format ) {}

    /*

    */
	virtual void BeginCastBuffer( MaterialIndexFormat_t format ) {}

    /*

    */
	virtual void EndCastBuffer( ) {}

    /*
    Returns the room remaining in the vertex buffer
    */
	virtual int GetRoomRemaining() const { return 0; }

    /*
    Returns the index buffer format used
    */
	virtual MaterialIndexFormat_t IndexFormat() const { return MATERIAL_INDEX_FORMAT_UNKNOWN; }

    /*

    */
	void LockMesh( int numVerts, int numIndices, MeshDesc_t& desc );

    /*

    */
	void UnlockMesh( int numVerts, int numIndices, MeshDesc_t& desc );

    /*

    */
	void ModifyBeginEx( bool bReadOnly, int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc );

    /*

    */
	void ModifyBegin( int firstVertex, int numVerts, int firstIndex, int numIndices, MeshDesc_t& desc );

    /*

    */
	void ModifyEnd( MeshDesc_t& desc );

	/*
    Returns the number of verticies (static meshes only)
    */
	int  VertexCount() const;

	/*
    Sets the primitive type
    */
	void SetPrimitiveType( MaterialPrimitiveType_t type );
	 
	/*
    Draws the mesh starting from the first index continuing for numIndicies
    */
	void Draw(int firstIndex, int numIndices);

    /*
    Draws an array of the size nPrims specified by pPrims
    */
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

    /*
    Used in debug modes to validate data
    */
	void ValidateData( int numVerts, int numIndices, const MeshDesc_t & desc );

	// gets the associated material
	IMaterial* GetMaterial();

    /*

    */
	void SetColorMesh( IMesh *pColorMesh, int nVertexOffset )
	{
	}

    /*
    Returns the number of indicies
    */
	virtual int IndexCount() const
	{
		return 0;
	}

    /*
    Sets this mesh's flex mesh
    */
	virtual void SetFlexMesh( IMesh *pMesh, int nVertexOffset ) {}

    /*
    Disables flex mesh
    */
	virtual void DisableFlexMesh() {}

    /*
    Marks this mesh as drawn
    */
	virtual void MarkAsDrawn() {}

    /*
    Returns the vertex format of the vertex buffer
    */
	virtual VertexFormat_t GetVertexFormat() const { return VERTEX_POSITION; }

    /*
    Returns this mesh
    */
	virtual IMesh *GetMesh()
	{
		return this;
	}

private:
	enum
	{
		VERTEX_BUFFER_SIZE = 1024 * 1024
	};

	unsigned char* m_pVertexMemory;
	bool m_bIsDynamic;
};