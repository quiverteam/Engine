//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines the shader API
//
// $NoKeywords: $
//
//===========================================================================//
#include "ShaderAPI.h"
#include "ShaderAPIEmpty.h"

#include "shaderapi/ishaderutil.h"

CShaderAPI::CShaderAPI()  : m_Mesh( false )
{
}

CShaderAPI::~CShaderAPI()
{
}


bool CShaderAPI::DoRenderTargetsNeedSeparateDepthBuffer() const
{
	return false;
}

// Can we download textures?
bool CShaderAPI::CanDownloadTextures() const
{
	return false;
}

// Used to clear the transition table when we know it's become invalid.
void CShaderAPI::ClearSnapshots()
{
}

// Members of IMaterialSystemHardwareConfig
bool CShaderAPI::HasDestAlphaBuffer() const
{
	return false;
}

bool CShaderAPI::HasStencilBuffer() const
{
	return false;
}

int CShaderAPI::MaxViewports() const
{
	return 1;
}

int CShaderAPI::GetShadowFilterMode() const
{
	return 0;
}

float CShaderAPI::GetShadowDepthBias() const { return 0; }
float CShaderAPI::GetShadowSlopeScaleDepthBias() const { return 0; }

int CShaderAPI::StencilBufferBits() const
{
	return 0;
}

int	 CShaderAPI::GetFrameBufferColorDepth() const
{
	return 0;
}

int  CShaderAPI::GetSamplerCount() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 60))
		return 1;
	if (( ShaderUtil()->GetConfig().dxSupportLevel >= 60 ) && ( ShaderUtil()->GetConfig().dxSupportLevel < 80 ))
		return 2;
	return 4;
}

bool CShaderAPI::HasSetDeviceGammaRamp() const
{
	return false;
}

bool CShaderAPI::SupportsCompressedTextures() const
{
	return false;
}

VertexCompressionType_t CShaderAPI::SupportsCompressedVertices() const
{
	return VERTEX_COMPRESSION_NONE;
}

bool CShaderAPI::SupportsVertexAndPixelShaders() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 80))
		return false;

	return true;
}

bool CShaderAPI::SupportsPixelShaders_1_4() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 81))
		return false;

	return true;
}

bool CShaderAPI::SupportsPixelShaders_2_0() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 90))
		return false;

	return true;
}

bool CShaderAPI::SupportsPixelShaders_2_b() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 90))
		return false;

	return true;
}

bool CShaderAPI::ActuallySupportsPixelShaders_2_b() const
{
	return true;
}

bool CShaderAPI::SupportsShaderModel_3_0() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
		(ShaderUtil()->GetConfig().dxSupportLevel < 95))
		return false;

	return true;
}

bool CShaderAPI::SupportsVertexShaders_2_0() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 90))
		return false;

	return true;
}

int  CShaderAPI::MaximumAnisotropicLevel() const
{
	return 0;
}

void CShaderAPI::SetAnisotropicLevel( int nAnisotropyLevel )
{
}

int  CShaderAPI::MaxTextureWidth() const
{
	// Should be big enough to cover all cases
	return 16384;
}

int  CShaderAPI::MaxTextureHeight() const
{
	// Should be big enough to cover all cases
	return 16384;
}

int  CShaderAPI::MaxTextureAspectRatio() const
{
	// Should be big enough to cover all cases
	return 16384;
}


int	 CShaderAPI::TextureMemorySize() const
{
	// fake it
	return 64 * 1024 * 1024;
}

int  CShaderAPI::GetDXSupportLevel() const 
{ 
	return 90; 
}

bool CShaderAPI::SupportsOverbright() const
{
	return false;
}

bool CShaderAPI::SupportsCubeMaps() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
		return false;

	return true;
}

bool CShaderAPI::SupportsNonPow2Textures() const
{
	return true;
}

bool CShaderAPI::SupportsMipmappedCubemaps() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
		return false;

	return true;
}

int  CShaderAPI::GetTextureStageCount() const
{
	return 4;
}

