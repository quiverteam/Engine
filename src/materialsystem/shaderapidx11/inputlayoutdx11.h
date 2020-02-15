//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef INPUTLAYOUTDX11_H
#define INPUTLAYOUTDX11_H

#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/IMaterial.h"


//-----------------------------------------------------------------------------
// Forward declaration
//-----------------------------------------------------------------------------
struct ID3D11InputLayout;
struct ID3D11ShaderReflection;


//-----------------------------------------------------------------------------
// Gets the input layout associated with a vertex format
// FIXME: Note that we'll need to change this from a VertexFormat_t
//-----------------------------------------------------------------------------
ID3D11InputLayout *CreateInputLayout( VertexFormat_t fmt, 
	 ID3D11ShaderReflection* pReflection, const void *pByteCode, size_t nByteCodeLen );


#endif // INPUTLAYOUTDX11_H 

