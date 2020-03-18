//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose:
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef SHADERAPIDX11_H
#define SHADERAPIDX11_H

#ifdef _WIN32
#pragma once
#endif

#include <d3d11.h>

#include "renderparm.h"
#include "shaderapidx9/shaderapibase.h"
#include "shaderapi/ishadershadow.h"
#include "materialsystem/idebugtextureinfo.h"
#include "shaderapidx9/hardwareconfig.h"
#include "imaterialinternal.h"
#include "shaderapidx9/meshbase.h"
#include "ShaderConstantBufferDx11.h"
#include "utllinkedlist.h"
#include "StatesDx11.h"
#include "TextureDx11.h"
#include "meshdx11.h"
#include "materialsystem/imesh.h"
#include "utlstack.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct MaterialSystemHardwareIdentifier_t;

//-----------------------------------------------------------------------------
// The Dx11 implementation of the shader API
//-----------------------------------------------------------------------------
class CShaderAPIDx11 : public CShaderAPIBase, public IDebugTextureInfo
{
	typedef CShaderAPIBase BaseClass;

public:
	// constructor, destructor
	CShaderAPIDx11();
	virtual ~CShaderAPIDx11();

	// Methods of IShaderAPI
	// NOTE: These methods have been ported over
public:
	virtual void SetViewports( int nCount, const ShaderViewport_t *pViewports );
	virtual int GetViewports( ShaderViewport_t *pViewports, int nMax ) const;
	virtual void ClearBuffers( bool bClearColor, bool bClearDepth, bool bClearStencil, int renderTargetWidth, int renderTargetHeight );
	virtual void ClearColor3ub( unsigned char r, unsigned char g, unsigned char b );
	virtual void ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	virtual void SetRasterState( const ShaderRasterState_t &state );
	virtual void BindVertexShader( VertexShaderHandle_t hVertexShader );
	virtual void BindGeometryShader( GeometryShaderHandle_t hGeometryShader );
	virtual void BindPixelShader( PixelShaderHandle_t hPixelShader );
	virtual void BindVertexBuffer( int nStreamID, IVertexBuffer *pVertexBuffer, int nOffsetInBytes, int nFirstVertex,
				       int nVertexCount, VertexFormat_t fmt, int nRepetitions = 1 );
	void SetUsingExtraVertexBuffers( bool bStaticLit, bool bUsingFlex, bool bUsingMorph );
	virtual void BindIndexBuffer( IIndexBuffer *pIndexBuffer, int nOffsetInBytes );
	virtual void Draw( MaterialPrimitiveType_t primitiveType, int nFirstIndex, int nIndexCount );
	void DrawIndexed( int nFirstIndex, int nIndexCount, int nBaseVertexLocation );
	void DrawNotIndexed( int nFirstVertex, int nVertCount );
	void DrawMesh( IMesh *pMesh );

	virtual void UpdateConstantBuffer( ConstantBufferHandle_t cbuffer, void *pNewData );
	virtual ConstantBufferHandle_t GetInternalConstantBuffer( int type );

	FORCEINLINE bool TextureIsAllocated( ShaderAPITextureHandle_t hTexture )
	{
		return m_Textures.IsValidIndex( hTexture ) && ( GetTexture( hTexture ).m_nFlags & CTextureDx11::IS_ALLOCATED );
	}

	// Methods of IShaderDynamicAPI
public:
	virtual void GetBackBufferDimensions( int &nWidth, int &nHeight ) const;

public:
	// Methods of CShaderAPIBase
	virtual bool OnDeviceInit();
	virtual void OnDeviceShutdown();
	virtual void ReleaseShaderObjects();
	virtual void RestoreShaderObjects();
	virtual void BeginPIXEvent( unsigned long color, const char *szName )
	{
	}
	virtual void EndPIXEvent()
	{
	}
	virtual void AdvancePIXFrame()
	{
	}

