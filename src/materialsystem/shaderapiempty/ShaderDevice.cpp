//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines the shader device 
//
// $NoKeywords: $
//
//===========================================================================//
#include "ShaderDevice.h"

void CShaderDevice::GetWindowSize( int &width, int &height ) const
{
	width = 0;
	height = 0;
}

void CShaderDevice::GetBackBufferDimensions( int& width, int& height ) const
{
	width = 1024;
	height = 768;
}

// Use this to spew information about the 3D layer 
void CShaderDevice::SpewDriverInfo() const
{
	//Warning("Empty shader\n");
}

// Creates/ destroys a child window
bool CShaderDevice::AddView( void* hwnd )
{
	return true;
}

void CShaderDevice::RemoveView( void* hwnd )
{
}

// Activates a view
void CShaderDevice::SetView( void* hwnd )
{
}

void CShaderDevice::ReleaseResources()
{
}

void CShaderDevice::ReacquireResources()
{
}

// Creates/destroys Mesh
IMesh* CShaderDevice::CreateStaticMesh( VertexFormat_t fmt, const char *pTextureBudgetGroup, IMaterial * pMaterial )
{
	return &m_Mesh;
}

void CShaderDevice::DestroyStaticMesh( IMesh* mesh )
{
}

// Creates/destroys static vertex + index buffers
IVertexBuffer *CShaderDevice::CreateVertexBuffer( ShaderBufferType_t type, VertexFormat_t fmt, int nVertexCount, const char *pTextureBudgetGroup )
{
	return ( type == SHADER_BUFFER_TYPE_STATIC || type == SHADER_BUFFER_TYPE_STATIC_TEMP ) ? &m_Mesh : &m_DynamicMesh;
}

void CShaderDevice::DestroyVertexBuffer( IVertexBuffer *pVertexBuffer )
{

}

IIndexBuffer *CShaderDevice::CreateIndexBuffer( ShaderBufferType_t bufferType, MaterialIndexFormat_t fmt, int nIndexCount, const char *pTextureBudgetGroup )
{
	switch( bufferType )
	{
	case SHADER_BUFFER_TYPE_STATIC:
	case SHADER_BUFFER_TYPE_STATIC_TEMP:
		return &m_Mesh;
	default:
		Assert( 0 );
	case SHADER_BUFFER_TYPE_DYNAMIC:
	case SHADER_BUFFER_TYPE_DYNAMIC_TEMP:
		return &m_DynamicMesh;
	}
}

void CShaderDevice::DestroyIndexBuffer( IIndexBuffer *pIndexBuffer )
{

}

IVertexBuffer *CShaderDevice::GetDynamicVertexBuffer( int streamID, VertexFormat_t vertexFormat, bool bBuffered )
{
	return &m_DynamicMesh;
}

IIndexBuffer *CShaderDevice::GetDynamicIndexBuffer( MaterialIndexFormat_t fmt, bool bBuffered )
{
	return &m_Mesh;
}