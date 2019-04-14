//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines the shader device 
//
// $NoKeywords: $
//
//===========================================================================//
#pragma once

#include "shaderapi/ishaderapi.h"
#include "Mesh.h"

/*

Defines a base shader device

Shader devices are simply graphics devices.
In GL they are called "servers". well kinda.

This class appears just to interface with a GPU through OpenGl/Vulkan/Whatever
graphics API we are using.

The most basic graphics operations are performed here, such as compiling shaders, 
creating shaders, setting the current window and creating buffers.

*/
class CShaderDevice : public IShaderDevice
{
public:
	CShaderDevice() : m_DynamicMesh( true ), m_Mesh( false ) {}

	/*
    Returns the current adapter's index
    */
	virtual int GetCurrentAdapter() const { return 0; }

    /*
    Returns if the device is actually doing anything
    */
	virtual bool IsUsingGraphics() const { return false; }

    /*
    Spew driver info to output stream
    */
	virtual void SpewDriverInfo() const;
    
    /*
    Returns the back buffer's image format
    Note: the backbuffer is the non-rendered framebuffer. It is switched as the active 
    framebuffer once screen drawing occurs
    */
	virtual ImageFormat GetBackBufferFormat() const { return IMAGE_FORMAT_RGB888; }

    /*
    Returns the dimensions of the back buffer
    */
	virtual void GetBackBufferDimensions( int& width, int& height ) const;

    /*

    */
	virtual int  StencilBufferBits() const { return 0; }

    /*
    Returns if anti-aliasing is enabled or not
    */
	virtual bool IsAAEnabled() const { return false; }

    /*
    Presents the scene
    Basically just sets the backbuffer as the active framebuffer
    */
	virtual void Present( ) {}

    /*
    Returns the window's size
    */
	virtual void GetWindowSize( int &width, int &height ) const;

    /*
    Adds another view. I think this will also allocate a new backbuffer/framebuffer
    */
	virtual bool AddView( void* hwnd );

    /*
    Removes view
    Also deletes framebuffer/backbuffer associated with it
    */
	virtual void RemoveView( void* hwnd );

    /*
    Sets the active view?
    */
	virtual void SetView( void* hwnd );

    /*
    Probably deallocates buffers and stuff
    Unclear if this is in host memory, gpu memory or both
    */
	virtual void ReleaseResources();

    /*
    Most likely reaquires resources?
    */
	virtual void ReacquireResources();

    /*
    Creates a new static (non-modifiable) mesh in GPU memory
    This will allocate the corresponding buffers
    */
	virtual IMesh* CreateStaticMesh( VertexFormat_t fmt, const char *pTextureBudgetGroup, IMaterial * pMaterial = NULL );

    /*
    Destroys the specified static mesh in GPU memory.
    This will free the corresponding buffers
    */
	virtual void DestroyStaticMesh( IMesh* mesh );

    /*
    Compiles the shader into a shader buffer
    pProgram will contain the source-code of the program
    nBufLen is the length of the src
    pShaderVersion is the version of the shader?
    Note for GL: IShaderBuffer needs to be specialized
    */
	virtual IShaderBuffer* CompileShader( const char *pProgram, size_t nBufLen, const char *pShaderVersion ) { return NULL; }

    /*
    Creates a new vertex shader from the shader buffer
    */
	virtual VertexShaderHandle_t CreateVertexShader( IShaderBuffer* pShaderBuffer ) { return VERTEX_SHADER_HANDLE_INVALID; }

    /*
    Removes a vertex shader
    */
	virtual void DestroyVertexShader( VertexShaderHandle_t hShader ) {}

    /*
    Creates a new geometry shader from the shader buffer
    */
	virtual GeometryShaderHandle_t CreateGeometryShader( IShaderBuffer* pShaderBuffer ) { return GEOMETRY_SHADER_HANDLE_INVALID; }

    /*
    Destroys a geometry shader
    */
	virtual void DestroyGeometryShader( GeometryShaderHandle_t hShader ) {}

    /*
    Creates a pixel shader from the shader buffer
    */
	virtual PixelShaderHandle_t CreatePixelShader( IShaderBuffer* pShaderBuffer ) { return PIXEL_SHADER_HANDLE_INVALID; }

    /*
    Destroys pixel shader
    */
	virtual void DestroyPixelShader( PixelShaderHandle_t hShader ) {}

    /*
    Allocates a new vertex buffer with the specified formats and size
    */
	virtual IVertexBuffer *CreateVertexBuffer( ShaderBufferType_t type, VertexFormat_t fmt, int nVertexCount, const char *pBudgetGroup );

    /*
    Destroys a vertex buffer
    */
	virtual void DestroyVertexBuffer( IVertexBuffer *pVertexBuffer );

    /*
    Creates an index buffer with the specified format and size
    */
	virtual IIndexBuffer *CreateIndexBuffer( ShaderBufferType_t bufferType, MaterialIndexFormat_t fmt, int nIndexCount, const char *pBudgetGroup );

    /*
    Destroys index buffer
    */
	virtual void DestroyIndexBuffer( IIndexBuffer *pIndexBuffer );

    /*
    Returns a pointer to a dynamic vertex buffer specified by steamID with the format specified by vertexFormat
    bBuffered likely specified that the steam is buffered (i.e. we are reading from a file)
    */
	virtual IVertexBuffer *GetDynamicVertexBuffer( int streamID, VertexFormat_t vertexFormat, bool bBuffered );

    /*
    Returns a pointer to a dynamic index buffer with the format specified by fmt
    bBuffered likely specified that the steam is buffered (i.e. we are reading from a file)
    */
	virtual IIndexBuffer *GetDynamicIndexBuffer( MaterialIndexFormat_t fmt, bool bBuffered );

    /*

    */
	virtual void SetHardwareGammaRamp( float fGamma, float fGammaTVRangeMin, float fGammaTVRangeMax, float fGammaTVExponent, bool bTVEnabled ) {}

    /*

    */
	virtual void EnableNonInteractiveMode( MaterialNonInteractiveMode_t mode, ShaderNonInteractiveInfo_t *pInfo ) {}

    /*
    Refreshes front buffer?
    */
	virtual void RefreshFrontBufferNonInteractive( ) {}

private:
	CMesh m_Mesh;
	CMesh m_DynamicMesh;
};