	// NOTE: These methods have not been ported over.
	// IDebugTextureInfo implementation.
public:
	virtual bool IsDebugTextureListFresh( int numFramesAllowed = 1 )
	{
		return false;
	}
	virtual void EnableDebugTextureList( bool bEnable )
	{
	}
	virtual void EnableGetAllTextures( bool bEnable )
	{
	}
	virtual KeyValues *GetDebugTextureList()
	{
		return NULL;
	}
	virtual int GetTextureMemoryUsed( TextureMemoryType eTextureMemory )
	{
		return 0;
	}
	virtual bool SetDebugTextureRendering( bool bEnable )
	{
		return false;
	}

public:
	// Other public methods
	void Unbind( VertexShaderHandle_t hShader );
	void Unbind( GeometryShaderHandle_t hShader );
	void Unbind( PixelShaderHandle_t hShader );
	void UnbindVertexBuffer( ID3D11Buffer *pBuffer );
	void UnbindIndexBuffer( ID3D11Buffer *pBuffer );

	void SetTopology( MaterialPrimitiveType_t topology );

	void IssueStateChanges( bool bForce = false );
	IMaterialInternal *GetBoundMaterial() const;

	void GetMatrix( MaterialMatrixMode_t matrixMode, DirectX::XMMATRIX &dst );
	D3D11_CULL_MODE GetCullMode() const;

private:
	// Returns a d3d texture associated with a texture handle
	virtual IDirect3DBaseTexture *GetD3DTexture( ShaderAPITextureHandle_t hTexture );

	virtual void QueueResetRenderState()
	{
	}

	virtual bool DoRenderTargetsNeedSeparateDepthBuffer() const;

	void SetHardwareGammaRamp( float fGamma )
	{
	}

	// Used to clear the transition table when we know it's become invalid.
	void ClearSnapshots();

	// Sets the mode...
	bool SetMode( void *hwnd, int nAdapter, const ShaderDeviceInfo_t &info );

	void ChangeVideoMode( const ShaderDeviceInfo_t &info )
	{
	}

	// Called when the dx support level has changed
	virtual void DXSupportLevelChanged()
	{
	}

	virtual void EnableUserClipTransformOverride( bool bEnable )
	{
	}
	virtual void UserClipTransform( const VMatrix &worldToView )
	{
	}
	virtual bool GetUserClipTransform( VMatrix &worldToView )
	{
		return false;
	}

	// Sets the default *dynamic* state
	void SetDefaultState();

	// Returns the snapshot id for the shader state
	StateSnapshot_t TakeSnapshot();

	// Returns true if the state snapshot is transparent
	bool IsTranslucent( StateSnapshot_t id ) const;
	bool IsAlphaTested( StateSnapshot_t id ) const;
	bool UsesVertexAndPixelShaders( StateSnapshot_t id ) const;
	virtual bool IsDepthWriteEnabled( StateSnapshot_t id ) const;

	// Gets the vertex format for a set of snapshot ids
	VertexFormat_t ComputeVertexFormat( int numSnapshots, StateSnapshot_t *pIds ) const;

	// Gets the vertex format for a set of snapshot ids
	VertexFormat_t ComputeVertexUsage( int numSnapshots, StateSnapshot_t *pIds ) const;

	// Begins a rendering pass that uses a state snapshot
	void BeginPass( StateSnapshot_t snapshot );

	// Uses a state snapshot
	void UseSnapshot( StateSnapshot_t snapshot );

	// Use this to get the mesh builder that allows us to modify vertex data
	CMeshBuilder *GetVertexModifyBuilder();

	// Sets the color to modulate by
	void Color3f( float r, float g, float b );
	void Color3fv( float const *pColor );
	void Color4f( float r, float g, float b, float a );
	void Color4fv( float const *pColor );

