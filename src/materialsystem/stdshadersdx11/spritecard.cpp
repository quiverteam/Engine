//===== Copyright © 1996-2007, Valve Corporation, All rights reserved. ======//
//
// Purpose: shader for drawing sprites as cards, with animation frame lerping
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "convar.h"

#include "spritecard_ps40.inc"
#include "spritecard_vs40.inc"
#include "splinecard_vs40.inc"

#include "tier0/icommandline.h" //command line

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DEFAULT_PARTICLE_FEATHERING_ENABLED 1

CREATE_CONSTANT_BUFFER( SpriteCard )
{
	Vector4D ScaleParms;
	Vector4D SizeParms;
	Vector4D SizeParms2;
	IntVector4D SpriteControls;

	Vector4D PixelParms;
	Vector4D DepthFeatheringConstants;
};

int GetDefaultDepthFeatheringValue( void ) //Allow the command-line to go against the default soft-particle value
{
	static int iRetVal = -1;

	if( iRetVal == -1 )
	{
#		if( DEFAULT_PARTICLE_FEATHERING_ENABLED == 1 )
		{
			if( CommandLine()->CheckParm( "-softparticlesdefaultoff" ) )
				iRetVal = 0;
			else
				iRetVal = 1;
		}
#		else
		{
			if( CommandLine()->CheckParm( "-softparticlesdefaulton" ) )
				iRetVal = 1;
			else
				iRetVal = 0;
		}
#		endif
	}

	return iRetVal;
}