int	 CShaderAPI::NumVertexShaderConstants() const
{
	return 128;
}

int	 CShaderAPI::NumBooleanVertexShaderConstants() const
{
	return 0;
}

int	 CShaderAPI::NumIntegerVertexShaderConstants() const
{
	return 0;
}

int	 CShaderAPI::NumPixelShaderConstants() const
{
	return 8;
}

int	 CShaderAPI::MaxNumLights() const
{
	return 4;
}

bool CShaderAPI::SupportsSpheremapping() const
{
	return false;
}


// This is the max dx support level supported by the card
int	CShaderAPI::GetMaxDXSupportLevel() const
{
	return 90;
}

bool CShaderAPI::SupportsHardwareLighting() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
		return false;

	return true;
}

int	 CShaderAPI::MaxBlendMatrices() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
	{
		return 1;
	}

	return 0;
}

int	 CShaderAPI::MaxBlendMatrixIndices() const
{
	if ((ShaderUtil()->GetConfig().dxSupportLevel > 0) &&
	    (ShaderUtil()->GetConfig().dxSupportLevel < 70))
	{
		return 1;
	}

	return 0;
}

int	 CShaderAPI::MaxVertexShaderBlendMatrices() const
{
	return 0;
}

int	CShaderAPI::MaxUserClipPlanes() const
{
	return 0;
}

bool CShaderAPI::SpecifiesFogColorInLinearSpace() const
{
	return false;
}

bool CShaderAPI::SupportsSRGB() const
{
	return false;
}

const char *CShaderAPI::GetHWSpecificShaderDLLName() const
{
	return 0;
}

// Sets the default *dynamic* state
void CShaderAPI::SetDefaultState()
{
}


// Returns the snapshot id for the shader state
StateSnapshot_t	 CShaderAPI::TakeSnapshot( )
{
	StateSnapshot_t id = 0;
	if (g_ShaderShadow.m_IsTranslucent)
		id |= TRANSLUCENT;
	if (g_ShaderShadow.m_IsAlphaTested)
		id |= ALPHATESTED;
	if (g_ShaderShadow.m_bUsesVertexAndPixelShaders)
		id |= VERTEX_AND_PIXEL_SHADERS;
	if (g_ShaderShadow.m_bIsDepthWriteEnabled)
		id |= DEPTHWRITE;
	return id;
}

// Returns true if the state snapshot is transparent
bool CShaderAPI::IsTranslucent( StateSnapshot_t id ) const
{
	return (id & TRANSLUCENT) != 0; 
}

bool CShaderAPI::IsAlphaTested( StateSnapshot_t id ) const
{
	return (id & ALPHATESTED) != 0; 
}

bool CShaderAPI::IsDepthWriteEnabled( StateSnapshot_t id ) const
{
	return (id & DEPTHWRITE) != 0; 
}

bool CShaderAPI::UsesVertexAndPixelShaders( StateSnapshot_t id ) const
{
	return (id & VERTEX_AND_PIXEL_SHADERS) != 0; 
}

// Gets the vertex format for a set of snapshot ids
VertexFormat_t CShaderAPI::ComputeVertexFormat( int numSnapshots, StateSnapshot_t* pIds ) const
{
	return 0;
}

// Gets the vertex format for a set of snapshot ids
VertexFormat_t CShaderAPI::ComputeVertexUsage( int numSnapshots, StateSnapshot_t* pIds ) const
{
	return 0;
}

// Uses a state snapshot
void CShaderAPI::UseSnapshot( StateSnapshot_t snapshot )
{
}

// Sets the color to modulate by
void CShaderAPI::Color3f( float r, float g, float b )
{
}

void CShaderAPI::Color3fv( float const* pColor )
{
}

void CShaderAPI::Color4f( float r, float g, float b, float a )
{
}

void CShaderAPI::Color4fv( float const* pColor )
{
}

// Faster versions of color
void CShaderAPI::Color3ub( unsigned char r, unsigned char g, unsigned char b )
{
}