	// Faster versions of color
	void Color3ub( unsigned char r, unsigned char g, unsigned char b );
	void Color3ubv( unsigned char const *rgb );
	void Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a );
	void Color4ubv( unsigned char const *rgba );

	// Sets the lights
	void SetLight( int lightNum, const LightDesc_t &desc );
	void SetAmbientLight( float r, float g, float b );
	void SetAmbientLightCube( Vector4D cube[6] );
	virtual void DisableAllLocalLights();
	virtual void SetLightingOrigin( Vector vLightingOrigin )
	{
	}

	// Get the lights
	int GetMaxLights( void ) const;
	const LightDesc_t &GetLight( int lightNum ) const;

	// Render state for the ambient light cube (vertex shaders)
	void SetVertexShaderStateAmbientLightCube();
	virtual void SetPixelShaderStateAmbientLightCube( int pshReg, bool bForceToBlack = false )
	{
	}
	void SetPixelShaderStateAmbientLightCube( int pshReg )
	{
	}
	virtual void GetDX9LightState( LightState_t *state ) const;

	float GetAmbientLightCubeLuminance( void );

	void SetSkinningMatrices();

	// Lightmap texture binding
	void BindLightmap( TextureStage_t stage );
	void BindLightmapAlpha( TextureStage_t stage )
	{
	}
	void BindBumpLightmap( TextureStage_t stage );
	void BindFullbrightLightmap( TextureStage_t stage );
	void BindWhite( TextureStage_t stage );
	void BindBlack( TextureStage_t stage );
	void BindGrey( TextureStage_t stage );
	void BindFBTexture( TextureStage_t stage, int textureIdex );
	void CopyRenderTargetToTexture( ShaderAPITextureHandle_t texID )
	{
	}

	void CopyRenderTargetToTextureEx( ShaderAPITextureHandle_t texID, int nRenderTargetID, Rect_t *pSrcRect, Rect_t *pDstRect )
	{
	}

	// Special system flat normal map binding.
	void BindFlatNormalMap( TextureStage_t stage );
	void BindNormalizationCubeMap( TextureStage_t stage );
	void BindSignedNormalizationCubeMap( TextureStage_t stage );

	// Set the number of bone weights
	void SetNumBoneWeights( int numBones );

	// Flushes any primitives that are buffered
	void FlushBufferedPrimitives();

	// Creates/destroys Mesh
	IMesh *CreateStaticMesh( VertexFormat_t fmt, const char *pTextureBudgetGroup, IMaterial *pMaterial = NULL );
	void DestroyStaticMesh( IMesh *mesh );

	// Gets the dynamic mesh; note that you've got to render the mesh
	// before calling this function a second time. Clients should *not*
	// call DestroyStaticMesh on the mesh returned by this call.
	IMesh *GetDynamicMesh( IMaterial *pMaterial, int nHWSkinBoneCount, bool buffered, IMesh *pVertexOverride, IMesh *pIndexOverride );
	IMesh *GetDynamicMeshEx( IMaterial *pMaterial, VertexFormat_t fmt, int nHWSkinBoneCount, bool buffered, IMesh *pVertexOverride, IMesh *pIndexOverride );
	IVertexBuffer *GetDynamicVertexBuffer( IMaterial *pMaterial, bool buffered );
	IIndexBuffer *GetDynamicIndexBuffer( IMaterial *pMaterial, bool buffered );
	IMesh *GetFlexMesh();

	// Renders a single pass of a material
	void RenderPass( int nPass, int nPassCount );

	// stuff related to matrix stacks
	void MatrixMode( MaterialMatrixMode_t matrixMode );
	void PushMatrix();
	void PopMatrix();
	void LoadMatrix( float *m );
	void LoadBoneMatrix( int boneIndex, const float *m );
	void MultMatrix( float *m );
	void MultMatrixLocal( float *m );
	void GetMatrix( MaterialMatrixMode_t matrixMode, float *dst );
	void LoadIdentity( void );
	void LoadCameraToWorld( void );
	void Ortho( double left, double top, double right, double bottom, double zNear, double zFar );
	void PerspectiveX( double fovx, double aspect, double zNear, double zFar );
	void PerspectiveOffCenterX( double fovx, double aspect, double zNear, double zFar, double bottom, double top, double left, double right );
	void PickMatrix( int x, int y, int width, int height );
	void Rotate( float angle, float x, float y, float z );
	void Translate( float x, float y, float z );
	void Scale( float x, float y, float z );
	void ScaleXY( float x, float y );

	// Fog methods...
	void FogMode( MaterialFogMode_t fogMode );
	void FogStart( float fStart );
	void FogEnd( float fEnd );
	void SetFogZ( float fogZ );
	void FogMaxDensity( float flMaxDensity );
	void GetFogDistances( float *fStart, float *fEnd, float *fFogZ );
	void FogColor3f( float r, float g, float b );
	void FogColor3fv( float const *rgb );
	void FogColor3ub( unsigned char r, unsigned char g, unsigned char b );
	void FogColor3ubv( unsigned char const *rgb );

	virtual void SceneFogColor3ub( unsigned char r, unsigned char g, unsigned char b );
	virtual void SceneFogMode( MaterialFogMode_t fogMode );
	virtual void GetSceneFogColor( unsigned char *rgb );
	virtual MaterialFogMode_t GetSceneFogMode();
	virtual int GetPixelFogCombo();

	void SetHeightClipZ( float z );
	void SetHeightClipMode( enum MaterialHeightClipMode_t heightClipMode );

	void SetClipPlane( int index, const float *pPlane );
	void EnableClipPlane( int index, bool bEnable );

	void SetFastClipPlane( const float *pPlane );
	void EnableFastClip( bool bEnable );

	// We use smaller dynamic VBs during level transitions, to free up memory
	virtual int GetCurrentDynamicVBSize( void );
	virtual void DestroyVertexBuffers( bool bExitingLevel = false );

	// Sets the vertex and pixel shaders
	void SetVertexShaderIndex( int vshIndex );
	void SetPixelShaderIndex( int pshIndex );

	// Sets the constant register for vertex and pixel shaders
	void SetVertexShaderConstant( int var, float const *pVec, int numConst = 1, bool bForce = false );
	void SetPixelShaderConstant( int var, float const *pVec, int numConst = 1, bool bForce = false );

	void SetBooleanVertexShaderConstant( int var, BOOL const *pVec, int numBools = 1, bool bForce = false )
	{
		Assert( 0 );
	}

	void SetIntegerVertexShaderConstant( int var, int const *pVec, int numIntVecs = 1, bool bForce = false )
	{
		Assert( 0 );
	}

	void SetBooleanPixelShaderConstant( int var, BOOL const *pVec, int numBools = 1, bool bForce = false )
	{
		Assert( 0 );
	}

	void SetIntegerPixelShaderConstant( int var, int const *pVec, int numIntVecs = 1, bool bForce = false )
	{
		Assert( 0 );
	}

	bool ShouldWriteDepthToDestAlpha( void ) const;

	void InvalidateDelayedShaderConstants( void );

	// Gamma<->Linear conversions according to the video hardware we're running on
	float GammaToLinear_HardwareSpecific( float fGamma ) const
	{
		return 0.;
	}

	float LinearToGamma_HardwareSpecific( float fLinear ) const
	{
		return 0.;
	}

	//Set's the linear->gamma conversion textures to use for this hardware for both srgb writes enabled and disabled(identity)
	void SetLinearToGammaConversionTextures( ShaderAPITextureHandle_t hSRGBWriteEnabledTexture, ShaderAPITextureHandle_t hIdentityTexture );

	// Cull mode
	void CullMode( MaterialCullMode_t cullMode );

	// Force writes only when z matches. . . useful for stenciling things out
	// by rendering the desired Z values ahead of time.
	void ForceDepthFuncEquals( bool bEnable );

	// Forces Z buffering on or off
	void OverrideDepthEnable( bool bEnable, bool bDepthEnable );

	// Sets the shade mode
	void ShadeMode( ShaderShadeMode_t mode );

	// Binds a particular material to render with
	void Bind( IMaterial *pMaterial );

	// Returns the nearest supported format
	ImageFormat GetNearestSupportedFormat( ImageFormat fmt ) const;
	ImageFormat GetNearestRenderTargetFormat( ImageFormat fmt ) const;

	// Sets the texture state
	void BindTexture( Sampler_t stage, ShaderAPITextureHandle_t textureHandle );
	void UnbindTexture( ShaderAPITextureHandle_t textureHandle );

	void SetRenderTarget( ShaderAPITextureHandle_t colorTextureHandle, ShaderAPITextureHandle_t depthTextureHandle )
	{
		SetRenderTargetEx( 0, colorTextureHandle, depthTextureHandle );
	}

	void SetRenderTargetEx( int nRenderTargetID, ShaderAPITextureHandle_t colorTextureHandle, ShaderAPITextureHandle_t depthTextureHandle );

	// Indicates we're going to be modifying this texture
	// TexImage2D, TexSubImage2D, TexWrap, TexMinFilter, and TexMagFilter
	// all use the texture specified by this function.
	void ModifyTexture( ShaderAPITextureHandle_t textureHandle );

	// Texture management methods
	void TexImage2D( int level, int cubeFace, ImageFormat dstFormat, int zOffset, int width, int height,
			 ImageFormat srcFormat, bool bSrcIsTiled, void *imageData );
	void TexSubImage2D( int level, int cubeFace, int xOffset, int yOffset, int zOffset, int width, int height,
			    ImageFormat srcFormat, int srcStride, bool bSrcIsTiled, void *imageData );

	bool TexLock( int level, int cubeFaceID, int xOffset, int yOffset,
		      int width, int height, CPixelWriter &writer );
	void TexUnlock();

	// These are bound to the texture, not the texture environment
	void TexMinFilter( ShaderTexFilterMode_t texFilterMode );
	void TexMagFilter( ShaderTexFilterMode_t texFilterMode );
	void TexWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode );
	void TexSetPriority( int priority );

	ShaderAPITextureHandle_t CreateTexture(
	    int width,
	    int height,
	    int depth,
	    ImageFormat dstImageFormat,
	    int numMipLevels,
	    int numCopies,
	    int flags,
	    const char *pDebugName,
	    const char *pTextureGroupName );
	void CreateTextures(
	    ShaderAPITextureHandle_t *pHandles,
	    int count,
	    int width,
	    int height,
	    int depth,
	    ImageFormat dstImageFormat,
	    int numMipLevels,
	    int numCopies,
	    int flags,
	    const char *pDebugName,
	    const char *pTextureGroupName );
	ShaderAPITextureHandle_t CreateDepthTexture( ImageFormat renderFormat, int width, int height, const char *pDebugName, bool bTexture );
	void DeleteTexture( ShaderAPITextureHandle_t textureHandle );
	bool IsTexture( ShaderAPITextureHandle_t textureHandle );
	bool IsTextureResident( ShaderAPITextureHandle_t textureHandle );

	// stuff that isn't to be used from within a shader
	void ClearBuffersObeyStencil( bool bClearColor, bool bClearDepth );
	void PerformFullScreenStencilOperation( void );
	void ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat );
	virtual void ReadPixels( Rect_t *pSrcRect, Rect_t *pDstRect, unsigned char *data, ImageFormat dstFormat, int nDstStride );

	// Selection mode methods
	int SelectionMode( bool selectionMode );
	void SelectionBuffer( unsigned int *pBuffer, int size );
	void ClearSelectionNames();
	void LoadSelectionName( int name );
	void PushSelectionName( int name );
	void PopSelectionName();
	void WriteHitRecord();
	bool IsInSelectionMode() const;
	void RegisterSelectionHit( float minz, float maxz );

	void FlushHardware();
	void ResetRenderState( bool bFullReset = true );

	// Can we download textures?
	virtual bool CanDownloadTextures() const;

	bool IsStateSet( unsigned int flag )  const
	{
		return ( m_StateChangeFlags & flag ) != 0;
	}

	void SetStateFlag( unsigned int flag )
	{
		m_StateChangeFlags |= flag;
	}

	// Board-independent calls, here to unify how shaders set state
	// Implementations should chain back to IShaderUtil->BindTexture(), etc.

	// Use this to begin and end the frame
	void BeginFrame();
	void EndFrame();

	// returns current time
	double CurrentTime() const;

	// Get the current camera position in world space.
	void GetWorldSpaceCameraPosition( float *pPos ) const;

	void ForceHardwareSync( void );

	int GetCurrentNumBones( void ) const;
	bool IsHWMorphingEnabled() const;
	int GetCurrentLightCombo( void ) const;
	int MapLightComboToPSLightCombo( int nLightCombo ) const;
	MaterialFogMode_t GetCurrentFogType( void ) const;

	void RecordString( const char *pStr );

	void EvictManagedResources();

	void SetTextureTransformDimension( TextureStage_t textureStage, int dimension, bool projected );
	void DisableTextureTransform( TextureStage_t textureStage )
	{
	}
	void SetBumpEnvMatrix( TextureStage_t textureStage, float m00, float m01, float m10, float m11 );

	// Gets the lightmap dimensions
	virtual void GetLightmapDimensions( int *w, int *h );

	virtual void SyncToken( const char *pToken );

	// Setup standard vertex shader constants (that don't change)
	// This needs to be called anytime that overbright changes.
	virtual void SetStandardVertexShaderConstants( float fOverbright )
	{
	}

	// Scissor Rect
	virtual void SetScissorRect( const int nLeft, const int nTop, const int nRight, const int nBottom, const bool bEnableScissor )
	{
	}

	// Reports support for a given CSAA mode
	bool SupportsCSAAMode( int nNumSamples, int nQualityLevel )
	{
		return false;
	}

	// Level of anisotropic filtering
	virtual void SetAnisotropicLevel( int nAnisotropyLevel );

	void SetDefaultDynamicState()
	{
		m_ShaderState.SetDefault();
	}
	virtual void CommitPixelShaderLighting( int pshReg )
	{
	}

	virtual void MarkUnusedVertexFields( unsigned int nFlags, int nTexCoordCount, bool *pUnusedTexCoords )
	{
	}

	// Occlusion queries
	ShaderAPIOcclusionQuery_t CreateOcclusionQueryObject( void );
	void DestroyOcclusionQueryObject( ShaderAPIOcclusionQuery_t handle );
	void BeginOcclusionQueryDrawing( ShaderAPIOcclusionQuery_t handle );
	void EndOcclusionQueryDrawing( ShaderAPIOcclusionQuery_t handle );
	int OcclusionQuery_GetNumPixelsRendered( ShaderAPIOcclusionQuery_t handle, bool bFlush );

	virtual void AcquireThreadOwnership()
	{
	}
	virtual void ReleaseThreadOwnership()
	{
	}

	virtual bool SupportsNormalMapCompression() const
	{
		return false;
	}
	virtual bool SupportsBorderColor() const
	{
		return false;
	}
	virtual bool SupportsFetch4() const
	{
		return true;
	}
	virtual void EnableBuffer2FramesAhead( bool bEnable )
	{
	}

	virtual void SetDepthFeatheringPixelShaderConstant( int iConstant, float fDepthBlendScale )
	{
	}

	void SetPixelShaderFogParams( int reg );

	virtual bool InFlashlightMode() const;
	virtual bool InEditorMode() const;

	// What fields in the morph do we actually use?
	virtual MorphFormat_t ComputeMorphFormat( int numSnapshots, StateSnapshot_t *pIds ) const;

	// Gets the bound morph's vertex format; returns 0 if no morph is bound
	virtual MorphFormat_t GetBoundMorphFormat();

	void GetStandardTextureDimensions( int *pWidth, int *pHeight, StandardTextureId_t id );

	// Binds a standard texture
	virtual void BindStandardTexture( Sampler_t stage, StandardTextureId_t id );
	virtual void BindStandardVertexTexture( VertexTextureSampler_t stage, StandardTextureId_t id );

	virtual void SetFlashlightState( const FlashlightState_t &state, const VMatrix &worldToTexture );
	virtual void SetFlashlightStateEx( const FlashlightState_t &state, const VMatrix &worldToTexture, ITexture *pFlashlightDepthTexture );
	virtual const FlashlightState_t &GetFlashlightState( VMatrix &worldToTexture ) const;
	virtual const FlashlightState_t &GetFlashlightStateEx( VMatrix &worldToTexture, ITexture **pFlashlightDepthTexture ) const;

	virtual void SetModeChangeCallback( ModeChangeCallbackFunc_t func )
	{
	}

	virtual void ClearVertexAndPixelShaderRefCounts();
	virtual void PurgeUnusedVertexAndPixelShaders();

	// Binds a vertex texture to a particular texture stage in the vertex pipe
	virtual void BindVertexTexture( VertexTextureSampler_t nStage, ShaderAPITextureHandle_t hTexture );

	// Sets morph target factors
	virtual void SetFlexWeights( int nFirstWeight, int nCount, const MorphWeight_t *pWeights );

	// NOTE: Stuff after this is added after shipping HL2.
	ITexture *GetRenderTargetEx( int nRenderTargetID )
	{
		return NULL;
	}

	void SetToneMappingScaleLinear( const Vector &scale )
	{
	}

	const Vector &GetToneMappingScaleLinear( void ) const
	{
		static Vector dummy;
		return dummy;
	}

	virtual float GetLightMapScaleFactor( void ) const;

	// For dealing with device lost in cases where SwapBuffers isn't called all the time (Hammer)
	virtual void HandleDeviceLost()
	{
	}

	virtual void EnableLinearColorSpaceFrameBuffer( bool bEnable )
	{
	}

	// Lets the shader know about the full-screen texture so it can
	virtual void SetFullScreenTextureHandle( ShaderAPITextureHandle_t h )
	{
	}

	void SetFloatRenderingParameter( int parm_number, float value );
	void SetIntRenderingParameter( int parm_number, int value );
	void SetVectorRenderingParameter( int parm_number, Vector const &value );
	float GetFloatRenderingParameter( int parm_number ) const;
	int GetIntRenderingParameter( int parm_number ) const;
	Vector GetVectorRenderingParameter( int parm_number ) const;

	// Methods related to stencil
	void SetStencilEnable( bool onoff );
	void SetStencilFailOperation( StencilOperation_t op );
	void SetStencilZFailOperation( StencilOperation_t op );
	void SetStencilPassOperation( StencilOperation_t op );
	void SetStencilCompareFunction( StencilComparisonFunction_t cmpfn );
	void SetStencilReferenceValue( int ref );
	void SetStencilTestMask( uint32 msk );
	void SetStencilWriteMask( uint32 msk );

	void ClearStencilBufferRectangle( int xmin, int ymin, int xmax, int ymax, int value )
	{
	}

	virtual void GetDXLevelDefaults( uint &max_dxlevel, uint &recommended_dxlevel )
	{
		max_dxlevel = recommended_dxlevel = 110;
	}

	virtual void GetMaxToRender( IMesh *pMesh, bool bMaxUntilFlush, int *pMaxVerts, int *pMaxIndices )
	{
		MeshMgr()->GetMaxToRender( pMesh, bMaxUntilFlush, pMaxVerts, pMaxIndices );
	}

	// Returns the max possible vertices + indices to render in a single draw call
	virtual int GetMaxVerticesToRender( IMaterial *pMaterial )
	{
		return MeshMgr()->GetMaxVerticesToRender( pMaterial );
	}

	virtual int GetMaxIndicesToRender()
	{
		return MeshMgr()->GetMaxIndicesToRender();
	}
	virtual int CompareSnapshots( StateSnapshot_t snapshot0, StateSnapshot_t snapshot1 );

	virtual bool SupportsMSAAMode( int nMSAAMode )
	{
		return true;
	}

	// Hooks for firing PIX events from outside the Material System...
	virtual void SetPIXMarker( unsigned long color, const char *szName )
	{
	}

	virtual void ComputeVertexDescription( unsigned char *pBuffer, VertexFormat_t vertexFormat, MeshDesc_t &desc ) const
	{
		return MeshMgr()->ComputeVertexDescription( pBuffer, vertexFormat, desc );
	}

	virtual bool SupportsShadowDepthTextures()
	{
		return false;
	}

	virtual int NeedsShaderSRGBConversion( void ) const
	{
		return 0;
	}

	virtual bool SupportsFetch4()
	{
		return false;
	}

	virtual void SetShadowDepthBiasFactors( float fShadowSlopeScaleDepthBias, float fShadowDepthBias )
	{
	}

	virtual void SetDisallowAccess( bool )
	{
	}
	virtual void EnableShaderShaderMutex( bool )
	{
	}
	virtual void ShaderLock()
	{
	}
	virtual void ShaderUnlock()
	{
	}
	virtual void EnableHWMorphing( bool bEnable )
	{
	}
	ImageFormat GetNullTextureFormat( void )
	{
		return IMAGE_FORMAT_RGBA8888;
	} // stub
	virtual void PushDeformation( DeformationBase_t const *Deformation )
	{
	}

	virtual void PopDeformation()
	{
	}

	virtual int GetNumActiveDeformations() const
	{
		return 0;
	}

	virtual void ExecuteCommandBuffer( uint8 *pBuf );

	void SetStandardTextureHandle( StandardTextureId_t, ShaderAPITextureHandle_t )
	{
	}

	int GetPackedDeformationInformation( int nMaskOfUnderstoodDeformations,
					     float *pConstantValuesOut,
					     int nBufferSize,
					     int nMaximumDeformations,
					     int *pNumDefsOut ) const
	{
		*pNumDefsOut = 0;
		return 0;
	}

	virtual bool OwnGPUResources( bool bEnable )
	{
		return false;
	}

