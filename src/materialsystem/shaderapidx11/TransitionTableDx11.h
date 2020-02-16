//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef TRANSITION_TABLE_H
#define TRANSITION_TABLE_H

#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "shadershadowdx11.h"
#include "utlsortvector.h"
#include "checksum_crc.h"
#include "shaderapi/ishaderapi.h"

// Required for DEBUG_BOARD_STATE
#include "shaderapidx11_global.h"


//-----------------------------------------------------------------------------
// The DX11 implementation of the transition table
//-----------------------------------------------------------------------------
class CTransitionTableDx11
{
public:
	struct CurrentState_t
	{
		// Everything in this 'CurrentState' structure is a state whose value we don't care about
		// under certain circumstances, (which therefore can diverge from the shadow state),
		// or states which we override in the dynamic pass.

		// Alpha state
		bool				m_AlphaBlendEnable;
		D3D11_BLEND			m_SrcBlend;
		D3D11_BLEND			m_DestBlend;

		// GR - Separate alpha state
		bool				m_SeparateAlphaBlendEnable;
		D3D11_BLEND			m_SrcBlendAlpha;
		D3D11_BLEND			m_DestBlendAlpha;

		// Depth testing states
		bool		m_ZEnable;
		D3D11_COMPARISON_FUNC			m_ZFunc;
		PolygonOffsetMode_t	m_ZBias;

		// Alpha testing states
		bool				m_AlphaTestEnable;
		D3D11_COMPARISON_FUNC			m_AlphaFunc;
		int					m_AlphaRef;

		bool				m_ForceDepthFuncEquals;
		bool				m_bOverrideDepthEnable;
		bool		m_OverrideZWriteEnable;

		bool				m_bLinearColorSpaceFrameBufferEnable;

		bool				m_StencilEnable;
		D3D11_COMPARISON_FUNC			m_StencilFunc;
		int					m_StencilRef;
		int					m_StencilMask;
		DWORD				m_StencilFail;
		DWORD				m_StencilZFail;
		DWORD				m_StencilPass;
		int					m_StencilWriteMask;
	};

public:
	// constructor, destructor
	CTransitionTableDx11( );
	virtual ~CTransitionTableDx11();

	// Initialization, shutdown
	bool Init( );
	void Shutdown( );

	// Resets the snapshots...
	void Reset();

	// Takes a snapshot
	StateSnapshot_t TakeSnapshot( );

	// Take startup snapshot
	void TakeDefaultStateSnapshot( );

	// Makes the board state match the snapshot
	void UseSnapshot( StateSnapshot_t snapshotId );

	// Cause the board to match the default state snapshot
	void UseDefaultState();

	// Snapshotted state overrides
	void ForceDepthFuncEquals( bool bEnable );
	void OverrideDepthEnable( bool bEnable, bool bDepthEnable );
	void EnableLinearColorSpaceFrameBuffer( bool bEnable );

	// Returns a particular snapshot
	const ShadowStateDx11_t &GetSnapshot( StateSnapshot_t snapshotId ) const;
	const ShadowShaderStateDx11_t &GetSnapshotShader( StateSnapshot_t snapshotId ) const;

	// Gets the current shadow state
	const ShadowStateDx11_t *CurrentShadowState() const;
	const ShadowShaderStateDx11_t *CurrentShadowShaderState() const;

	// Return the current shapshot
	int CurrentSnapshot() const { return m_CurrentSnapshotId; }

	CurrentState_t& CurrentState() { return m_CurrentState; }

#ifdef DEBUG_BOARD_STATE
	ShadowStateDx11_t& BoardState() { return m_BoardState; }
	ShadowShaderStateDx11_t& BoardShaderState() { return m_BoardShaderState; }
#endif

	// The following are meant to be used by the transition table only
public:
	// Applies alpha blending
	void ApplyAlphaBlend( const ShadowStateDx11_t& state );
	// GR - separate alpha blend
	void ApplySeparateAlphaBlend( const ShadowStateDx11_t& state );
	void ApplyAlphaTest( const ShadowStateDx11_t& state );
	void ApplyDepthTest( const ShadowStateDx11_t& state );

	void ApplySRGBWriteEnable( const ShadowStateDx11_t& state );
private:
	enum
	{
		INVALID_TRANSITION_OP = 0xFFFF
	};

	typedef short ShadowStateId_t;

	// For the transition table
	struct TransitionList_t
	{
		unsigned short m_FirstOperation;
		unsigned short m_NumOperations;
	};

	union TransitionOp_t
	{
		unsigned char m_nBits;
		struct
		{
			unsigned char m_nOpCode : 7;
			unsigned char m_bIsTextureCode : 1;
		} m_nInfo;
	};

	struct SnapshotShaderState_t
	{
		ShadowShaderStateDx11_t m_ShaderState;
		ShadowStateId_t m_ShadowStateId;
		unsigned short m_nReserved;	// Pad to 2 ints
		unsigned int m_nReserved2;
	};

	struct ShadowStateDictEntry_t
	{
		CRC32_t	m_nChecksum;
		ShadowStateId_t m_nShadowStateId;
	};

	struct SnapshotDictEntry_t
	{
		CRC32_t	m_nChecksum;
		StateSnapshot_t m_nSnapshot;
	};

