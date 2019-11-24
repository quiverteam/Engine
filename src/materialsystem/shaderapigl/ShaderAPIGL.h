//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Empty template shaderapi
// 			Replace this when creating a new shaderapi
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
class CEmptyMesh : public CMesh {};

//-----------------------------------------------------------------------------
// The empty shader shadow
//-----------------------------------------------------------------------------
class CShaderShadowEmpty : public CShaderShadow {};

//-----------------------------------------------------------------------------
// The DX8 implementation of the shader device
//-----------------------------------------------------------------------------
class CShaderDeviceEmpty : public CShaderDevice {};

static CShaderDeviceEmpty s_ShaderDeviceEmpty;

// FIXME: Remove; it's for backward compat with the materialsystem only for now
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderDeviceEmpty, IShaderDevice, 
								  SHADER_DEVICE_INTERFACE_VERSION, s_ShaderDeviceEmpty )


//-----------------------------------------------------------------------------
// The DX8 implementation of the shader device
//-----------------------------------------------------------------------------
class CShaderDeviceMgrEmpty : public CShaderDeviceManager {};

static CShaderDeviceMgrEmpty s_ShaderDeviceMgrEmpty;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderDeviceMgrEmpty, IShaderDeviceMgr, 
								  SHADER_DEVICE_MGR_INTERFACE_VERSION, s_ShaderDeviceMgrEmpty )


//-----------------------------------------------------------------------------
// The DX8 implementation of the shader API
//-----------------------------------------------------------------------------
class CShaderAPIEmpty : public CShaderAPI {};

//-----------------------------------------------------------------------------
// Class Factory
//-----------------------------------------------------------------------------

static CShaderAPI g_ShaderAPIEmpty;
static CShaderShadowEmpty g_ShaderShadow;

// FIXME: Remove; it's for backward compat with the materialsystem only for now
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIEmpty, IShaderAPI, 
									SHADERAPI_INTERFACE_VERSION, g_ShaderAPIEmpty )

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderShadowEmpty, IShaderShadow, 
								SHADERSHADOW_INTERFACE_VERSION, g_ShaderShadow )

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIEmpty, IMaterialSystemHardwareConfig, 
				MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, g_ShaderAPIEmpty )

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CShaderAPIEmpty, IDebugTextureInfo, 
				DEBUG_TEXTURE_INFO_VERSION, g_ShaderAPIEmpty )


//-----------------------------------------------------------------------------
// The main GL Shader util interface
//-----------------------------------------------------------------------------
extern IShaderUtil* g_pShaderUtil;