BEGIN_VS_SHADER_FLAGS( Spritecard, "Help for Spritecard", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
        SHADER_PARAM( DEPTHBLEND, SHADER_PARAM_TYPE_INTEGER, "0", "fade at intersection boundaries" )
		SHADER_PARAM( DEPTHBLENDSCALE, SHADER_PARAM_TYPE_FLOAT, "50.0", "Amplify or reduce DEPTHBLEND fading. Lower values make harder edges." )
	    SHADER_PARAM( ORIENTATION, SHADER_PARAM_TYPE_INTEGER, "0", "0 = always face camera, 1 = rotate around z, 2= parallel to ground" )
	    SHADER_PARAM( ADDBASETEXTURE2, SHADER_PARAM_TYPE_FLOAT, "0.0", "amount to blend second texture into frame by" )
	    SHADER_PARAM( OVERBRIGHTFACTOR, SHADER_PARAM_TYPE_FLOAT, "1.0", "overbright factor for texture. For HDR effects.")
	    SHADER_PARAM( DUALSEQUENCE, SHADER_PARAM_TYPE_INTEGER, "0", "blend two separate animated sequences.")
	    SHADER_PARAM( SEQUENCE_BLEND_MODE, SHADER_PARAM_TYPE_INTEGER, "0", "defines the blend mode between the images un dual sequence particles. 0 = avg, 1=alpha from first, rgb from 2nd, 2= first over second" )
		SHADER_PARAM( MAXLUMFRAMEBLEND1, SHADER_PARAM_TYPE_INTEGER, "0", "instead of blending between animation frames for the first sequence, select pixels based upon max luminance" )
		SHADER_PARAM( MAXLUMFRAMEBLEND2, SHADER_PARAM_TYPE_INTEGER, "0", "instead of blending between animation frames for the 2nd sequence, select pixels based upon max luminance" )
		SHADER_PARAM( RAMPTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "if specified, then the red value of the image is used to index this ramp to produce the output color" )
	    SHADER_PARAM( ZOOMANIMATESEQ2, SHADER_PARAM_TYPE_FLOAT, "1.0", "amount to gradually zoom between frames on the second sequence. 2.0 will double the size of a frame over its lifetime.")
	    SHADER_PARAM( EXTRACTGREENALPHA, SHADER_PARAM_TYPE_INTEGER, "0", "grayscale data sitting in green/alpha channels")
		SHADER_PARAM( ADDOVERBLEND, SHADER_PARAM_TYPE_INTEGER, "0", "use ONE:INVSRCALPHA blending")
	    SHADER_PARAM( ADDSELF, SHADER_PARAM_TYPE_FLOAT, "0.0", "amount of base texture to additively blend in" )
	    SHADER_PARAM( BLENDFRAMES, SHADER_PARAM_TYPE_BOOL, "1", "whether or not to smoothly blend between animated frames" )
	    SHADER_PARAM( MINSIZE, SHADER_PARAM_TYPE_FLOAT, "0.0", "minimum screen fractional size of particle")
	    SHADER_PARAM( STARTFADESIZE, SHADER_PARAM_TYPE_FLOAT, "10.0", "screen fractional size to start fading particle out")
	    SHADER_PARAM( ENDFADESIZE, SHADER_PARAM_TYPE_FLOAT, "20.0", "screen fractional size to finish fading particle out")
	    SHADER_PARAM( MAXSIZE, SHADER_PARAM_TYPE_FLOAT, "20.0", "maximum screen fractional size of particle")
	    SHADER_PARAM( USEINSTANCING, SHADER_PARAM_TYPE_BOOL, "1", "whether to use GPU vertex instancing (submit 1 vert per particle quad)")
	    SHADER_PARAM( SPLINETYPE, SHADER_PARAM_TYPE_INTEGER, "0", "spline type 0 = none,  1=ctamull rom")
	    SHADER_PARAM( MAXDISTANCE, SHADER_PARAM_TYPE_FLOAT, "100000.0", "maximum distance to draw particles at")
	    SHADER_PARAM( FARFADEINTERVAL, SHADER_PARAM_TYPE_FLOAT, "400.0", "interval over which to fade out far away particles")
	END_SHADER_PARAMS

	DECLARE_CONSTANT_BUFFER(SpriteCard)

	SHADER_INIT_GLOBAL
	{
		INIT_CONSTANT_BUFFER( SpriteCard );
	}

	SHADER_INIT_PARAMS()
	{
		INIT_FLOAT_PARM( MAXDISTANCE, 100000.0);
		INIT_FLOAT_PARM( FARFADEINTERVAL, 400.0);
		INIT_FLOAT_PARM( MAXSIZE, 20.0 );
		INIT_FLOAT_PARM( ENDFADESIZE, 20.0 );
		INIT_FLOAT_PARM( STARTFADESIZE, 10.0 );
		INIT_FLOAT_PARM( DEPTHBLENDSCALE, 50.0 );
		INIT_FLOAT_PARM( OVERBRIGHTFACTOR, 1.0 );
		INIT_FLOAT_PARM( ADDBASETEXTURE2, 0.0 );
		INIT_FLOAT_PARM( ADDSELF, 0.0 );
		INIT_FLOAT_PARM( ZOOMANIMATESEQ2, 0.0 );

		if ( !params[DEPTHBLEND]->IsDefined() )
		{
			params[ DEPTHBLEND ]->SetIntValue( GetDefaultDepthFeatheringValue() );
		}
		if ( !g_pHardwareConfig->SupportsPixelShaders_2_b() )
		{
			params[ DEPTHBLEND ]->SetIntValue( 0 );
		}
		if ( !params[DUALSEQUENCE]->IsDefined() )
		{
			params[DUALSEQUENCE]->SetIntValue( 0 );
		}
		if ( !params[MAXLUMFRAMEBLEND1]->IsDefined() )
		{
			params[MAXLUMFRAMEBLEND1]->SetIntValue( 0 );
		}
		if ( !params[MAXLUMFRAMEBLEND2]->IsDefined() )
		{
			params[MAXLUMFRAMEBLEND2]->SetIntValue( 0 );
		}
		if ( !params[EXTRACTGREENALPHA]->IsDefined() )
		{
			params[EXTRACTGREENALPHA]->SetIntValue( 0 );
		}
		if ( !params[ADDOVERBLEND]->IsDefined() )
		{
			params[ADDOVERBLEND]->SetIntValue( 0 );
		}
		if ( !params[BLENDFRAMES]->IsDefined() )
		{
			params[ BLENDFRAMES ]->SetIntValue( 1 );
		}
		if ( !params[USEINSTANCING]->IsDefined() )
		{
			params[ USEINSTANCING ]->SetIntValue( IsX360() ? 1 : 0 );
		}
		SET_FLAGS2( MATERIAL_VAR2_IS_SPRITECARD );
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );

		if ( params[BASETEXTURE]->IsDefined() )
		{
			bool bExtractGreenAlpha = false;
			if ( params[EXTRACTGREENALPHA]->IsDefined() )
				bExtractGreenAlpha = params[EXTRACTGREENALPHA]->GetIntValue() != 0;

			LoadTexture( BASETEXTURE );
		}
		if ( params[RAMPTEXTURE]->IsDefined() )
		{
			LoadTexture( RAMPTEXTURE );
		}
	}

	SHADER_DRAW
	{
		bool bUseRampTexture = ( params[RAMPTEXTURE]->IsDefined() );
		bool bZoomSeq2 = ( ( params[ZOOMANIMATESEQ2]->GetFloatValue()) > 1.0 );
		bool bDepthBlend = ( params[DEPTHBLEND]->GetIntValue() != 0 );
		bool bAdditive2ndTexture = params[ADDBASETEXTURE2]->GetFloatValue() != 0.0;
		bool bExtractGreenAlpha = ( params[EXTRACTGREENALPHA]->GetIntValue() != 0 );
		int nSplineType = params[SPLINETYPE]->GetIntValue();
		bool bUseInstancing = false;

		SHADOW_STATE
		{
			bool bSecondSequence = params[DUALSEQUENCE]->GetIntValue() != 0;
			bool bAddOverBlend = params[ADDOVERBLEND]->GetIntValue() != 0;
			bool bBlendFrames = ( params[BLENDFRAMES]->GetIntValue() != 0 );
			if ( nSplineType )
			{
				bSecondSequence = false;
				bBlendFrames = false;
				bUseInstancing = false;
				bExtractGreenAlpha = false;
				bAdditive2ndTexture = false;
			}
			bool bAddSelf = params[ADDSELF]->GetFloatValue() != 0.0;

			// draw back-facing because of yaw spin
			pShaderShadow->EnableCulling( false );

			// Be sure not to write to dest alpha
			pShaderShadow->EnableAlphaWrites( false );

			if ( bAdditive2ndTexture || bAddOverBlend || bAddSelf )
			{
				EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			else
			{
				if ( IS_FLAG_SET(MATERIAL_VAR_ADDITIVE) )
				{
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				}
				else
				{
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
				}
			}

			unsigned int flags = VERTEX_POSITION | VERTEX_COLOR;
			static int s_TexCoordSize[8]={4,				// 0 = sheet bounding uvs, frame0
										  4,				// 1 = sheet bounding uvs, frame 1
										  4,				// 2 = frame blend, rot, radius, ???
										  2,				// 3 = corner identifier ( 0/0,1/0,1/1, 1/0 )
										  4,				// 4 = texture 2 bounding uvs
										  4,				// 5 = second sequence bounding uvs, frame0
										  4,				// 6 = second sequence bounding uvs, frame1
										  4,				// 7 = second sequence frame blend, ?,?,?
			};
			static int s_TexCoordSizeSpline[]={4,				// 0 = sheet bounding uvs, frame0
											   4,				// 1 = sheet bounding uvs, frame 1
											   4,				// 2 = frame blend, rot, radius, ???
											   4,				// 3 = corner identifier ( 0/0,1/0,1/1, 1/0 )
											   4,				// 4 = texture 2 bounding uvs
											   4,				// 5 = second sequence bounding uvs, frame0
											   4,				// 6 = second sequence bounding uvs, frame1
											   4,				// 7 = second sequence frame blend, ?,?,?
			};

			int numTexCoords = 4;
			if ( bAdditive2ndTexture )
			{
				numTexCoords = 5;
			}
			if ( bSecondSequence )
			{
				// the whole shebang - 2 sequences, with a possible multi-image sequence first
				numTexCoords = 8;
			}
			pShaderShadow->VertexShaderVertexFormat( flags,
													 numTexCoords, 
													 nSplineType? s_TexCoordSizeSpline : s_TexCoordSize, 0 );

			if ( nSplineType )
			{
				SetVertexShaderConstantBuffer( 0, SHADER_CONSTANTBUFFER_PERFRAME );
				SetVertexShaderConstantBuffer( 1, SHADER_CONSTANTBUFFER_PERSCENE );

				DECLARE_STATIC_VERTEX_SHADER( splinecard_vs40 );
				SET_STATIC_VERTEX_SHADER( splinecard_vs40 );
			}
			else
			{
				SetVertexShaderConstantBuffer( 0, SHADER_CONSTANTBUFFER_PERMODEL );
				SetVertexShaderConstantBuffer( 1, SHADER_CONSTANTBUFFER_PERFRAME );
				SetVertexShaderConstantBuffer( 2, SHADER_CONSTANTBUFFER_PERSCENE );
				SetVertexShaderConstantBuffer( 3, CONSTANT_BUFFER( SpriteCard ) );

				DECLARE_STATIC_VERTEX_SHADER( spritecard_vs40 );
				SET_STATIC_VERTEX_SHADER_COMBO( DUALSEQUENCE, bSecondSequence );
				SET_STATIC_VERTEX_SHADER( spritecard_vs40 );
			}

			SetPixelShaderConstantBuffer( 0, CONSTANT_BUFFER( SpriteCard ) );

			DECLARE_STATIC_PIXEL_SHADER( spritecard_ps40 );
			SET_STATIC_PIXEL_SHADER_COMBO( ADDBASETEXTURE2, bAdditive2ndTexture );
			SET_STATIC_PIXEL_SHADER_COMBO( ADDSELF, bAddSelf );
			SET_STATIC_PIXEL_SHADER_COMBO( ANIMBLEND, bBlendFrames );
			SET_STATIC_PIXEL_SHADER_COMBO( DUALSEQUENCE, bSecondSequence );
			SET_STATIC_PIXEL_SHADER_COMBO( SEQUENCE_BLEND_MODE, bSecondSequence ? params[SEQUENCE_BLEND_MODE]->GetIntValue() : 0 );
			SET_STATIC_PIXEL_SHADER_COMBO( MAXLUMFRAMEBLEND1, params[MAXLUMFRAMEBLEND1]->GetIntValue() );
			SET_STATIC_PIXEL_SHADER_COMBO( MAXLUMFRAMEBLEND2, bSecondSequence? params[MAXLUMFRAMEBLEND1]->GetIntValue() : 0 );
			SET_STATIC_PIXEL_SHADER_COMBO( COLORRAMP, bUseRampTexture );
			SET_STATIC_PIXEL_SHADER_COMBO( EXTRACTGREENALPHA, bExtractGreenAlpha );
			SET_STATIC_PIXEL_SHADER_COMBO( DEPTHBLEND, bDepthBlend );
			SET_STATIC_PIXEL_SHADER_COMBO( ALPHATEST, !( bAdditive2ndTexture || bAddSelf ) );
			SET_STATIC_PIXEL_SHADER( spritecard_ps40 );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

			if ( bUseRampTexture )
			{
				BindTexture( SHADER_SAMPLER1, RAMPTEXTURE, FRAME );
			}

			if ( bDepthBlend )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER2, TEXTURE_FRAME_BUFFER_FULL_DEPTH );
			}

			int nOrientation = params[ORIENTATION]->GetIntValue();
			nOrientation = clamp( nOrientation, 0, 2 );

			ALIGN16 CONSTANT_BUFFER_TYPE( SpriteCard ) consts;

			if ( bZoomSeq2 )
			{
				float flZScale=1.0/(params[ZOOMANIMATESEQ2]->GetFloatValue());
				consts.ScaleParms.Init( 0.5 * ( 1.0 + flZScale ), flZScale, 0, 0 );
			}

			// set fade constants in vsconsts 8 and 9
			float flMaxDistance = params[MAXDISTANCE]->GetFloatValue();
			float flStartFade = max( 1.0, flMaxDistance - params[FARFADEINTERVAL]->GetFloatValue() );

			float VC0[8]={ params[MINSIZE]->GetFloatValue(), params[MAXSIZE]->GetFloatValue(),
						   params[STARTFADESIZE]->GetFloatValue(), params[ENDFADESIZE]->GetFloatValue(),
						   flStartFade, 1.0/(flMaxDistance-flStartFade),
						   0,0 };

			consts.SizeParms = VC0;
			consts.SizeParms2 = VC0 + 4;
			consts.SpriteControls.Init( bZoomSeq2, bExtractGreenAlpha, bUseInstancing, 0 );

			// FIXME
			//pShaderAPI->SetDepthFeatheringPixelShaderConstant( 2, params[DEPTHBLENDSCALE]->GetFloatValue() );
			consts.DepthFeatheringConstants.Init();

			float C0[4]={ params[ADDBASETEXTURE2]->GetFloatValue(),
						  params[OVERBRIGHTFACTOR]->GetFloatValue(),
						  params[ADDSELF]->GetFloatValue(),
						  0.0f };
			consts.PixelParms = C0;

			UPDATE_CONSTANT_BUFFER( SpriteCard, consts );

			if ( nSplineType )
			{
				DECLARE_DYNAMIC_VERTEX_SHADER( splinecard_vs40 );
				SET_DYNAMIC_VERTEX_SHADER( splinecard_vs40 );
			}
			else
			{
				DECLARE_DYNAMIC_VERTEX_SHADER( spritecard_vs40 );
				SET_DYNAMIC_VERTEX_SHADER_COMBO( ORIENTATION, nOrientation );
				SET_DYNAMIC_VERTEX_SHADER( spritecard_vs40 );
			}

			DECLARE_DYNAMIC_PIXEL_SHADER( spritecard_ps40 );
			SET_DYNAMIC_PIXEL_SHADER( spritecard_ps40 );
		}
		Draw( );
	}
END_SHADER