	class ShadowStateDictLessFunc
	{
	public:
		bool Less( const ShadowStateDictEntry_t &src1, const ShadowStateDictEntry_t &src2, void *pCtx );
	};

	class SnapshotDictLessFunc
	{
	public:
		bool Less( const SnapshotDictEntry_t &src1, const SnapshotDictEntry_t &src2, void *pCtx );
	};

	class UniqueSnapshotLessFunc
	{
	public:
		bool Less( const TransitionList_t &src1, const TransitionList_t &src2, void *pCtx );
	};

	// creates state snapshots
	ShadowStateId_t  CreateShadowState( const ShadowStateDx11_t &currentState );
	StateSnapshot_t  CreateStateSnapshot( ShadowStateId_t shadowStateId, const ShadowShaderStateDx11_t& currentShaderState );

	// finds state snapshots
	ShadowStateId_t FindShadowState( const ShadowStateDx11_t& currentState ) const;
	StateSnapshot_t FindStateSnapshot( ShadowStateId_t id, const ShadowShaderStateDx11_t& currentState ) const;

	// Finds identical transition lists
	unsigned short FindIdenticalTransitionList( unsigned short firstElem, 
		unsigned short numOps, unsigned short nFirstTest ) const;

	// Adds a transition
	void AddTransition( RenderStateFunc_t func );

	// Apply a transition
	void ApplyTransition( TransitionList_t& list, int snapshot );

	// Creates an entry in the transition table
	void CreateTransitionTableEntry( int to, int from );

	// Checks if a state is valid
	bool TestShadowState( const ShadowStateDx11_t& state, const ShadowShaderStateDx11_t &shaderState );

	// Perform state block overrides
	void PerformShadowStateOverrides( );

	// Applies the transition list
	void ApplyTransitionList( int snapshot, int nFirstOp, int nOpCount );

	// Apply shader state (stuff that doesn't lie in the transition table)
	void ApplyShaderState( const ShadowStateDx11_t &shadowState, const ShadowShaderStateDx11_t &shaderState );

	// Wrapper for the non-standard transitions for stateblock + non-stateblock cases
	int CreateNormalTransitions( const ShadowStateDx11_t& fromState, const ShadowStateDx11_t& toState, bool bForce );

	// State setting methods
	void SetZEnable( bool nEnable );
	void SetZFunc( D3D11_COMPARISON_FUNC nCmpFunc );

private:
	// Sets up the default state
	StateSnapshot_t m_DefaultStateSnapshot;
	TransitionList_t m_DefaultTransition;
	ShadowStateDx11_t m_DefaultShadowState;
	
	// The current snapshot id
	ShadowStateId_t m_CurrentShadowId;
	StateSnapshot_t m_CurrentSnapshotId;

	// Maintains a list of all used snapshot transition states
	CUtlVector< ShadowStateDx11_t >	m_ShadowStateList;

	// Lookup table for fast snapshot finding
	CUtlSortVector< ShadowStateDictEntry_t, ShadowStateDictLessFunc >	m_ShadowStateDict;

	// The snapshot transition table
	CUtlVector< CUtlVector< TransitionList_t > >	m_TransitionTable;

	// List of unique transitions
	CUtlSortVector< TransitionList_t, UniqueSnapshotLessFunc >	m_UniqueTransitions;

	// Stores all state transition operations
	CUtlVector< TransitionOp_t > m_TransitionOps;

	// Stores all state for a particular snapshot
	CUtlVector< SnapshotShaderState_t >	m_SnapshotList;

	// Lookup table for fast snapshot finding
	CUtlSortVector< SnapshotDictEntry_t, SnapshotDictLessFunc >	m_SnapshotDict;

	// The current board state.
	CurrentState_t m_CurrentState;

#ifdef DEBUG_BOARD_STATE
	// Maintains the total shadow state
	ShadowStateDx11_t m_BoardState;
	ShadowShaderStateDx11_t m_BoardShaderState;
#endif
};


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline const ShadowStateDx11_t &CTransitionTableDx11::GetSnapshot( StateSnapshot_t snapshotId ) const
{
	Assert( (snapshotId >= 0) && (snapshotId < m_SnapshotList.Count()) );
	return m_ShadowStateList[m_SnapshotList[snapshotId].m_ShadowStateId];
}

inline const ShadowShaderStateDx11_t & CTransitionTableDx11::GetSnapshotShader( StateSnapshot_t snapshotId ) const
{
	Assert( (snapshotId >= 0) && (snapshotId < m_SnapshotList.Count()) );
	return m_SnapshotList[snapshotId].m_ShaderState;
}

inline const ShadowStateDx11_t * CTransitionTableDx11::CurrentShadowState() const
{
	if ( m_CurrentShadowId == -1 )
		return NULL;

	Assert( (m_CurrentShadowId >= 0) && (m_CurrentShadowId < m_ShadowStateList.Count()) );
	return &m_ShadowStateList[m_CurrentShadowId];
}

inline const ShadowShaderStateDx11_t * CTransitionTableDx11::CurrentShadowShaderState() const
{
	if ( m_CurrentShadowId == -1 )
		return NULL;

	Assert( (m_CurrentShadowId >= 0) && (m_CurrentShadowId < m_ShadowStateList.Count()) );
	return &m_SnapshotList[m_CurrentShadowId].m_ShaderState;
}


#endif // TRANSITION_TABLE_H
