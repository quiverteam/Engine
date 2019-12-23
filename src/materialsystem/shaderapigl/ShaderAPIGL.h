//===== Copyright (C) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Shaderapi for OpenGL
//
// $NoKeywords: $
//
//===========================================================================//
#pragma once

#include "Mesh.h"
#include "ShaderShadow.h"
#include "ShaderDevice.h"
#include "DeviceManager.h"
#include "ShaderAPI.h"

//-----------------------------------------------------------------------------
// The empty mesh
//-----------------------------------------------------------------------------
class CGLMesh : public CMesh {};

//-----------------------------------------------------------------------------
// The empty shader shadow
//-----------------------------------------------------------------------------
class CGLShaderShadow : public CShaderShadow {};

//-----------------------------------------------------------------------------
// The DX8 implementation of the shader device
//-----------------------------------------------------------------------------
class CGLShaderDevice : public CShaderDevice {};

static CShaderDeviceEmpty s_GLShaderDevice;

// FIXME: Remove; it's for backward compat with the materialsystem only for now
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( COpenGLShaderDevice, IShaderDevice, 
								  SHADER_DEVICE_INTERFACE_VERSION, s_GLShaderDevice )


//-----------------------------------------------------------------------------
// The DX8 implementation of the shader device
//-----------------------------------------------------------------------------
class CGLShaderDeviceMgr : public CShaderDeviceManager {};

static CGLShaderDeviceMgr s_GLShaderDeviceMgr;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderDeviceMgrEmpty, IShaderDeviceMgr, 
								  SHADER_DEVICE_MGR_INTERFACE_VERSION, s_GLShaderDeviceMgr )


//-----------------------------------------------------------------------------
// The DX8 implementation of the shader API
//-----------------------------------------------------------------------------
class CGLShaderAPI : public CShaderAPIGL {};

//-----------------------------------------------------------------------------
// Class Factory
//-----------------------------------------------------------------------------

static CShaderAPIGL g_ShaderAPIGL;
static CGLShaderShadow g_ShaderShadow;

// FIXME: Remove; it's for backward compat with the materialsystem only for now
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIEmpty, IShaderAPI, 
									SHADERAPI_INTERFACE_VERSION, g_ShaderAPIGL )

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderShadowEmpty, IShaderShadow, 
								SHADERSHADOW_INTERFACE_VERSION, g_ShaderShadow )

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIEmpty, IMaterialSystemHardwareConfig, 
				MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, g_ShaderAPIGL )

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIEmpty, IDebugTextureInfo, 
				DEBUG_TEXTURE_INFO_VERSION, g_ShaderAPIGL )


//-----------------------------------------------------------------------------
// The main GL Shader util interface
//-----------------------------------------------------------------------------
extern IShaderUtil* g_pShaderUtil;