void CShaderAPI::Color3ubv( unsigned char const* rgb )
{
}

void CShaderAPI::Color4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
}

void CShaderAPI::Color4ubv( unsigned char const* rgba )
{
}

// The shade mode
void CShaderAPI::ShadeMode( ShaderShadeMode_t mode )
{
}

// Binds a particular material to render with
void CShaderAPI::Bind( IMaterial* pMaterial )
{
}

// Cull mode
void CShaderAPI::CullMode( MaterialCullMode_t cullMode )
{
}

void CShaderAPI::ForceDepthFuncEquals( bool bEnable )
{
}

// Forces Z buffering on or off
void CShaderAPI::OverrideDepthEnable( bool bEnable, bool bDepthEnable )
{
}


//legacy fast clipping linkage
void CShaderAPI::SetHeightClipZ( float z )
{
}

void CShaderAPI::SetHeightClipMode( enum MaterialHeightClipMode_t heightClipMode )
{
}

// Sets the lights
void CShaderAPI::SetLight( int lightNum, const LightDesc_t& desc )
{
}

// Sets lighting origin for the current model
void CShaderAPI::SetLightingOrigin( Vector vLightingOrigin )
{
}

void CShaderAPI::SetAmbientLight( float r, float g, float b )
{
}

void CShaderAPI::SetAmbientLightCube( Vector4D cube[6] )
{
}

// Get lights
int CShaderAPI::GetMaxLights( void ) const
{
	return 0;
}

const LightDesc_t& CShaderAPI::GetLight( int lightNum ) const
{
	static LightDesc_t blah;
	return blah;
}

// Render state for the ambient light cube (vertex shaders)
void CShaderAPI::SetVertexShaderStateAmbientLightCube()
{
}

void CShaderAPI::SetSkinningMatrices()
{
}

// Lightmap texture binding
void CShaderAPI::BindLightmap( TextureStage_t stage )
{
}

void CShaderAPI::BindBumpLightmap( TextureStage_t stage )
{
}

void CShaderAPI::BindFullbrightLightmap( TextureStage_t stage )
{
}

void CShaderAPI::BindWhite( TextureStage_t stage )
{
}

void CShaderAPI::BindBlack( TextureStage_t stage )
{
}

void CShaderAPI::BindGrey( TextureStage_t stage )
{
}

// Gets the lightmap dimensions
void CShaderAPI::GetLightmapDimensions( int *w, int *h )
{
	g_pShaderUtil->GetLightmapDimensions( w, h );
}

// Special system flat normal map binding.
void CShaderAPI::BindFlatNormalMap( TextureStage_t stage )
{
}

void CShaderAPI::BindNormalizationCubeMap( TextureStage_t stage )
{
}

void CShaderAPI::BindSignedNormalizationCubeMap( TextureStage_t stage )
{
}

void CShaderAPI::BindFBTexture( TextureStage_t stage, int textureIndex )
{
}

// Flushes any primitives that are buffered
void CShaderAPI::FlushBufferedPrimitives()
{
}

// Gets the dynamic mesh; note that you've got to render the mesh
// before calling this function a second time. Clients should *not*
// call DestroyStaticMesh on the mesh returned by this call.
IMesh* CShaderAPI::GetDynamicMesh( IMaterial* pMaterial, int nHWSkinBoneCount, bool buffered, IMesh* pVertexOverride, IMesh* pIndexOverride )
{
	return &m_Mesh;
}

IMesh* CShaderAPI::GetDynamicMeshEx( IMaterial* pMaterial, VertexFormat_t fmt, int nHWSkinBoneCount, bool buffered, IMesh* pVertexOverride, IMesh* pIndexOverride )
{
	return &m_Mesh;
}

IMesh* CShaderAPI::GetFlexMesh()
{
	return &m_Mesh;
}

// Begins a rendering pass that uses a state snapshot
void CShaderAPI::BeginPass( StateSnapshot_t snapshot  )
{
}

// Renders a single pass of a material
void CShaderAPI::RenderPass( int nPass, int nPassCount )
{
}