private:
	enum
	{
		TRANSLUCENT		 = 0x1,
		ALPHATESTED		 = 0x2,
		VERTEX_AND_PIXEL_SHADERS = 0x4,
		DEPTHWRITE		 = 0x8,
	};
	void EnableAlphaToCoverage(){};
	void DisableAlphaToCoverage(){};

	ImageFormat GetShadowDepthTextureFormat()
	{
		return IMAGE_FORMAT_NV_DST24;
	};

	//
	// NOTE: Under here are real methods being used by dx11 implementation
	// above is stuff I still have to port over.
	//
private:
	//void ClearShaderState( ShaderStateDx11_t *pState );

	void CreateTextureHandles( ShaderAPITextureHandle_t *handles, int count );
	CTextureDx11 &GetTexture(ShaderAPITextureHandle_t handle);
	ShaderAPITextureHandle_t CreateTextureHandle();

	void DoIssueVertexShader();
	void DoIssuePixelShader();
	void DoIssueGeometryShader();
	bool DoIssueConstantBuffers( bool bForce );
	void DoIssueSampler( bool bPixel, bool bVertex );
	void DoIssueRasterState();
	void DoIssueBlendState();
	void DoIssueDepthStencilState();
	bool DoIssueVertexBuffer();
	void DoIssueIndexBuffer();
	void DoIssueInputLayout();
	void DoIssueTopology();
	void DoIssueViewports();
	void DoIssueShaderState( bool bForce );
	void DoIssueRenderTargets();

	void SortLights( int *index = NULL );

	void AdvanceCurrentTextureCopy( ShaderAPITextureHandle_t handle );

	bool MatrixIsChanging();
	bool IsDeactivated() const;
	DirectX::XMMATRIX &GetMatrix( MaterialMatrixMode_t mode );
	DirectX::XMMATRIX &GetCurrentMatrix();
	DirectX::XMMATRIX GetMatrixCopy( MaterialMatrixMode_t mode ) const;
	DirectX::XMMATRIX GetCurrentMatrixCopy() const;
	void HandleMatrixModified();

	void RenderPassWithVertexAndIndexBuffers();

	int ComputeNumLights() const;

