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

abstract_class IMeshMgrDx11
{
public:
	virtual IMesh *CreateStaticMesh( VertexFormat_t fmt, const char *pTextureBudgetGroup, IMaterial *pMaterial ) = 0;
	virtual void DestroyStaticMesh( IMesh *mesh ) = 0;
	virtual IMesh *GetDynamicMesh( IMaterial *pMaterial, VertexFormat_t fmt, int nHWSkinBoneCount,
					 bool buffered, IMesh *pVertexOverride, IMesh *pIndexOverride ) = 0;
	virtual IVertexBuffer *GetDynamicVertexBuffer( IMaterial *pMaterial, bool buffered ) = 0;
	virtual IIndexBuffer *GetDynamicIndexBuffer( IMaterial *pMaterial, bool buffered ) = 0;
	virtual IMesh *GetFlexMesh() = 0;

	virtual VertexFormat_t ComputeVertexFormat( int nFlags, int nTexCoords, int *pTexCoordDimensions,
						    int nBoneWeights, int nUserDataSize ) = 0;

	virtual int VertexFormatSize( VertexFormat_t fmt ) = 0;

	virtual void RenderPass( IMesh *pMesh ) = 0;
};

IMeshMgrDx11 *MeshMgr();

#endif // MESHDX11_H