// stuff related to matrix stacks
void CShaderAPI::MatrixMode( MaterialMatrixMode_t matrixMode )
{
}

void CShaderAPI::PushMatrix()
{
}

void CShaderAPI::PopMatrix()
{
}

void CShaderAPI::LoadMatrix( float *m )
{
}

void CShaderAPI::MultMatrix( float *m )
{
}

void CShaderAPI::MultMatrixLocal( float *m )
{
}

void CShaderAPI::GetMatrix( MaterialMatrixMode_t matrixMode, float *dst )
{
}

void CShaderAPI::LoadIdentity( void )
{
}

void CShaderAPI::LoadCameraToWorld( void )
{
}

void CShaderAPI::Ortho( double left, double top, double right, double bottom, double zNear, double zFar )
{
}

void CShaderAPI::PerspectiveX( double fovx, double aspect, double zNear, double zFar )
{
}

void CShaderAPI::PerspectiveOffCenterX( double fovx, double aspect, double zNear, double zFar, double bottom, double top, double left, double right )
{
}

void CShaderAPI::PickMatrix( int x, int y, int width, int height )
{
}

void CShaderAPI::Rotate( float angle, float x, float y, float z )
{
}

void CShaderAPI::Translate( float x, float y, float z )
{
}

void CShaderAPI::Scale( float x, float y, float z )
{
}

void CShaderAPI::ScaleXY( float x, float y )
{
}

// Fog methods...
void CShaderAPI::FogMode( MaterialFogMode_t fogMode )
{
}

void CShaderAPI::FogStart( float fStart )
{
}

void CShaderAPI::FogEnd( float fEnd )
{
}

void CShaderAPI::SetFogZ( float fogZ )
{
}
	
void CShaderAPI::FogMaxDensity( float flMaxDensity )
{
}

void CShaderAPI::GetFogDistances( float *fStart, float *fEnd, float *fFogZ )
{
}


void CShaderAPI::SceneFogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
}


void CShaderAPI::SceneFogMode( MaterialFogMode_t fogMode )
{
}

void CShaderAPI::GetSceneFogColor( unsigned char *rgb )
{
}

MaterialFogMode_t CShaderAPI::GetSceneFogMode( )
{
	return MATERIAL_FOG_NONE;
}

int CShaderAPI::GetPixelFogCombo( )
{
	return 0;
}

void CShaderAPI::FogColor3f( float r, float g, float b )
{
}

void CShaderAPI::FogColor3fv( float const* rgb )
{
}

void CShaderAPI::FogColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
}

void CShaderAPI::FogColor3ubv( unsigned char const* rgb )
{
}

void CShaderAPI::SetViewports( int nCount, const ShaderViewport_t* pViewports )
{
}

int CShaderAPI::GetViewports( ShaderViewport_t* pViewports, int nMax ) const
{
	return 1;
}

// Sets the vertex and pixel shaders
void CShaderAPI::SetVertexShaderIndex( int vshIndex )
{
}

void CShaderAPI::SetPixelShaderIndex( int pshIndex )
{
}

// Sets the constant registers for vertex and pixel shaders
void CShaderAPI::SetVertexShaderConstant( int var, float const* pVec, int numConst, bool bForce )
{
}

void CShaderAPI::SetBooleanVertexShaderConstant( int var, BOOL const* pVec, int numConst, bool bForce )
{
}

void CShaderAPI::SetIntegerVertexShaderConstant( int var, int const* pVec, int numConst, bool bForce )
{
}

void CShaderAPI::SetPixelShaderConstant( int var, float const* pVec, int numConst, bool bForce )
{
}

void CShaderAPI::SetBooleanPixelShaderConstant( int var, BOOL const* pVec, int numBools, bool bForce )
{
}

void CShaderAPI::SetIntegerPixelShaderConstant( int var, int const* pVec, int numIntVecs, bool bForce )
{
}

void CShaderAPI::InvalidateDelayedShaderConstants( void )
{
}