private:
	// Current material + mesh
	IMaterialInternal *m_pMaterial;
	CMeshBase *m_pMesh;
	int m_nDynamicVBSize;

	FlashlightState_t m_FlashlightState;
	bool m_bFlashlightStateChanged;
	VMatrix m_FlashlightWorldToTexture;
	ITexture *m_pFlashlightDepthTexture;

	// rendering parameter storage
	int IntRenderingParameters[MAX_INT_RENDER_PARMS];
	float FloatRenderingParameters[MAX_FLOAT_RENDER_PARMS];
	Vector VectorRenderingParameters[MAX_VECTOR_RENDER_PARMS];

	// Members related to textures
	// UNDONE: Should this stuff be in ShaderDeviceDx11?
	CUtlVector<CTextureDx11> m_Textures;
	char m_ModifyTextureLockedLevel;
	char m_ModifyTextureLockedFace;
	ShaderAPITextureHandle_t m_ModifyTextureHandle;
	ShaderAPITextureHandle_t m_hBackBuffer;
	ShaderAPITextureHandle_t m_hDepthBuffer;

	// Common constant buffers
	ConstantBufferHandle_t m_hPerModelConstants;
	ConstantBufferHandle_t m_hPerSceneConstants;
	ConstantBufferHandle_t m_hPerFrameConstants;
	ConstantBufferHandle_t m_hSkinningConstants;
	//ConstantBufferHandle_t m_hFlexConstants;

	// Current and target states
	StateSnapshot_t m_CurrentSnapshot;
	bool m_bResettingRenderState : 1;

	const StatesDx11::ShadowState *m_ShadowState;
	StatesDx11::DynamicState m_DynamicState;
	StatesDx11::ShaderState m_ShaderState;
	unsigned int m_StateChangeFlags;

	// Setting matrices
	MaterialMatrixMode_t m_MatrixMode;
	StatesDx11::MatrixItem_t *m_pCurMatrixItem;

	bool m_bSelectionMode;

	// Mesh builder used to modify vertex data
	CMeshBuilder m_ModifyBuilder;

	// Selection name stack
	CUtlStack< int >	m_SelectionNames;
	bool				m_InSelectionMode;
	unsigned int *m_pSelectionBufferEnd;
	unsigned int *m_pSelectionBuffer;
	unsigned int *m_pCurrSelectionRecord;
	float				m_SelectionMinZ;
	float				m_SelectionMaxZ;
	int					m_NumHits;

	friend class CTempMeshDX11;
	friend class CMeshDX11;
	friend class CDynamicMeshDX11;
	friend class CBufferedMeshDX11;
	friend class CMeshMgr;
	friend class CShaderDeviceDx11;
	friend class CShaderShadowDx11;

};

//-----------------------------------------------------------------------------
// Singleton global
//-----------------------------------------------------------------------------
extern CShaderAPIDx11 *g_pShaderAPIDx11;

#endif // SHADERAPIDX11_H