float CShaderAPI::GammaToLinear_HardwareSpecific( float fGamma ) const
{
	return 0.0f;
}

float CShaderAPI::LinearToGamma_HardwareSpecific( float fLinear ) const
{
	return 0.0f;
}

void CShaderAPI::SetLinearToGammaConversionTextures( ShaderAPITextureHandle_t hSRGBWriteEnabledTexture, ShaderAPITextureHandle_t hIdentityTexture )
{
}


// Returns the nearest supported format
ImageFormat CShaderAPI::GetNearestSupportedFormat( ImageFormat fmt ) const
{
	return fmt;
}

ImageFormat CShaderAPI::GetNearestRenderTargetFormat( ImageFormat fmt ) const
{
	return fmt;
}

// Sets the texture state
void CShaderAPI::BindTexture( Sampler_t stage, ShaderAPITextureHandle_t textureHandle )
{
}

void CShaderAPI::ClearColor3ub( unsigned char r, unsigned char g, unsigned char b )
{
}

void CShaderAPI::ClearColor4ub( unsigned char r, unsigned char g, unsigned char b, unsigned char a )
{
}

// Indicates we're going to be modifying this texture
// TexImage2D, TexSubImage2D, TexWrap, TexMinFilter, and TexMagFilter
// all use the texture specified by this function.
void CShaderAPI::ModifyTexture( ShaderAPITextureHandle_t textureHandle )
{
}

// Texture management methods
void CShaderAPI::TexImage2D( int level, int cubeFace, ImageFormat dstFormat, int zOffset, int width, int height, 
						 ImageFormat srcFormat, bool bSrcIsTiled, void *imageData )
{
}

void CShaderAPI::TexSubImage2D( int level, int cubeFace, int xOffset, int yOffset, int zOffset, int width, int height,
						 ImageFormat srcFormat, int srcStride, bool bSrcIsTiled, void *imageData )
{
}

bool CShaderAPI::TexLock( int level, int cubeFaceID, int xOffset, int yOffset, 
								int width, int height, CPixelWriter& writer )
{
	return false;
}

void CShaderAPI::TexUnlock( )
{
}


// These are bound to the texture, not the texture environment
void CShaderAPI::TexMinFilter( ShaderTexFilterMode_t texFilterMode )
{
}

void CShaderAPI::TexMagFilter( ShaderTexFilterMode_t texFilterMode )
{
}

void CShaderAPI::TexWrap( ShaderTexCoordComponent_t coord, ShaderTexWrapMode_t wrapMode )
{
}

void CShaderAPI::TexSetPriority( int priority )
{
}

ShaderAPITextureHandle_t CShaderAPI::CreateTexture( 
	int width, 
	int height,
	int depth,
	ImageFormat dstImageFormat, 
	int numMipLevels, 
	int numCopies, 
	int flags, 
	const char *pDebugName,
	const char *pTextureGroupName )
{
	return 0;
}

// Create a multi-frame texture (equivalent to calling "CreateTexture" multiple times, but more efficient)
void CShaderAPI::CreateTextures( 
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
							const char *pTextureGroupName )
{
	for ( int k = 0; k < count; ++ k )
		pHandles[ k ] = 0;
}


ShaderAPITextureHandle_t CShaderAPI::CreateDepthTexture( ImageFormat renderFormat, int width, int height, const char *pDebugName, bool bTexture )
{
	return 0;
}

void CShaderAPI::DeleteTexture( ShaderAPITextureHandle_t textureHandle )
{
}

bool CShaderAPI::IsTexture( ShaderAPITextureHandle_t textureHandle )
{
	return true;
}

bool CShaderAPI::IsTextureResident( ShaderAPITextureHandle_t textureHandle )
{
	return false;
}

// stuff that isn't to be used from within a shader
void CShaderAPI::ClearBuffers( bool bClearColor, bool bClearDepth, bool bClearStencil, int renderTargetWidth, int renderTargetHeight )
{
}

void CShaderAPI::ClearBuffersObeyStencil( bool bClearColor, bool bClearDepth )
{
}

void CShaderAPI::PerformFullScreenStencilOperation( void )
{
}

void CShaderAPI::SetScissorRect( const int nLeft, const int nTop, const int nRight, const int nBottom, const bool bEnableScissor )
{
}

void CShaderAPI::ReadPixels( int x, int y, int width, int height, unsigned char *data, ImageFormat dstFormat )
{
}

void CShaderAPI::ReadPixels( Rect_t *pSrcRect, Rect_t *pDstRect, unsigned char *data, ImageFormat dstFormat, int nDstStride )
{
}

void CShaderAPI::FlushHardware()
{
}

void CShaderAPI::ResetRenderState( bool bFullReset )
{
}

// Set the number of bone weights
void CShaderAPI::SetNumBoneWeights( int numBones )
{
}

void CShaderAPI::EnableHWMorphing( bool bEnable )
{
}

// Selection mode methods
int CShaderAPI::SelectionMode( bool selectionMode )
{
	return 0;
}

void CShaderAPI::SelectionBuffer( unsigned int* pBuffer, int size )
{
}

void CShaderAPI::ClearSelectionNames( )
{
}

void CShaderAPI::LoadSelectionName( int name )
{
}

void CShaderAPI::PushSelectionName( int name )
{
}

void CShaderAPI::PopSelectionName()
{
}


// Use this to get the mesh builder that allows us to modify vertex data
CMeshBuilder* CShaderAPI::GetVertexModifyBuilder()
{
	return 0;
}

// Board-independent calls, here to unify how shaders set state
// Implementations should chain back to IShaderUtil->BindTexture(), etc.

// Use this to begin and end the frame
void CShaderAPI::BeginFrame()
{
}

void CShaderAPI::EndFrame()
{
}

// returns the current time in seconds....
double CShaderAPI::CurrentTime() const
{
	return 0.0;
}

// Get the current camera position in world space.
void CShaderAPI::GetWorldSpaceCameraPosition( float * pPos ) const
{
}

void CShaderAPI::ForceHardwareSync( void )
{
}

void CShaderAPI::SetClipPlane( int index, const float *pPlane )
{
}

void CShaderAPI::EnableClipPlane( int index, bool bEnable )
{
}

void CShaderAPI::SetFastClipPlane( const float *pPlane )
{
}

void CShaderAPI::EnableFastClip( bool bEnable )
{
}

int CShaderAPI::GetCurrentNumBones( void ) const
{
	return 0;
}

bool CShaderAPI::IsHWMorphingEnabled( void ) const
{
	return false;
}

int CShaderAPI::GetCurrentLightCombo( void ) const
{
	return 0;
}

void CShaderAPI::GetDX9LightState( LightState_t *state ) const
{
	state->m_nNumLights = 0;
	state->m_bAmbientLight = false;
	state->m_bStaticLight = false;
}

MaterialFogMode_t CShaderAPI::GetCurrentFogType( void ) const
{
	return MATERIAL_FOG_NONE;
}

void CShaderAPI::RecordString( const char *pStr )
{
}

bool CShaderAPI::ReadPixelsFromFrontBuffer() const
{
	return true;
}

bool CShaderAPI::PreferDynamicTextures() const
{
	return false;
}

bool CShaderAPI::PreferReducedFillrate() const
{ 
	return false; 
}

bool CShaderAPI::HasProjectedBumpEnv() const
{
	return true;
}

int  CShaderAPI::GetCurrentDynamicVBSize( void )
{
	return 0;
}

void CShaderAPI::DestroyVertexBuffers( bool bExitingLevel )
{
}

void CShaderAPI::EvictManagedResources()
{
}

void CShaderAPI::SetTextureTransformDimension( TextureStage_t textureStage, int dimension, bool projected )
{
}

void CShaderAPI::SetBumpEnvMatrix( TextureStage_t textureStage, float m00, float m01, float m10, float m11 )
{
}

void CShaderAPI::SyncToken( const char *pToken )
{
}
