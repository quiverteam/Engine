//==== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Vertex/Pixel Shaders
//
//===========================================================================//

#if defined( _WIN32 ) && !defined( _X360 )
#include <windows.h>
#endif
#include "vertexshaderdx11.h"
#include "tier1/UtlSymbol.h"
#include "tier1/UtlVector.h"
#include "tier1/UtlDict.h"
#include "tier1/utllinkedlist.h"
#include "tier1/UtlBuffer.h"
#include "tier1/UtlStringMap.h"
#include "shaderapidx9/locald3dtypes.h"
#include "shaderapidx11_global.h"
#include "shaderapidx9/recording.h"
#include "tier0/vprof.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "shaderapidx11.h"
#include "materialsystem/ishader.h"
#include "ishadersystem.h"
#include "tier0/fasttimer.h"
#include <stdlib.h>
#include "filesystem.h"
#include "convar.h"
#include "materialsystem/shader_vcs_version.h"
#include "../thirdparty/bzip2/bzlib.h"
#include "tier1/lzmadecoder.h"
#include "tier1/utlmap.h"

#include "datacache/idatacache.h"
#include "tier1/diff.h"
#include "shaderdevicedx11.h"
#include "filesystem/IQueuedLoader.h"
#include "tier2/tier2.h"
#include "shaderapi/ishaderutil.h"
#include "tier0/icommandline.h"

#include "Color.h"
#include "tier0/dbg.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

// uncomment to get dynamic compilation for HLSL shaders
// i don't think this works atm, but im probably using this wrong
// X360 NOTE: By default, the system looks for a shared folder named "stdshaders" on the host machine and is completely compatible with -dvd. Ensure that the share is writable if you plan on generating UPDB's.
#define DYNAMIC_SHADER_COMPILE

// uncomment to get spew about what combos are being compiled.
//#define DYNAMIC_SHADER_COMPILE_VERBOSE

// uncomment and fill in with a path to use a specific set of shader source files. Meant for network use.
//		PC path format is of style "\\\\somemachine\\sourcetreeshare\\materialsystem\\stdshaders"
//		Xbox path format is of style "net:\\smb\\somemachine\\sourcetreeshare\\materialsystem\\stdshaders"
//			- Xbox dynamic compiles without a custom path default to look directly for "stdshaders" share on host pc

//#define DYNAMIC_SHADER_COMPILE_CUSTOM_PATH ""

// uncomment to get disassembled (asm) shader code in your game dir as *.asm
//#define DYNAMIC_SHADER_COMPILE_WRITE_ASSEMBLY

// uncomment to get disassembled (asm) shader code in your game dir as *.asm
//#define WRITE_ASSEMBLY

//#define PROFILE_SHADER_CREATE

// debugging aid
#define MAX_SHADER_HISTORY	16

#if !defined( _X360 )
#define SHADER_FNAME_EXTENSION	".vcs"
#else
#define SHADER_FNAME_EXTENSION	".360.vcs"
#endif

#ifdef DYNAMIC_SHADER_COMPILE
volatile static char s_ShaderCompileString[]="dynamic_shader_compile_is_on";
#endif

static const char *GetLightTypeName( VertexShaderLightTypes_t type )
{
	static const char *s_VertexShaderLightTypeNames[] = 
	{
		"LIGHT_NONE",
		"LIGHT_SPOT",
		"LIGHT_POINT",
		"LIGHT_DIRECTIONAL",
		"LIGHT_STATIC",
		"LIGHT_AMBIENTCUBE",
	};
	return s_VertexShaderLightTypeNames[type+1];
}

#ifdef PROFILE_SHADER_CREATE
static FILE *GetDebugFileHandle( void )
{
	static FILE *fp = NULL;
	if( !fp )
	{
		fp = fopen( "shadercreate.txt", "w" );
		Assert( fp );
	}
	return fp;
}
#endif // PROFILE_SHADER_CREATE


//-----------------------------------------------------------------------------
// Explicit instantiation of shader buffer implementation
//-----------------------------------------------------------------------------
template class CShaderBuffer< ID3DBlob >;


//-----------------------------------------------------------------------------
// Used to find unique shaders
//-----------------------------------------------------------------------------
#ifdef MEASURE_DRIVER_ALLOCATIONS
static CUtlMap< CRC32_t, int, int > s_UniqueVS( 0, 0, DefLessFunc( CRC32_t ) );
static CUtlMap< CRC32_t, int, int > s_UniquePS( 0, 0, DefLessFunc( CRC32_t ) );
static CUtlMap< IDirect3DVertexShader9*, CRC32_t, int > s_VSLookup( 0, 0, DefLessFunc( IDirect3DVertexShader9* ) );
static CUtlMap< IDirect3DPixelShader9*, CRC32_t, int > s_PSLookup( 0, 0, DefLessFunc( IDirect3DPixelShader9* ) );
#endif

static void RegisterVS( const void* pShaderBits, int nShaderSize, ID3D11VertexShader* pShader )
{
#ifdef MEASURE_DRIVER_ALLOCATIONS
	CRC32_t crc;
	CRC32_Init( &crc );
	CRC32_ProcessBuffer( &crc, pShaderBits, nShaderSize );
	CRC32_Final( &crc );

	s_VSLookup.Insert( pShader, crc );

	int nIndex = s_UniqueVS.Find( crc );
	if ( nIndex != s_UniqueVS.InvalidIndex() )
	{
		++s_UniqueVS[nIndex];
	}
	else
	{
		int nMemUsed = 23 * 1024;
		s_UniqueVS.Insert( crc, 1 );
		VPROF_INCREMENT_GROUP_COUNTER( "unique vs count", COUNTER_GROUP_NO_RESET, 1 );
		VPROF_INCREMENT_GROUP_COUNTER( "vs driver mem", COUNTER_GROUP_NO_RESET, nMemUsed );
		VPROF_INCREMENT_GROUP_COUNTER( "total driver mem", COUNTER_GROUP_NO_RESET, nMemUsed );
	}
#endif
}

static void RegisterPS( const void* pShaderBits, int nShaderSize, ID3D11PixelShader* pShader )
{
#ifdef MEASURE_DRIVER_ALLOCATIONS
	CRC32_t crc;
	CRC32_Init( &crc );
	CRC32_ProcessBuffer( &crc, pShaderBits, nShaderSize );
	CRC32_Final( &crc );

	s_PSLookup.Insert( pShader, crc );

	int nIndex = s_UniquePS.Find( crc );
	if ( nIndex != s_UniquePS.InvalidIndex() )
	{
		++s_UniquePS[nIndex];
	}
	else
	{
		int nMemUsed = 400;
		s_UniquePS.Insert( crc, 1 );
		VPROF_INCREMENT_GROUP_COUNTER( "unique ps count", COUNTER_GROUP_NO_RESET, 1 );
		VPROF_INCREMENT_GROUP_COUNTER( "ps driver mem", COUNTER_GROUP_NO_RESET, nMemUsed );
		VPROF_INCREMENT_GROUP_COUNTER( "total driver mem", COUNTER_GROUP_NO_RESET, nMemUsed );
	}
#endif
}

static void UnregisterVS( ID3D11VertexShader* pShader )
{
#ifdef MEASURE_DRIVER_ALLOCATIONS
	int nCRCIndex = s_VSLookup.Find( pShader );
	if ( nCRCIndex == s_VSLookup.InvalidIndex() )
		return;

	CRC32_t crc = s_VSLookup[nCRCIndex];
	s_VSLookup.RemoveAt( nCRCIndex );

	int nIndex = s_UniqueVS.Find( crc );
	if ( nIndex != s_UniqueVS.InvalidIndex() )
	{
		if ( --s_UniqueVS[nIndex] <= 0 )
		{
			int nMemUsed = 23 * 1024;
			VPROF_INCREMENT_GROUP_COUNTER( "unique vs count", COUNTER_GROUP_NO_RESET, -1 );
			VPROF_INCREMENT_GROUP_COUNTER( "vs driver mem", COUNTER_GROUP_NO_RESET, -nMemUsed );
			VPROF_INCREMENT_GROUP_COUNTER( "total driver mem", COUNTER_GROUP_NO_RESET, -nMemUsed );
			s_UniqueVS.Remove( nIndex );
		}
	}
#endif
}

static void UnregisterPS( ID3D11PixelShader* pShader )
{
#ifdef MEASURE_DRIVER_ALLOCATIONS
	int nCRCIndex = s_PSLookup.Find( pShader );
	if ( nCRCIndex == s_PSLookup.InvalidIndex() )
		return;

	CRC32_t crc = s_PSLookup[nCRCIndex];
	s_PSLookup.RemoveAt( nCRCIndex );

	int nIndex = s_UniquePS.Find( crc );
	if ( nIndex != s_UniquePS.InvalidIndex() )
	{
		if ( --s_UniquePS[nIndex] <= 0 )
		{
			int nMemUsed = 400;
			VPROF_INCREMENT_GROUP_COUNTER( "unique ps count", COUNTER_GROUP_NO_RESET, -1 );
			VPROF_INCREMENT_GROUP_COUNTER( "ps driver mem", COUNTER_GROUP_NO_RESET, -nMemUsed );
			VPROF_INCREMENT_GROUP_COUNTER( "total driver mem", COUNTER_GROUP_NO_RESET, -nMemUsed );
			s_UniquePS.Remove( nIndex );
		}
	}
#endif
}

template<class T> int BinarySearchCombos( uint32 nStaticComboID, int nCombos, T const *pRecords )
{
	// Use binary search - data is sorted
	int nLowerIdx = 1;
	int nUpperIdx = nCombos;
	for (;;)
	{
		if ( nUpperIdx < nLowerIdx )
			return -1;

		int nMiddleIndex = ( nLowerIdx + nUpperIdx ) / 2;
		uint32 nProbe = pRecords[nMiddleIndex-1].m_nStaticComboID;
		if ( nStaticComboID < nProbe )
		{
			nUpperIdx = nMiddleIndex - 1;
		}
		else
		{
			if ( nStaticComboID > nProbe )
				nLowerIdx = nMiddleIndex + 1;
			else
				return nMiddleIndex - 1;
		}
	}
}

inline int FindShaderStaticCombo( uint32 nStaticComboID, const ShaderHeader_t& header, StaticComboRecord_t *pRecords )
{
	if ( header.m_nVersion < 5 )
		return -1;

	return BinarySearchCombos( nStaticComboID, header.m_nNumStaticCombos, pRecords );
}

// cache redundant i/o fetched components of the vcs files
struct ShaderFileCache_t
{
	CUtlSymbol			m_Name;
	CUtlSymbol			m_Filename;
	ShaderHeader_t		m_Header;
	bool				m_bVertexShader;

	// valid for diff version only - contains the microcode used as the reference for diff algorithm
	CUtlBuffer			m_ReferenceCombo;

	// valid for ver5 only - contains the directory
	CUtlVector< StaticComboRecord_t >	m_StaticComboRecords;
	CUtlVector< StaticComboAliasRecord_t > m_StaticComboDupRecords;

	ShaderFileCache_t()
	{
		// invalid until version established
		m_Header.m_nVersion = 0;
	}

	bool IsValid() const
	{
		return m_Header.m_nVersion != 0;
	}

	bool IsOldVersion() const
	{
		return m_Header.m_nVersion < 5;
	}

	int IsVersion6() const
	{
		return ( m_Header.m_nVersion == 6 );
	}

	int FindCombo( uint32 nStaticComboID )
	{
		int nSearchAliases = BinarySearchCombos( nStaticComboID, m_StaticComboDupRecords.Count(), m_StaticComboDupRecords.Base() );
		if ( nSearchAliases != -1 )
			nStaticComboID = m_StaticComboDupRecords[nSearchAliases].m_nSourceStaticCombo;
		return FindShaderStaticCombo( nStaticComboID, m_Header, m_StaticComboRecords.Base() );
	}

	bool operator==( const ShaderFileCache_t& a ) const
	{
		return m_Name == a.m_Name && m_bVertexShader == a.m_bVertexShader;
	}
};


//-----------------------------------------------------------------------------
// Vertex + pixel shader manager
//-----------------------------------------------------------------------------
class CShaderManager : public IShaderManager
{
public:
	CShaderManager();
	virtual ~CShaderManager();

	// Methods of IShaderManager
	virtual void				Init();
	virtual void				Shutdown();
	virtual IShaderBuffer		*CompileShader( const char *pProgram, size_t nBufLen, const char *pShaderVersion );
	virtual VertexShaderHandle_t CreateVertexShader( IShaderBuffer* pShaderBuffer );
	virtual void				DestroyVertexShader( VertexShaderHandle_t hShader );
	virtual PixelShaderHandle_t CreatePixelShader( IShaderBuffer* pShaderBuffer );
	virtual void				DestroyPixelShader( PixelShaderHandle_t hShader );
	virtual VertexShader_t		CreateVertexShader( const char *pVertexShaderFile, ShaderIndex_t nStaticVshIndex = 0 );
	virtual PixelShader_t		CreatePixelShader( const char *pPixelShaderFile, ShaderIndex_t nStaticPshIndex = 0 );
	virtual void				SetVertexShader( VertexShader_t shader );
	virtual void				SetPixelShader( PixelShader_t shader );
	virtual void				BindVertexShader( VertexShaderHandle_t shader );
	virtual void				BindPixelShader( PixelShaderHandle_t shader );
	virtual void				*GetCurrentVertexShader();
	virtual void				*GetCurrentPixelShader();
	virtual void				ResetShaderState();
	void						FlushShaders();
	virtual void				ClearVertexAndPixelShaderRefCounts();
	virtual void				PurgeUnusedVertexAndPixelShaders();
	void						SpewVertexAndPixelShaders();
	const char					*GetActiveVertexShaderName();
	const char					*GetActivePixelShaderName();
	bool						CreateDynamicCombos_Ver4( void *pContext, uint8 *pComboBuffer );
	bool						CreateDynamicCombos_Ver5( void *pContext, uint8 *pComboBuffer );

	static void					QueuedLoaderCallback( void *pContext, void *pContext2, const void *pData, int nSize, LoaderError_t loaderError );

private:
	
	struct ShaderStaticCombos_t
	{
		int					m_nCount;

		// Can't use CUtlVector here since you CUtlLinkedList<CUtlVector<>> doesn't work.
		HardwareShader_t	*m_pHardwareShaders;
		struct ShaderCreationData_t
		{
			CUtlVector<uint8> 	ByteCode;
			uint32 				iCentroidMask;
		};

		ShaderCreationData_t *m_pCreationData;
	};
	
	struct ShaderLookupDx11_t
	{
		CUtlSymbol				m_Name;
		ShaderIndex_t						m_nStaticIndex;
		ShaderStaticCombos_t	m_ShaderStaticCombos;
		DWORD					m_Flags;
		int						m_nRefCount;
		unsigned int			m_hShaderFileCache;

		// for queued loading, bias an aligned optimal buffer forward to correct location
		int						m_nDataOffset;

		// diff version, valid during load only
		ShaderDictionaryEntry_t	*m_pComboDictionary;

		bool operator=( const ShaderLookupDx11_t &other )
		{
			m_Name = other.m_Name;
			m_nStaticIndex = other.m_nStaticIndex;
			m_ShaderStaticCombos = other.m_ShaderStaticCombos;
			m_Flags = other.m_Flags;
			m_nRefCount = other.m_nRefCount;
			m_hShaderFileCache = other.m_hShaderFileCache;
			m_nDataOffset = other.m_nDataOffset;
			m_pComboDictionary = other.m_pComboDictionary;
		}
		ShaderLookupDx11_t()
		{
			m_Flags = 0;
			m_nRefCount = 0;
			m_ShaderStaticCombos.m_nCount = 0;
			m_ShaderStaticCombos.m_pHardwareShaders = 0;
			m_ShaderStaticCombos.m_pCreationData = 0;
			m_pComboDictionary = NULL;
		}
		void IncRefCount()
		{
			m_nRefCount++;
		}
		bool operator==( const ShaderLookupDx11_t& a ) const
		{
			return m_Name == a.m_Name && m_nStaticIndex == a.m_nStaticIndex;
		}
	};

#ifdef DYNAMIC_SHADER_COMPILE
	struct Combo_t
	{
		CUtlSymbol m_ComboName;
		int m_nMin;
		int m_nMax;
	};

	struct ShaderCombos_t
	{
		CUtlVector<Combo_t> m_StaticCombos;
		CUtlVector<Combo_t> m_DynamicCombos;
		uint64 GetNumDynamicCombos( void ) const
		{
			uint64 combos = 1;
			int i;
			for( i = 0; i < m_DynamicCombos.Count(); i++ )
			{
				combos *= ( m_DynamicCombos[i].m_nMax - m_DynamicCombos[i].m_nMin + 1 );
			}
			return combos;
		}
		uint64 GetNumStaticCombos( void ) const
		{
			uint64 combos = 1;
			int i;
			for( i = 0; i < m_StaticCombos.Count(); i++ )
			{
				combos *= ( m_StaticCombos[i].m_nMax - m_StaticCombos[i].m_nMin + 1 );
			}
			return combos;
		}
	};
#endif

private:
	void					CreateStaticShaders();
	void					DestroyStaticShaders();

	// The low-level dx call to set the vertex shader state
	void					SetVertexShaderState( HardwareShader_t shader, DataCacheHandle_t hCachedShader = DC_INVALID_HANDLE );

	// The low-level dx call to set the pixel shader state
	void					SetPixelShaderState( HardwareShader_t shader, DataCacheHandle_t hCachedShader = DC_INVALID_HANDLE );

	// Destroys all shaders
	void					DestroyAllShaders();

	// Destroy a particular vertex shader
	void					DestroyVertexShader( VertexShader_t shader );
	// Destroy a particular pixel shader
	void					DestroyPixelShader( PixelShader_t shader );

	bool					LoadAndCreateShaders( ShaderLookupDx11_t &lookup, bool bVertexShader );
	FileHandle_t			OpenFileAndLoadHeader( const char *pFileName, ShaderHeader_t *pHeader );

#ifdef DYNAMIC_SHADER_COMPILE
	bool					LoadAndCreateShaders_Dynamic( ShaderLookupDx11_t &lookup, bool bVertexShader );
	const ShaderCombos_t	*FindOrCreateShaderCombos( const char *pShaderName );
	HardwareShader_t		CompileShader( const char *pShaderName, ShaderIndex_t nStaticIndex, ShaderIndex_t nDynamicIndex, bool bVertexShader );
#endif

	void					DisassembleShader( ShaderLookupDx11_t *pLookup, ShaderIndex_t dynamicCombo, uint8 *pByteCode );

	CUtlFixedLinkedList< ShaderLookupDx11_t > m_VertexShaderDict;
	CUtlFixedLinkedList< ShaderLookupDx11_t > m_PixelShaderDict;

	CUtlSymbolTable m_ShaderSymbolTable;
	
#ifdef DYNAMIC_SHADER_COMPILE	
	//typedef HRESULT (__stdcall *ShaderCompileFromFileFunc_t)( LPCSTR pSrcFile, CONST D3DXMACRO* pDefines,
	//	LPD3DXINCLUDE pInclude,	LPCSTR pFunctionName, LPCSTR pProfile, DWORD Flags,
	//	LPD3DXBUFFER* ppShader, LPD3DXBUFFER * ppErrorMsgs,	LPD3DXCONSTANTTABLE * ppConstantTable );
	CUtlStringMap<ShaderCombos_t>	 m_ShaderNameToCombos;
#endif
	
	// The current vertex and pixel shader
	HardwareShader_t	m_HardwareVertexShader;
	HardwareShader_t	m_HardwarePixelShader;

	CUtlFixedLinkedList< ShaderFileCache_t > m_ShaderFileCache;

	// false, creates during init.
	// true, creates on access, helps reduce d3d memory for tools, but causes i/o hitches.
	bool m_bCreateShadersOnDemand;

#if defined( _DEBUG )
	// for debugging (can't resolve UtlSym)
	// need some history because 360 d3d has rips related to sequencing
	char	vshDebugName[MAX_SHADER_HISTORY][64];
	int		vshDebugIndex;
	char	pshDebugName[MAX_SHADER_HISTORY][64];
	int		pshDebugIndex;
#endif
};


//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
static CShaderManager s_ShaderManager;
IShaderManager *g_pShaderManager = &s_ShaderManager;
IShaderManager *g_pShaderManagerDx11 = g_pShaderManager;

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CShaderManager::CShaderManager() : 
	m_ShaderSymbolTable( 0, 32, true /* caseInsensitive */ ),
	m_VertexShaderDict( 32 ),
	m_PixelShaderDict( 32 ),
	m_ShaderFileCache( 32 )
{
	m_bCreateShadersOnDemand = false;

#ifdef _DEBUG
	vshDebugIndex = 0;
	pshDebugIndex = 0;
#endif
}

CShaderManager::~CShaderManager()
{
}


//-----------------------------------------------------------------------------
// Initialization, shutdown
//-----------------------------------------------------------------------------
void CShaderManager::Init()
{
	// incomptaible on the 360, violates loading system
	// only used by pc to help tools reduce d3d footprint
	m_bCreateShadersOnDemand = IsPC() && ( ShaderUtil()->InEditorMode() || CommandLine()->CheckParm( "-shadersondemand" ) );

	CreateStaticShaders();
}

void CShaderManager::Shutdown()
{

	DestroyAllShaders();
	DestroyStaticShaders();
}


//-----------------------------------------------------------------------------
// Compiles shaders
//-----------------------------------------------------------------------------
IShaderBuffer *CShaderManager::CompileShader( const char *pProgram, size_t nBufLen, const char *pShaderVersion )
{
	return g_pShaderDeviceDx11->CompileShader( pProgram, nBufLen, pShaderVersion );
}


VertexShaderHandle_t CShaderManager::CreateVertexShader( IShaderBuffer* pShaderBuffer )
{
	return g_pShaderDeviceDx11->CreateVertexShader( pShaderBuffer );
}

void CShaderManager::DestroyVertexShader( VertexShaderHandle_t hShader )
{
	g_pShaderDeviceDx11->DestroyVertexShader( hShader );
}

PixelShaderHandle_t CShaderManager::CreatePixelShader( IShaderBuffer* pShaderBuffer )
{
	return g_pShaderDeviceDx11->CreatePixelShader( pShaderBuffer );
}

void CShaderManager::DestroyPixelShader( PixelShaderHandle_t hShader )
{
	g_pShaderDeviceDx11->DestroyPixelShader( hShader );
}

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
HardwareShader_t s_pIllegalMaterialPS = INVALID_HARDWARE_SHADER;

//-----------------------------------------------------------------------------
// Static methods
//-----------------------------------------------------------------------------
void CShaderManager::CreateStaticShaders()
{
	MEM_ALLOC_D3D_CREDIT();

	if ( !HardwareConfig()->SupportsVertexAndPixelShaders() )
	{
		return;
	}

#if 0
	if ( IsPC() )
	{
		// GR - hack for illegal materials
		const DWORD psIllegalMaterial[] =
		{
			0xffff0101, 0x00000051, 0xa00f0000, 0x00000000, 0x3f800000, 0x00000000, 
			0x3f800000, 0x00000001, 0x800f0000, 0xa0e40000, 0x0000ffff
		};
		// create default shader
		Dx9Device()->CreatePixelShader( psIllegalMaterial, ( IDirect3DPixelShader9 ** )&s_pIllegalMaterialPS );
	}
#endif
}

void CShaderManager::DestroyStaticShaders()
{
#if 0
	// GR - invalid material hack
	// destroy internal shader
	if ( s_pIllegalMaterialPS != INVALID_HARDWARE_SHADER )
	{
		( ( IDirect3DPixelShader9 * )s_pIllegalMaterialPS )->Release();
		s_pIllegalMaterialPS = INVALID_HARDWARE_SHADER;
	}
#endif
}

#ifdef DYNAMIC_SHADER_COMPILE
static const char *GetShaderSourcePath( void )
{
	static char shaderDir[MAX_PATH];
	// GR - just in case init this...
	static bool bHaveShaderDir = false;
	if( !bHaveShaderDir )
	{
		bHaveShaderDir = true;
#		if ( defined( DYNAMIC_SHADER_COMPILE_CUSTOM_PATH ) )
		{
			Q_strncpy( shaderDir, DYNAMIC_SHADER_COMPILE_CUSTOM_PATH, MAX_PATH );
		}
#		else
		{
#			if ( defined( _X360 ) )
			{
				char hostName[128] = "";
				const char *pHostName = CommandLine()->ParmValue( "-host" );
				if ( !pHostName )
				{
					// the 360 machine name must be <HostPC>_360
					DWORD length = sizeof( hostName );
					DmGetXboxName( hostName, &length );
					char *p = strstr( hostName, "_360" );
					*p = '\0';
					pHostName = hostName;
				}

				Q_snprintf( shaderDir, MAX_PATH, "net:\\smb\\%s\\stdshaders", pHostName );
			}
#			else
			{
				Q_strncpy( shaderDir, __FILE__, MAX_PATH );
				Q_StripFilename( shaderDir );
				Q_StripLastDir( shaderDir, MAX_PATH );
				Q_strncat( shaderDir, "stdshadersdx11", MAX_PATH, COPY_ALL_CHARACTERS );
			}
#			endif
		}
#		endif
	}
	return shaderDir;
}
#endif

#ifdef DYNAMIC_SHADER_COMPILE
const CShaderManager::ShaderCombos_t *CShaderManager::FindOrCreateShaderCombos( const char *pShaderName )
{
	if( m_ShaderNameToCombos.Defined( pShaderName ) )
	{
		return &m_ShaderNameToCombos[pShaderName];
	}
	ShaderCombos_t &combos = m_ShaderNameToCombos[pShaderName];
	char filename[MAX_PATH];
	// try the vsh dir first.
	Q_strncpy( filename, GetShaderSourcePath(), MAX_PATH );
	Q_strncat( filename, "\\", MAX_PATH, COPY_ALL_CHARACTERS );
	Q_strncat( filename, pShaderName, MAX_PATH, COPY_ALL_CHARACTERS );
	Q_strncat( filename, ".vsh", MAX_PATH, COPY_ALL_CHARACTERS );
	CUtlInplaceBuffer bffr( 0, 0, CUtlInplaceBuffer::TEXT_BUFFER );
	if ( bool bOpenResult = g_pFullFileSystem->ReadFile( filename, NULL, bffr ) )
	{
		NULL;
	}
	else
	{
		// try the fxc dir.
		Q_strncpy( filename, GetShaderSourcePath(), MAX_PATH );
		Q_strncat( filename, "\\", MAX_PATH, COPY_ALL_CHARACTERS );
		Q_strncat( filename, pShaderName, MAX_PATH, COPY_ALL_CHARACTERS );
		Q_strncat( filename, ".fxc", MAX_PATH, COPY_ALL_CHARACTERS );
		bOpenResult = g_pFullFileSystem->ReadFile( filename, NULL, bffr );

		if ( !bOpenResult )
		{
			// Maybe this is a specific version [20 & 20b] -> [2x]
			if ( Q_strlen( pShaderName ) >= 3 )
			{
				char *pszEndFilename = filename + strlen( filename );
				if ( !Q_stricmp( pszEndFilename - 6, "30.fxc" ) )
				{
					// Total hack. Who knows what builds that 30 shader?
					strcpy( pszEndFilename - 6, "20b.fxc" );
					bOpenResult = g_pFullFileSystem->ReadFile( filename, NULL, bffr );
					if ( !bOpenResult )
					{
						strcpy( pszEndFilename - 6, "2x.fxc" );
						bOpenResult = g_pFullFileSystem->ReadFile( filename, NULL, bffr );
					}
					if ( !bOpenResult )
					{
						strcpy( pszEndFilename - 6, "20.fxc" );
						bOpenResult = g_pFullFileSystem->ReadFile( filename, NULL, bffr );
					}
				}
				else
				{
					if ( !stricmp( pszEndFilename - 6, "20.fxc" ) )
					{
						pszEndFilename[ -5 ] = 'x';
					}
					else if ( !stricmp( pszEndFilename - 7, "20b.fxc" ) )
					{
						strcpy( pszEndFilename - 7, "2x.fxc" );
						--pszEndFilename;
					}
					else if ( !stricmp( pszEndFilename - 6, "11.fxc" ) )
					{
						strcpy( pszEndFilename - 6, "xx.fxc" );
					}

					bOpenResult = g_pFullFileSystem->ReadFile( filename, NULL, bffr );
					if ( !bOpenResult )
					{
						if ( !stricmp( pszEndFilename - 6, "2x.fxc" ) )
						{
							pszEndFilename[ -6 ] = 'x';
							bOpenResult = g_pFullFileSystem->ReadFile( filename, NULL, bffr );
						}
					}
				}
			}
		}

		if ( !bOpenResult )
		{
			Assert( 0 );
			return NULL;
		}
	}

	while( char *line = bffr.InplaceGetLinePtr() )
	{
		// dear god perl is better at this kind of shit!
		int begin = 0;
		int end = 0;

		// check if the line starts with '//'
		if( line[0] != '/' || line[1] != '/' )
		{
			continue;
		}

		// Check if line intended for platform lines
		if( IsX360() )
		{
			if ( Q_stristr( line, "[PC]" ) )
				continue;
		}
		else
		{
			if ( Q_stristr( line, "[360]" ) || Q_stristr( line, "[XBOX]" ) )
				continue;
		}

		// Skip any lines intended for other shader version
		if ( Q_stristr( pShaderName, "_ps20" ) && !Q_stristr( pShaderName, "_ps20b" ) &&
			 Q_stristr( line, "[ps" ) && !Q_stristr( line, "[ps20]" ) )
			 continue;
		if ( Q_stristr( pShaderName, "_ps20b" ) &&
			 Q_stristr( line, "[ps" ) && !Q_stristr( line, "[ps20b]" ) )
			 continue;
		if ( Q_stristr( pShaderName, "_ps30" ) &&
			Q_stristr( line, "[ps" ) &&	 !Q_stristr( line, "[ps30]" ) )
			continue;
		if ( Q_stristr( pShaderName, "_ps40" ) &&
		     Q_stristr( line, "[ps" ) && !Q_stristr( line, "[ps40]" ) )
			continue;
		if ( Q_stristr( pShaderName, "_vs20" ) &&
			Q_stristr( line, "[vs" ) &&	 !Q_stristr( line, "[vs20]" ) )
			continue;
		if ( Q_stristr( pShaderName, "_vs30" ) &&
			Q_stristr( line, "[vs" ) &&	 !Q_stristr( line, "[vs30]" ) )
			continue;
		if ( Q_stristr( pShaderName, "_vs40" ) &&
		     Q_stristr( line, "[vs" ) && !Q_stristr( line, "[vs40]" ) )
			continue;

		char *pScan = &line[2];
		while( *pScan == ' ' || *pScan == '\t' )
		{
			pScan++;
		}

		bool bDynamic;
		if( Q_strncmp( pScan, "DYNAMIC", 7 ) == 0 )
		{
			bDynamic = true;
			pScan += 7;
		}
		else if( Q_strncmp( pScan, "STATIC", 6 ) == 0 )
		{
			bDynamic = false;
			pScan += 6;
		}
		else
		{
			continue;
		}

		// skip whitespace
		while( *pScan == ' ' || *pScan == '\t' )
		{
			pScan++;
		}

		// check for colon
		if( *pScan != ':' )
		{
			continue;
		}
		pScan++;

		// skip whitespace
		while( *pScan == ' ' || *pScan == '\t' )
		{
			pScan++;
		}

		// check for quote
		if( *pScan != '\"' )
		{
			continue;
		}
		pScan++;

		char *pBeginningOfName = pScan;
		while( 1 )
		{
			if( *pScan == '\0' )
			{
				break;
			}
			if( *pScan == '\"' )
			{
				break;
			}
			pScan++;
		}

		if( *pScan == '\0' )
		{
			continue;
		}

		// must have hit a quote. .done with string.
		// slam a NULL at the end quote of the string so that we have the string at pBeginningOfName.
		*pScan = '\0';
		pScan++;

		// skip whitespace
		while( *pScan == ' ' || *pScan == '\t' )
		{
			pScan++;
		}

		// check for quote
		if( *pScan != '\"' )
		{
			continue;
		}
		pScan++;

		// make sure that we have a number after the quote.
		if( !isdigit( *pScan ) )
		{
			continue;
		}

		while( isdigit( *pScan ) )
		{
			begin = begin * 10 + ( *pScan - '0' );
			pScan++;
		}

		if( pScan[0] != '.' || pScan[1] != '.' )
		{
			continue;
		}
		pScan += 2;

		// make sure that we have a number
		if( !isdigit( *pScan ) )
		{
			continue;
		}

		while( isdigit( *pScan ) )
		{
			end = end * 10 + ( *pScan - '0' );
			pScan++;
		}

		if( pScan[0] != '\"' )
		{
			continue;
		}

		// sweet freaking jesus. .done parsing the line.
//		char buf[1024];
//		sprintf( buf, "\"%s\" \"%s\" %d %d\n", bDynamic ? "DYNAMIC" : "STATIC", pBeginningOfName, begin, end );
//		Plat_DebugString( buf );

		Combo_t *pCombo = NULL;
		if( bDynamic )
		{
			pCombo = &combos.m_DynamicCombos[combos.m_DynamicCombos.AddToTail()];
		}
		else
		{
			pCombo = &combos.m_StaticCombos[combos.m_StaticCombos.AddToTail()];
		}

		pCombo->m_ComboName = m_ShaderSymbolTable.AddString( pBeginningOfName );
		pCombo->m_nMin = begin;
		pCombo->m_nMax = end;
	}
	
	return &combos;
}
#endif // DYNAMIC_SHADER_COMPILE

#ifdef DYNAMIC_SHADER_COMPILE
//-----------------------------------------------------------------------------
// Used to deal with include files
//-----------------------------------------------------------------------------
class CDxInclude : public ID3DInclude
{
public:
	CDxInclude( const char *pMainFileName );

	virtual HRESULT WINAPI Open( D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID * ppData, UINT * pBytes );

	virtual HRESULT WINAPI Close( LPCVOID pData );

private:
	char m_pBasePath[MAX_PATH];
	
#if defined( _X360 )
	char m_pFullPath[MAX_PATH];
#endif
};

CDxInclude::CDxInclude( const char *pMainFileName )
{
	Q_ExtractFilePath( pMainFileName, m_pBasePath, sizeof(m_pBasePath) );
}


HRESULT CDxInclude::Open( D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID * ppData, UINT * pBytes )
{
	char pTemp[MAX_PATH];
	if ( !Q_IsAbsolutePath( pFileName ) && ( IncludeType == D3D_INCLUDE_LOCAL ) )
	{
		Q_ComposeFileName( m_pBasePath, pFileName, pTemp, sizeof(pTemp) );
		pFileName = pTemp;
	}

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	if ( !g_pFullFileSystem->ReadFile( pFileName, NULL, buf ) )
		return E_FAIL;

	*pBytes = buf.TellMaxPut();
	void *pMem = malloc( *pBytes );
	memcpy( pMem, buf.Base(), *pBytes );
	*ppData = pMem;

#	if ( defined( _X360 ) )
	{
		Q_ComposeFileName( m_pBasePath, pFileName, m_pFullPath, sizeof(m_pFullPath) );
		pFullPath = m_pFullPath;
		cbFullPath = MAX_PATH;
	}
#	endif

	return S_OK;
}

HRESULT CDxInclude::Close( LPCVOID pData )
{
	void *pMem = const_cast<void*>( pData );
	free( pMem );
	return S_OK;
}

static const char *FileNameToShaderModel( const char *pShaderName, bool bVertexShader )
{
	// Figure out the shader model
	const char *pShaderModel = NULL;
	if( bVertexShader )
	{
		if ( Q_stristr( pShaderName, "vs40" ) )
		{
			pShaderModel = "vs_4_0";
			bVertexShader = true;
		}
		else if( Q_stristr( pShaderName, "vs20" ) )
		{
			pShaderModel = "vs_2_0";
			bVertexShader = true;
		}
		else if( Q_stristr( pShaderName, "vs11" ) )
		{
			pShaderModel = "vs_1_1";
			bVertexShader = true;
		}
		else if( Q_stristr( pShaderName, "vs14" ) )
		{
			pShaderModel = "vs_1_1";
			bVertexShader = true;
		}
		else if( Q_stristr( pShaderName, "vs30" ) )
		{
			pShaderModel = "vs_3_0";
			bVertexShader = true;
		}
		else
		{
#ifdef _DEBUG
			Error( "Failed dynamic shader compiled\nBuild shaderapidx9.dll in debug to find problem\n" );
#else
			Assert( 0 );
#endif
		}
	}
	else
	{
		if ( Q_stristr( pShaderName, "ps40" ) )
		{
			pShaderModel = "ps_4_0";
		}
		else if( Q_stristr( pShaderName, "ps20b" ) )
		{
			pShaderModel = "ps_2_b";
		}
		else if( Q_stristr( pShaderName, "ps20" ) )
		{
			pShaderModel = "ps_2_0";
		}
		else if( Q_stristr( pShaderName, "ps11" ) )
		{
			pShaderModel = "ps_1_1";
		}
		else if( Q_stristr( pShaderName, "ps14" ) )
		{
			pShaderModel = "ps_1_4";
		}
		else if( Q_stristr( pShaderName, "ps30" ) )
		{
			pShaderModel = "ps_3_0";
		}
		else
		{
#ifdef _DEBUG
			Error( "Failed dynamic shader compiled\nBuild shaderapidx9.dll in debug to find problem\n" );
#else
			Assert( 0 );
#endif
		}
	}
	return pShaderModel;
}
#endif

#ifdef DYNAMIC_SHADER_COMPILE

#if defined( _X360 )
static ConVar mat_flushshaders_generate_updbs( "mat_flushshaders_generate_updbs", "0", 0, "Generates UPDBs whenever you flush shaders." );
#endif

HardwareShader_t CShaderManager::CompileShader( const char *pShaderName, 
						ShaderIndex_t nStaticIndex, ShaderIndex_t nDynamicIndex, bool bVertexShader )
{
	VPROF_BUDGET( "CompileShader", "CompileShader" );
	Assert( m_ShaderNameToCombos.Defined( pShaderName ) );
	if( !m_ShaderNameToCombos.Defined( pShaderName ) )
	{
		return INVALID_HARDWARE_SHADER;
	}
	const ShaderCombos_t &combos = m_ShaderNameToCombos[pShaderName];
#ifdef _DEBUG
	uint64 numStaticCombos = combos.GetNumStaticCombos();
	uint64 numDynamicCombos = combos.GetNumDynamicCombos();
#endif
	Assert( nStaticIndex % numDynamicCombos == 0 );
	Assert( ( nStaticIndex % numDynamicCombos ) >= 0 && ( nStaticIndex % numDynamicCombos ) < numStaticCombos );
	Assert( nDynamicIndex >= 0 && nDynamicIndex < numDynamicCombos );

#	ifdef DYNAMIC_SHADER_COMPILE_VERBOSE

	//Warning( "Compiling %s %s\n\tdynamic:", bVertexShader ? "vsh" : "psh", pShaderName );
	Warning( "Compiling " );
	if ( bVertexShader )
		ConColorMsg( Color( 0, 255, 0, 255 ), "vsh - %s ", pShaderName );
	else
		ConColorMsg( Color( 0, 255, 255, 255 ), "psh - %s ", pShaderName );
	Warning( "\n\tdynamic:" );

#	endif

	CUtlVector<D3D_SHADER_MACRO> macros;
	// plus 1 for null termination, plus 1 for #define SHADER_MODEL_*, and plus 1 for #define _X360 on 360
	macros.SetCount( combos.m_DynamicCombos.Count() + combos.m_StaticCombos.Count() + 2 + ( IsX360() ? 1 : 0 ) );

	uint64 nCombo = nStaticIndex + nDynamicIndex;
	int macroIndex = 0;
	int i;
	for( i = 0; i < combos.m_DynamicCombos.Count(); i++ )
	{
		int countForCombo = combos.m_DynamicCombos[i].m_nMax - combos.m_DynamicCombos[i].m_nMin + 1;
		int val = nCombo % countForCombo + combos.m_DynamicCombos[i].m_nMin;
		nCombo /= countForCombo;
		macros[macroIndex].Name = m_ShaderSymbolTable.String( combos.m_DynamicCombos[i].m_ComboName );
		char buf[16];
		sprintf( buf, "%d", val );
		CUtlSymbol valSymbol( buf );
		macros[macroIndex].Definition = valSymbol.String();
#	ifdef DYNAMIC_SHADER_COMPILE_VERBOSE
		Warning( " %s=%s", macros[macroIndex].Name, macros[macroIndex].Definition );
#	endif
		macroIndex++;
	}

#	ifdef DYNAMIC_SHADER_COMPILE_VERBOSE
	Warning( "\n\tstatic:" );
#	endif
	for( i = 0; i < combos.m_StaticCombos.Count(); i++ )
	{
		int countForCombo = combos.m_StaticCombos[i].m_nMax - combos.m_StaticCombos[i].m_nMin + 1;
		int val = nCombo % countForCombo + combos.m_StaticCombos[i].m_nMin;
		nCombo /= countForCombo;
		macros[macroIndex].Name = m_ShaderSymbolTable.String( combos.m_StaticCombos[i].m_ComboName );
		char buf[16];
		sprintf( buf, "%d", val );
		CUtlSymbol valSymbol( buf );
		macros[macroIndex].Definition = valSymbol.String();
#	ifdef DYNAMIC_SHADER_COMPILE_VERBOSE
		Warning( " %s=%s", macros[macroIndex].Name, macros[macroIndex].Definition );
#	endif
		macroIndex++;
	}

#	ifdef DYNAMIC_SHADER_COMPILE_VERBOSE
	Warning( "\n" );
#	endif

	char filename[MAX_PATH];
	Q_strncpy( filename, GetShaderSourcePath(), MAX_PATH );
	Q_strncat( filename, "\\", MAX_PATH, COPY_ALL_CHARACTERS );
	Q_strncat( filename, pShaderName, MAX_PATH, COPY_ALL_CHARACTERS );
	Q_strncat( filename, ".fxc", MAX_PATH, COPY_ALL_CHARACTERS );
	
	const char *pShaderModel = FileNameToShaderModel( pShaderName, bVertexShader );
	
	// define the shader model
	char shaderModelDefineString[1024];
	Q_snprintf( shaderModelDefineString, 1024, "SHADER_MODEL_%s", pShaderModel );
	Q_strupr( shaderModelDefineString );
	macros[macroIndex].Name = shaderModelDefineString;
	macros[macroIndex].Definition = "1";
	macroIndex++;

	char x360DefineString[1024];
	if( IsX360() )
	{
		Q_snprintf( x360DefineString, 1024, "_X360", pShaderModel );
		Q_strupr( x360DefineString );
		macros[macroIndex].Name = x360DefineString;
		macros[macroIndex].Definition = "1";
		macroIndex++;
	}

	// NULL terminate.
	macros[macroIndex].Name = NULL;
	macros[macroIndex].Definition = NULL;

	// Instead of erroring out, infinite-loop on shader compilation
	// (i.e. give developers a chance to fix the shader code w/out restarting the game)
#ifndef _DEBUG
	int retriesLeft = 20;
retry_compile:
#endif

	// Try and open the file to see if it exists
	FileHandle_t fp = g_pFullFileSystem->Open( filename, "r" );

	if ( fp == FILESYSTEM_INVALID_HANDLE )
	{
		// Maybe this is a specific version [20 & 20b] -> [2x]
		if ( strlen( pShaderName ) >= 3 )
		{
			char *pszEndFilename = filename + strlen( filename );
			if ( !Q_stricmp( pszEndFilename - 6, "30.fxc" ) )
			{
				strcpy( pszEndFilename - 6, "20b.fxc" );
				fp = g_pFullFileSystem->Open( filename, "r" );
				if ( fp == FILESYSTEM_INVALID_HANDLE )
				{
					strcpy( pszEndFilename - 6, "2x.fxc" );
					fp = g_pFullFileSystem->Open( filename, "r" );
				}
				if ( fp == FILESYSTEM_INVALID_HANDLE )
				{
					strcpy( pszEndFilename - 6, "20.fxc" );
					fp = g_pFullFileSystem->Open( filename, "r" );
				}
			}
			else
			{
				if ( !Q_stricmp( pszEndFilename - 6, "20.fxc" ) )
				{
					pszEndFilename[ -5 ] = 'x';
					fp = g_pFullFileSystem->Open( filename, "r" );
				}
				else if ( !Q_stricmp( pszEndFilename - 7, "20b.fxc" ) )
				{
					strcpy( pszEndFilename - 7, "2x.fxc" );
					fp = g_pFullFileSystem->Open( filename, "r" );
				}
				else if ( !stricmp( pszEndFilename - 6, "11.fxc" ) )
				{
					strcpy( pszEndFilename - 6, "xx.fxc" );
					fp = g_pFullFileSystem->Open( filename, "r" );
				}

				if ( fp == FILESYSTEM_INVALID_HANDLE )
				{
					if ( !stricmp( pszEndFilename - 6, "2x.fxc" ) )
					{
						pszEndFilename[ -6 ] = 'x';
						fp = g_pFullFileSystem->Open( filename, "r" );
					}
				}
			}
		}
	}

	if ( fp != FILESYSTEM_INVALID_HANDLE )
	{
		g_pFullFileSystem->Close( fp );
	}

	wchar_t wfilename[MAX_PATH];
	memset( wfilename, 0, MAX_PATH );
	//mbtowc( wfilename, filename, strlen( filename ) );

	int nCompileFlags = D3DCOMPILE_AVOID_FLOW_CONTROL;
	nCompileFlags |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;

#ifdef _DEBUG
	nCompileFlags |= D3DCOMPILE_DEBUG;
#endif

	LPD3DBLOB pShader;
	LPD3DBLOB pErrorMessages;
	HRESULT hr;
	CDxInclude dxInclude( filename );
	// Open the top-level file via our include interface
	LPCVOID lpcvData;
	UINT numBytes;
	hr = dxInclude.Open( (D3D_INCLUDE_TYPE)0, filename, NULL, &lpcvData, &numBytes );

	LPCSTR pShaderData = (LPCSTR)lpcvData;

	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, filename, -1, wfilename, MAX_PATH );
	hr = D3DCompile( pShaderData, numBytes, filename, macros.Base(), &dxInclude, "main", pShaderModel, nCompileFlags, 0, &pShader, &pErrorMessages );

	dxInclude.Close( lpcvData );

	if ( hr != S_OK && pErrorMessages )
	{
		Warning( "Failed to compile shader %s:\n", filename );
		const char *pErrorMessageString = ( const char * )pErrorMessages->GetBufferPointer();
		Warning( pErrorMessageString );
		Warning( "\n" );

#if 0//#ifndef _DEBUG
		if ( retriesLeft-- > 0 )
		{
			DevMsg( 0, "Failed dynamic shader compiled - fix the shader while the debugger is at the breakpoint, then continue\n" );
			DebuggerBreakIfDebugging();
			goto retry_compile;
		}
		if( !IsX360() ) //errors make the 360 puke and die. We have a better solution for this particular error
			Error( "Failed dynamic shader compile\nBuild shaderapidx9.dll in debug to find problem\n" );
#else
		Assert( 0 );
#endif

		return INVALID_HARDWARE_SHADER;
	}
	else
	{
#ifdef DYNAMIC_SHADER_COMPILE_WRITE_ASSEMBLY
		// enable to dump the disassembly for shader validation
		char exampleCommandLine[2048];
		Q_strncpy( exampleCommandLine, "// Run from stdshaders\n// ..\\..\\dx9sdk\\utilities\\fxc.exe ", sizeof( exampleCommandLine ) );
		int i;
		for( i = 0; macros[i].Name; i++ )
		{
			Q_strncat( exampleCommandLine, "/D", sizeof( exampleCommandLine ) );
			Q_strncat( exampleCommandLine, macros[i].Name, sizeof( exampleCommandLine ) );
			Q_strncat( exampleCommandLine, "=", sizeof( exampleCommandLine ) );
			Q_strncat( exampleCommandLine, macros[i].Definition, sizeof( exampleCommandLine ) );
			Q_strncat( exampleCommandLine, " ", sizeof( exampleCommandLine ) );
		}

		Q_strncat( exampleCommandLine, "/T", sizeof( exampleCommandLine ) );
		Q_strncat( exampleCommandLine, pShaderModel, sizeof( exampleCommandLine ) );
		Q_strncat( exampleCommandLine, " ", sizeof( exampleCommandLine ) );
		Q_strncat( exampleCommandLine, filename, sizeof( exampleCommandLine ) );
		Q_strncat( exampleCommandLine, "\n", sizeof( exampleCommandLine ) );

		ID3DBlob *pd3dxBuffer;
		HRESULT hr;
		hr = D3DDisassemble( pShader->GetBufferPointer(), pShader->GetBufferSize(), 0, NULL, &pd3dxBuffer );
		Assert( hr == D3D_OK );
		CUtlBuffer tempBuffer;
		tempBuffer.SetBufferType( true, false );
		int exampleCommandLineLength = strlen( exampleCommandLine );
		tempBuffer.EnsureCapacity( pd3dxBuffer->GetBufferSize() + exampleCommandLineLength );
		memcpy( tempBuffer.Base(), exampleCommandLine, exampleCommandLineLength );
		memcpy( ( char * )tempBuffer.Base() + exampleCommandLineLength, pd3dxBuffer->GetBufferPointer(), pd3dxBuffer->GetBufferSize() );
		tempBuffer.SeekPut( CUtlBuffer::SEEK_CURRENT, pd3dxBuffer->GetBufferSize() + exampleCommandLineLength );
		char filename[MAX_PATH];
		sprintf( filename, "%s_%d_%d.asm", pShaderName, nStaticIndex, nDynamicIndex );
		g_pFullFileSystem->WriteFile( filename, "DEFAULT_WRITE_PATH", tempBuffer );
#endif
		if ( bVertexShader )
		{
			return g_pShaderDeviceDx11->CreateVertexShader( pShader->GetBufferPointer(), pShader->GetBufferSize() );
		}
		else
		{
			return g_pShaderDeviceDx11->CreatePixelShader( pShader->GetBufferPointer(), pShader->GetBufferSize() ); // hack hack hack!  need to get centroid info from the source
		}
	}
	if ( pShader )
	{
		pShader->Release();
	}
	if ( pErrorMessages )
	{
		pErrorMessages->Release();
	}
}
#endif

#ifdef DYNAMIC_SHADER_COMPILE	
bool CShaderManager::LoadAndCreateShaders_Dynamic( ShaderLookupDx11_t &lookup, bool bVertexShader )
{	
	const char *pName = m_ShaderSymbolTable.String( lookup.m_Name );
	const ShaderCombos_t *pCombos = FindOrCreateShaderCombos( pName );
	if ( !pCombos )
	{
		return false;
	}

	int numDynamicCombos = pCombos->GetNumDynamicCombos();
	lookup.m_ShaderStaticCombos.m_pHardwareShaders = new HardwareShader_t[numDynamicCombos];
	lookup.m_ShaderStaticCombos.m_nCount = numDynamicCombos;
	lookup.m_ShaderStaticCombos.m_pCreationData = new ShaderStaticCombos_t::ShaderCreationData_t[numDynamicCombos];

	int i;
	for( i = 0; i < numDynamicCombos; i++ )
	{
		lookup.m_ShaderStaticCombos.m_pHardwareShaders[i] = INVALID_HARDWARE_SHADER;
	}
	return true;
}
#endif

//-----------------------------------------------------------------------------
// Open the shader file, optionally gets the header
//-----------------------------------------------------------------------------
FileHandle_t CShaderManager::OpenFileAndLoadHeader( const char *pFileName, ShaderHeader_t *pHeader )
{
	//FileHandle_t fp = g_pFullFileSystem->Open( pFileName, "rb", "GAME" );
	FileHandle_t fp = g_pFullFileSystem->Open( pFileName, "rb", "CORE" );
	if ( fp == FILESYSTEM_INVALID_HANDLE )
	{
		return FILESYSTEM_INVALID_HANDLE;
	}

	if ( pHeader )
	{
		// read the header 
		g_pFullFileSystem->Read( pHeader, sizeof( ShaderHeader_t ), fp );

		switch ( pHeader->m_nVersion )
		{
			case 4:
				// version with combos done as diffs vs a reference combo
				// vsh/psh or older fxc
				break;

			case 5:
			case 6:
				// version with optimal dictionary and compressed combo block
				break;

			default:
				Assert( 0 );
				Warning( "Shader %s is the wrong version %d, expecting %d\n", pFileName, pHeader->m_nVersion, SHADER_VCS_VERSION_NUMBER );
				g_pFullFileSystem->Close( fp );
				return FILESYSTEM_INVALID_HANDLE;
		}
	}

	return fp;
}

//-----------------------------------------------------------------------------
// Disassemble a shader for debugging. Writes .asm files.
//-----------------------------------------------------------------------------
void CShaderManager::DisassembleShader( ShaderLookupDx11_t *pLookup, ShaderIndex_t dynamicCombo, uint8 *pByteCode )
{
#if defined( WRITE_ASSEMBLY )
	const char *pName = m_ShaderSymbolTable.String( pLookup->m_Name );

	ID3DXBuffer *pd3dxBuffer;
	HRESULT hr;
	hr = D3DXDisassembleShader( (DWORD*)pByteCode, false, NULL, &pd3dxBuffer );
	Assert( hr == D3D_OK );

	CUtlBuffer tempBuffer;
	tempBuffer.SetBufferType( true, false );
	tempBuffer.EnsureCapacity( pd3dxBuffer->GetBufferSize() );
	memcpy( ( char * )tempBuffer.Base(), pd3dxBuffer->GetBufferPointer(), pd3dxBuffer->GetBufferSize() );
	tempBuffer.SeekPut( CUtlBuffer::SEEK_CURRENT, pd3dxBuffer->GetBufferSize() );

	char filename[MAX_PATH];
	sprintf( filename, "%s_%d_%d.asm", pName, pLookup->m_nStaticIndex, dynamicCombo );
	g_pFullFileSystem->WriteFile( filename, "DEFAULT_WRITE_PATH", tempBuffer );
#endif
}

//-----------------------------------------------------------------------------
// Create dynamic combos
//-----------------------------------------------------------------------------
bool CShaderManager::CreateDynamicCombos_Ver4( void *pContext, uint8 *pComboBuffer )
{
	ShaderLookupDx11_t* pLookup = (ShaderLookupDx11_t *)pContext;

	ShaderFileCache_t *pFileCache = &m_ShaderFileCache[pLookup->m_hShaderFileCache];
	ShaderHeader_t *pHeader = &pFileCache->m_Header;

	int nReferenceComboSizeForDiffs = ((ShaderHeader_t_v4 *)pHeader)->m_nDiffReferenceSize;

	uint8 *pReferenceShader = NULL;
	uint8 *pDiffOutputBuffer = NULL;
	if ( nReferenceComboSizeForDiffs )
	{
		// reference combo is *always* the largest combo, so safe worst case size for uncompression buffer
		pReferenceShader = (uint8 *)pFileCache->m_ReferenceCombo.Base();
		pDiffOutputBuffer = (uint8 *)_alloca( nReferenceComboSizeForDiffs ); 
	}

	// build this shader's dynamic combos
	bool bOK = true;
	int nStartingOffset = 0;
	for ( int i = 0; i < pHeader->m_nDynamicCombos; i++ )
	{
		if ( pLookup->m_pComboDictionary[i].m_Offset == -1 )
		{
			// skipped
			continue;
		}

		if ( !nStartingOffset )
		{
			nStartingOffset = pLookup->m_pComboDictionary[i].m_Offset;
		}

		// offsets better be sequentially ascending
		Assert( nStartingOffset <= pLookup->m_pComboDictionary[i].m_Offset );

		if ( pLookup->m_pComboDictionary[i].m_Size <= 0 )
		{
			// skipped
			continue;
		}

		// get the right byte code from the monolithic buffer
		uint8 *pByteCode = (uint8 *)pComboBuffer + pLookup->m_nDataOffset + pLookup->m_pComboDictionary[i].m_Offset - nStartingOffset;
		int nByteCodeSize = pLookup->m_pComboDictionary[i].m_Size;

		if ( pReferenceShader )
		{
			// reference combo better be the largest combo, otherwise memory corruption
			Assert( nReferenceComboSizeForDiffs >= nByteCodeSize );

			// use the differencing algorithm to recover the full shader
			int nOriginalSize;
			ApplyDiffs( 
				pReferenceShader, 
				pByteCode,
				nReferenceComboSizeForDiffs,
				nByteCodeSize,
				nOriginalSize,
				pDiffOutputBuffer,
				nReferenceComboSizeForDiffs );

			pByteCode = pDiffOutputBuffer;
			nByteCodeSize = nOriginalSize;
		}

#if defined( WRITE_ASSEMBLY )
		DisassembleShader( pLookup, i, pByteCode );
#endif
		HardwareShader_t hardwareShader = INVALID_HARDWARE_SHADER;

		if ( IsPC() && m_bCreateShadersOnDemand )
		{
			// cache the code off for later
			pLookup->m_ShaderStaticCombos.m_pCreationData[i].ByteCode.SetSize( nByteCodeSize );
			V_memcpy( pLookup->m_ShaderStaticCombos.m_pCreationData[i].ByteCode.Base(), pByteCode, nByteCodeSize );
			pLookup->m_ShaderStaticCombos.m_pCreationData[i].iCentroidMask = pFileCache->m_bVertexShader ? 0 : pHeader->m_nCentroidMask;
		}
		else
		{
			if ( pFileCache->m_bVertexShader )
			{
				hardwareShader = g_pShaderDeviceDx11->CreateVertexShader( reinterpret_cast< DWORD *>( pByteCode ), nByteCodeSize );
			}
			else
			{
				hardwareShader = g_pShaderDeviceDx11->CreatePixelShader( reinterpret_cast< DWORD *>( pByteCode ), nByteCodeSize );
			}
			if ( hardwareShader == INVALID_HARDWARE_SHADER )
			{
				Assert( 0 );
				bOK = false;
				break;
			}
		}
		pLookup->m_ShaderStaticCombos.m_pHardwareShaders[i] = hardwareShader;
	}

	delete [] pLookup->m_pComboDictionary;
	pLookup->m_pComboDictionary = NULL;

	return bOK;
}

//-----------------------------------------------------------------------------
// Create dynamic combos
//-----------------------------------------------------------------------------
static uint32 NextULONG( uint8 * &pData )
{
	// handle unaligned read
	uint32 nRet;
	memcpy( &nRet, pData, sizeof( nRet ) );
	pData += sizeof( nRet );
	return nRet;
}
bool CShaderManager::CreateDynamicCombos_Ver5( void *pContext, uint8 *pComboBuffer )
{
	ShaderLookupDx11_t* pLookup = (ShaderLookupDx11_t *)pContext;
	ShaderFileCache_t *pFileCache = &m_ShaderFileCache[pLookup->m_hShaderFileCache];
	uint8 *pCompressedShaders = pComboBuffer + pLookup->m_nDataOffset;

	uint8 *pUnpackBuffer = new uint8[MAX_SHADER_UNPACKED_BLOCK_SIZE];

	// now, loop through all blocks
	bool bOK = true;
	while ( bOK )
	{
		uint32 nBlockSize = NextULONG( pCompressedShaders );
		if ( nBlockSize == 0xffffffff )	
		{
			// any more blocks?
			break;
		}

		switch( nBlockSize  & 0xc0000000 )
		{
			case 0:											// bzip2
			{
				// uncompress
				uint32 nOutsize = MAX_SHADER_UNPACKED_BLOCK_SIZE;
				int nRslt = BZ2_bzBuffToBuffDecompress( 
					reinterpret_cast<char *>( pUnpackBuffer ),
					&nOutsize,
					reinterpret_cast<char *>( pCompressedShaders ),
					nBlockSize, 1, 0 );
				if ( nRslt < 0 )
				{
					// errors are negative for bzip
					Assert( 0 );
					Warning( "BZIP Error (%d) decompressing shader", nRslt );
					bOK = false;
				}
				
				pCompressedShaders += nBlockSize;
				nBlockSize = nOutsize;		// how much data there is
			}
			break;

			case 0x80000000:								// uncompressed
			{
				// not compressed, as is
				nBlockSize &= 0x3fffffff;
				memcpy( pUnpackBuffer, pCompressedShaders, nBlockSize );
				pCompressedShaders += nBlockSize;
			}
			break;

			case 0x40000000:								// lzma compressed
			{
				CLZMA lzDecoder;
				nBlockSize &= 0x3fffffff;
				
				size_t nOutsize = lzDecoder.Uncompress(
					reinterpret_cast<uint8 *>( pCompressedShaders ),
					pUnpackBuffer );
				pCompressedShaders += nBlockSize;
				nBlockSize = nOutsize;		// how much data there is
			}
			break;
			
			default:
			{
				Assert( 0 );
				Error(" unrecognized shader compression type = file corrupt?");
				bOK = false;
			}
		}
		
		uint8 *pReadPtr = pUnpackBuffer;
		while ( pReadPtr < pUnpackBuffer+nBlockSize )
		{
			uint32 nCombo_ID = NextULONG( pReadPtr );
			uint32 nShaderSize = NextULONG( pReadPtr );
			
#if defined( WRITE_ASSEMBLY )
			DisassembleShader( pLookup, nCombo_ID, pReadPtr );
#endif
			HardwareShader_t hardwareShader = INVALID_HARDWARE_SHADER;

			int iIndex = nCombo_ID;
			if ( iIndex >= pLookup->m_nStaticIndex )
				iIndex -= pLookup->m_nStaticIndex;			// ver5 stores combos as full combo, ver6 as dynamic combo # only
			if ( IsPC() && m_bCreateShadersOnDemand )
			{
				// cache the code off for later
				pLookup->m_ShaderStaticCombos.m_pCreationData[iIndex].ByteCode.SetSize( nShaderSize );
				V_memcpy( pLookup->m_ShaderStaticCombos.m_pCreationData[iIndex].ByteCode.Base(), pReadPtr, nShaderSize );
				pLookup->m_ShaderStaticCombos.m_pCreationData[iIndex].iCentroidMask = pFileCache->m_bVertexShader ? 0 : pFileCache->m_Header.m_nCentroidMask;
			}
			else
			{
				if ( pFileCache->m_bVertexShader )
				{
					hardwareShader = g_pShaderDeviceDx11->CreateVertexShader( reinterpret_cast< DWORD *>( pReadPtr ), nShaderSize );
				}
				else
				{
					hardwareShader = g_pShaderDeviceDx11->CreatePixelShader( reinterpret_cast< DWORD *>( pReadPtr ), nShaderSize );
				}
				if ( hardwareShader == INVALID_HARDWARE_SHADER )
				{
					Warning( "failed to create shader\n" );
					Assert( 0 );
					bOK = false;
					break;
				}
			}
			pLookup->m_ShaderStaticCombos.m_pHardwareShaders[iIndex] = hardwareShader;
			pReadPtr += nShaderSize;
		}
	}

	delete[] pUnpackBuffer;

	return bOK;
}

//-----------------------------------------------------------------------------
// Static method, called by thread, don't call anything non-threadsafe from handler!!!
//-----------------------------------------------------------------------------
void CShaderManager::QueuedLoaderCallback( void *pContext, void *pContext2, const void *pData, int nSize, LoaderError_t loaderError )
{
	ShaderLookupDx11_t* pLookup = (ShaderLookupDx11_t *)pContext;

	bool bOK = ( loaderError == LOADERERROR_NONE );
	if ( bOK )
	{
		if ( pContext2 )
		{
			// presence denotes diff version
			bOK = s_ShaderManager.CreateDynamicCombos_Ver4( pContext, (uint8 *)pData );
		}
		else
		{
			bOK = s_ShaderManager.CreateDynamicCombos_Ver5( pContext, (uint8 *)pData );
		}
	}
	if ( !bOK )
	{
		pLookup->m_Flags |= SHADER_FAILED_LOAD;
	}
}

//-----------------------------------------------------------------------------
// Loads all shaders
//-----------------------------------------------------------------------------
bool CShaderManager::LoadAndCreateShaders( ShaderLookupDx11_t &lookup, bool bVertexShader )
{
#ifdef DYNAMIC_SHADER_COMPILE
	lookup.m_Flags &= ~SHADER_DYNAMIC_COMPILE_IS_HLSL;
#endif
	const char *pName = m_ShaderSymbolTable.String( lookup.m_Name );

	// find it in the cache
	// a cache hit prevents costly i/o for static components, i.e. header, ref combo, etc.
	ShaderFileCache_t fileCacheLookup;
	fileCacheLookup.m_Name = lookup.m_Name;
	fileCacheLookup.m_bVertexShader = bVertexShader;
	int fileCacheIndex = m_ShaderFileCache.Find( fileCacheLookup );
	if ( fileCacheIndex == m_ShaderFileCache.InvalidIndex() )
	{
		// not found, create a new entry
		fileCacheIndex = m_ShaderFileCache.AddToTail();
	}

	lookup.m_hShaderFileCache = fileCacheIndex;

	// fetch from cache
	ShaderFileCache_t *pFileCache = &m_ShaderFileCache[fileCacheIndex];
	ShaderHeader_t *pHeader = &pFileCache->m_Header;

	FileHandle_t hFile = FILESYSTEM_INVALID_HANDLE;
	if ( pFileCache->IsValid() )
	{
		// using cached header, just open file, no read of header needed
		hFile = OpenFileAndLoadHeader( m_ShaderSymbolTable.String( pFileCache->m_Filename ), NULL );
		if ( hFile == FILESYSTEM_INVALID_HANDLE )
		{
			// shouldn't happen
			Assert( 0 );
			return false;
		}
	}
	else
	{
		V_memset( pHeader, 0, sizeof( ShaderHeader_t ) );

		// try the vsh/psh dir first
		char filename[MAX_PATH];
		Q_snprintf( filename, MAX_PATH, "shaders\\%s\\%s" SHADER_FNAME_EXTENSION, bVertexShader ? "vsh" : "psh", pName );
		hFile = OpenFileAndLoadHeader( filename, pHeader );
		if ( hFile == FILESYSTEM_INVALID_HANDLE )
		{
#ifdef DYNAMIC_SHADER_COMPILE
			// Dynamically compile if it's HLSL.
			if ( LoadAndCreateShaders_Dynamic( lookup, bVertexShader ) )
			{
				lookup.m_Flags |= SHADER_DYNAMIC_COMPILE_IS_HLSL;
				return true;
			}
			else
			{
				return false;
			}
#endif
			// next, try the fxc dir
			Q_snprintf( filename, MAX_PATH, "shaders\\fxc\\%s" SHADER_FNAME_EXTENSION, pName );
			hFile = OpenFileAndLoadHeader( filename, pHeader );
			if ( hFile == FILESYSTEM_INVALID_HANDLE )
			{
				lookup.m_Flags |= SHADER_FAILED_LOAD;
				Warning( "Couldn't load %s shader %s\n", bVertexShader ? "vertex" : "pixel", pName );
				return false;
			}
		}

		lookup.m_Flags = pHeader->m_nFlags;

		pFileCache->m_Name = lookup.m_Name;
		pFileCache->m_Filename = m_ShaderSymbolTable.AddString( filename );
		pFileCache->m_bVertexShader = bVertexShader;

		if ( pFileCache->IsOldVersion() )
		{ 
			int referenceComboSize = ((ShaderHeader_t_v4 *)pHeader)->m_nDiffReferenceSize;
			if ( referenceComboSize )
			{
				// cache the reference combo
				pFileCache->m_ReferenceCombo.EnsureCapacity( referenceComboSize );
				g_pFullFileSystem->Read( pFileCache->m_ReferenceCombo.Base(), referenceComboSize, hFile );
			}
		}
		else
		{
			// cache the dictionary
			pFileCache->m_StaticComboRecords.EnsureCount( pHeader->m_nNumStaticCombos );
			g_pFullFileSystem->Read( pFileCache->m_StaticComboRecords.Base(), pHeader->m_nNumStaticCombos * sizeof( StaticComboRecord_t ), hFile );
			if ( pFileCache->IsVersion6() )
			{
				// read static combo alias records
				int nNumDups;
				g_pFullFileSystem->Read( &nNumDups, sizeof( nNumDups ), hFile );
				if ( nNumDups )
				{
					pFileCache->m_StaticComboDupRecords.EnsureCount( nNumDups );
					g_pFullFileSystem->Read( pFileCache->m_StaticComboDupRecords.Base(), nNumDups * sizeof( StaticComboAliasRecord_t ), hFile );
				}
			}

		}
	}

	// FIXME: should make lookup and ShaderStaticCombos_t are pool allocated.
	int i;
	lookup.m_ShaderStaticCombos.m_nCount = pHeader->m_nDynamicCombos;
	lookup.m_ShaderStaticCombos.m_pHardwareShaders = new HardwareShader_t[pHeader->m_nDynamicCombos];
	if ( IsPC() && m_bCreateShadersOnDemand )
	{
		lookup.m_ShaderStaticCombos.m_pCreationData = new ShaderStaticCombos_t::ShaderCreationData_t[pHeader->m_nDynamicCombos];
	}
	for ( i = 0; i < pHeader->m_nDynamicCombos; i++ )
	{
		lookup.m_ShaderStaticCombos.m_pHardwareShaders[i] = INVALID_HARDWARE_SHADER;
	}

	int nStartingOffset = 0;
	int nEndingOffset = 0;

	if ( pFileCache->IsOldVersion() )
	{
		int nDictionaryOffset = sizeof( ShaderHeader_t ) + ((ShaderHeader_t_v4 *)pHeader)->m_nDiffReferenceSize;

		// read in shader's dynamic combos directory
		lookup.m_pComboDictionary = new ShaderDictionaryEntry_t[pHeader->m_nDynamicCombos];
		g_pFullFileSystem->Seek( hFile, nDictionaryOffset + lookup.m_nStaticIndex * sizeof( ShaderDictionaryEntry_t ), FILESYSTEM_SEEK_HEAD );
		g_pFullFileSystem->Read( lookup.m_pComboDictionary, pHeader->m_nDynamicCombos * sizeof( ShaderDictionaryEntry_t ), hFile );

		// want single read of all this shader's dynamic combos into a target buffer
		// shaders are written sequentially, determine starting offset and length
		for ( i = 0; i < pHeader->m_nDynamicCombos; i++ )
		{
			if ( lookup.m_pComboDictionary[i].m_Offset == -1 )
			{
				// skipped
				continue;
			}

			// ensure offsets are in fact sequentially ascending 
			Assert( lookup.m_pComboDictionary[i].m_Offset >= nStartingOffset && lookup.m_pComboDictionary[i].m_Size >= 0 );

			if ( !nStartingOffset )
			{
				nStartingOffset = lookup.m_pComboDictionary[i].m_Offset;
			}
			nEndingOffset = lookup.m_pComboDictionary[i].m_Offset + lookup.m_pComboDictionary[i].m_Size;
		}
		if ( !nStartingOffset )
		{
			g_pFullFileSystem->Close( hFile );
			Warning( "Shader '%s' - All dynamic combos skipped. This is bad!\n", m_ShaderSymbolTable.String( pFileCache->m_Filename ) );
			return false;
		}
	}
	else
	{
		int nStaticComboIdx = pFileCache->FindCombo( lookup.m_nStaticIndex / pFileCache->m_Header.m_nDynamicCombos );
		if ( nStaticComboIdx == -1 )
		{
			g_pFullFileSystem->Close( hFile );
			lookup.m_Flags |= SHADER_FAILED_LOAD;
			Warning( "Shader '%s' - Couldn't load combo %d of shader (dyn=%d)\n", m_ShaderSymbolTable.String( pFileCache->m_Filename ), lookup.m_nStaticIndex, pFileCache->m_Header.m_nDynamicCombos );
			return false;
		}

		nStartingOffset = pFileCache->m_StaticComboRecords[nStaticComboIdx].m_nFileOffset;
		nEndingOffset = pFileCache->m_StaticComboRecords[nStaticComboIdx+1].m_nFileOffset;
	}

	// align offsets for unbuffered optimal i/o - fastest i/o possible
	unsigned nOffsetAlign, nSizeAlign, nBufferAlign;
	g_pFullFileSystem->GetOptimalIOConstraints( hFile, &nOffsetAlign, &nSizeAlign, &nBufferAlign );
	unsigned int nAlignedOffset = AlignValue( ( nStartingOffset - nOffsetAlign ) + 1, nOffsetAlign );
	unsigned int nAlignedBytesToRead = AlignValue( nEndingOffset - nAlignedOffset, nSizeAlign );

	// used for adjusting provided buffer to actual data
	lookup.m_nDataOffset = nStartingOffset - nAlignedOffset;

	bool bOK = true;
	if ( IsX360() && g_pQueuedLoader->IsMapLoading() )
	{
		LoaderJob_t loaderJob;
		loaderJob.m_pFilename = m_ShaderSymbolTable.String( pFileCache->m_Filename );
		loaderJob.m_pPathID = "GAME";
		loaderJob.m_pCallback = QueuedLoaderCallback;
		loaderJob.m_pContext = (void *)&lookup;
		loaderJob.m_pContext2 = (void *)pFileCache->IsOldVersion();
		loaderJob.m_Priority = LOADERPRIORITY_DURINGPRELOAD;
		loaderJob.m_nBytesToRead = nAlignedBytesToRead;
		loaderJob.m_nStartOffset = nAlignedOffset;
		g_pQueuedLoader->AddJob( &loaderJob );
	}
	else
	{
		// single optimal read of all dynamic combos into monolithic buffer
		uint8 *pOptimalBuffer = (uint8 *)g_pFullFileSystem->AllocOptimalReadBuffer( hFile, nAlignedBytesToRead, nAlignedOffset );
		g_pFullFileSystem->Seek( hFile, nAlignedOffset, FILESYSTEM_SEEK_HEAD );
		g_pFullFileSystem->Read( pOptimalBuffer, nAlignedBytesToRead, hFile );

		if ( pFileCache->IsOldVersion() )
		{
			bOK = CreateDynamicCombos_Ver4( &lookup, pOptimalBuffer );
		}
		else
		{
			bOK = CreateDynamicCombos_Ver5( &lookup, pOptimalBuffer );
		}

		g_pFullFileSystem->FreeOptimalReadBuffer( pOptimalBuffer );
	}

	g_pFullFileSystem->Close( hFile );

	if ( !bOK )
	{
		lookup.m_Flags |= SHADER_FAILED_LOAD;
	}

	return bOK;
}

//-----------------------------------------------------------------------------
// Creates and destroys vertex shaders
//-----------------------------------------------------------------------------
VertexShader_t CShaderManager::CreateVertexShader( const char *pFileName, ShaderIndex_t nStaticVshIndex )
{
	MEM_ALLOC_CREDIT();

	if ( !pFileName )
	{
		return INVALID_SHADER;
	}

	VertexShader_t shader;
	ShaderLookupDx11_t lookup;
	lookup.m_Name = m_ShaderSymbolTable.AddString( pFileName );
	lookup.m_nStaticIndex = nStaticVshIndex;
	shader = m_VertexShaderDict.Find( lookup );
	if ( shader == m_VertexShaderDict.InvalidIndex() )
	{
		shader = m_VertexShaderDict.AddToTail( lookup );
		if ( !LoadAndCreateShaders( m_VertexShaderDict[shader], true ) )
		{
			return INVALID_SHADER;
		}
	}
	m_VertexShaderDict[shader].IncRefCount();
	return shader;
}

//-----------------------------------------------------------------------------
// Create pixel shader
//-----------------------------------------------------------------------------
PixelShader_t CShaderManager::CreatePixelShader( const char *pFileName, ShaderIndex_t nStaticPshIndex )
{
	MEM_ALLOC_CREDIT();

	if ( !pFileName )
	{
		return INVALID_SHADER;
	}

	PixelShader_t shader;
	ShaderLookupDx11_t lookup;
	lookup.m_Name = m_ShaderSymbolTable.AddString( pFileName );
	lookup.m_nStaticIndex = nStaticPshIndex;
	shader = m_PixelShaderDict.Find( lookup );
	if ( shader == m_PixelShaderDict.InvalidIndex() )
	{
		shader = m_PixelShaderDict.AddToTail( lookup );
		if ( !LoadAndCreateShaders( m_PixelShaderDict[shader], false ) )
		{
			return INVALID_SHADER;
		}
	}
	m_PixelShaderDict[shader].IncRefCount();
	return shader;
}

//-----------------------------------------------------------------------------
// Clear the refCounts to zero
//-----------------------------------------------------------------------------
void CShaderManager::ClearVertexAndPixelShaderRefCounts()
{
	for ( VertexShader_t vshIndex = m_VertexShaderDict.Head(); 
		 vshIndex != m_VertexShaderDict.InvalidIndex(); 
		 vshIndex = m_VertexShaderDict.Next( vshIndex ) )
	{
		m_VertexShaderDict[vshIndex].m_nRefCount = 0;
	}

	for ( PixelShader_t pshIndex = m_PixelShaderDict.Head(); 
		 pshIndex != m_PixelShaderDict.InvalidIndex(); 
		 pshIndex = m_PixelShaderDict.Next( pshIndex ) )
	{
		m_PixelShaderDict[pshIndex].m_nRefCount = 0;
	}
}

//-----------------------------------------------------------------------------
// Destroy all shaders that have no reference
//-----------------------------------------------------------------------------
void CShaderManager::PurgeUnusedVertexAndPixelShaders()
{
	return; // FIXME

	// iterate vertex shaders
	for ( VertexShader_t vshIndex = m_VertexShaderDict.Head(); vshIndex != m_VertexShaderDict.InvalidIndex(); )
	{
		Assert( m_VertexShaderDict[vshIndex].m_nRefCount >= 0 );

		// Get the next one before we potentially delete the current one.
		VertexShader_t next = m_VertexShaderDict.Next( vshIndex );
		if ( m_VertexShaderDict[vshIndex].m_nRefCount <= 0 )
		{
			DestroyVertexShader( vshIndex );
		}
		vshIndex = next;
	}

	// iterate pixel shaders
	for ( PixelShader_t pshIndex = m_PixelShaderDict.Head(); pshIndex != m_PixelShaderDict.InvalidIndex(); )
	{
		Assert( m_PixelShaderDict[pshIndex].m_nRefCount >= 0 );

		// Get the next one before we potentially delete the current one.
		PixelShader_t next = m_PixelShaderDict.Next( pshIndex );
		if ( m_PixelShaderDict[pshIndex].m_nRefCount <= 0 )
		{
			DestroyPixelShader( pshIndex );
		}
		pshIndex = next;
	}
}



void* CShaderManager::GetCurrentVertexShader()
{
	return (void*)m_HardwareVertexShader;
}

void* CShaderManager::GetCurrentPixelShader()
{
	return (void*)m_HardwarePixelShader;
}


//-----------------------------------------------------------------------------
// The low-level dx call to set the vertex shader state
//-----------------------------------------------------------------------------
void CShaderManager::SetVertexShaderState( HardwareShader_t shader, DataCacheHandle_t hCachedShader )
{
}

void CShaderManager::BindVertexShader( VertexShaderHandle_t hVertexShader )
{
	//if ( m_HardwareVertexShader != hVertexShader )
	//{
		g_pShaderAPIDx11->BindVertexShader( hVertexShader );
		//m_HardwareVertexShader = hVertexShader;
	//}
}


//-----------------------------------------------------------------------------
// Sets a particular vertex shader as the current shader
//-----------------------------------------------------------------------------
void CShaderManager::SetVertexShader( VertexShader_t shader )
{
	// Determine which vertex shader to use...
	if ( shader == INVALID_SHADER )
	{
		BindVertexShader( 0 );
		return;
	}

	int vshIndex = m_nVertexShaderIndex;
	Assert( vshIndex >= 0 );
	if( vshIndex < 0 )
	{
		vshIndex = 0;
	}

	//ShaderLookupDx11_t &tlookup = m_VertexShaderDict[shader];
	//Warning( "vsh: %s static: %d dynamic: %d\n", m_ShaderSymbolTable.String( tlookup.m_Name ),
	//	 tlookup.m_nStaticIndex, m_nVertexShaderIndex );

#ifdef DYNAMIC_SHADER_COMPILE
	HardwareShader_t &dxshader = m_VertexShaderDict[shader].m_ShaderStaticCombos.m_pHardwareShaders[vshIndex];
	if ( dxshader == INVALID_HARDWARE_SHADER )
	{
		// compile it since we haven't already!
		ShaderLookupDx11_t &vshLookup = m_VertexShaderDict[shader];
		dxshader = CompileShader( m_ShaderSymbolTable.String( vshLookup.m_Name ), 
			vshLookup.m_nStaticIndex, vshIndex, true );
		Assert( dxshader != INVALID_HARDWARE_SHADER );
	}
#else
	CShaderManager::ShaderLookupDx11_t *lookup = (ShaderLookupDx11_t *)&m_VertexShaderDict[shader];
	if ( lookup->m_Flags & SHADER_FAILED_LOAD )
	{
		Assert( 0 );
		return;
	}
#ifdef _DEBUG
	vshDebugIndex = (vshDebugIndex + 1) % MAX_SHADER_HISTORY;
	Q_strncpy( vshDebugName[vshDebugIndex], m_ShaderSymbolTable.String( lookup->m_Name ), sizeof( vshDebugName[0] ) );
#endif
	HardwareShader_t dxshader = lookup->m_ShaderStaticCombos.m_pHardwareShaders[vshIndex];
#endif

	if ( IsPC() && ( dxshader == INVALID_HARDWARE_SHADER ) && m_bCreateShadersOnDemand )
	{
#ifdef DYNAMIC_SHADER_COMPILE
		ShaderStaticCombos_t::ShaderCreationData_t *pCreationData = &m_VertexShaderDict[shader].m_ShaderStaticCombos.m_pCreationData[vshIndex];
#else
		ShaderStaticCombos_t::ShaderCreationData_t *pCreationData = &lookup->m_ShaderStaticCombos.m_pCreationData[vshIndex];
#endif

		dxshader = g_pShaderDeviceDx11->CreateVertexShader( ( DWORD * )pCreationData->ByteCode.Base(), pCreationData->ByteCode.Count() );

#ifdef DYNAMIC_SHADER_COMPILE 
		// copy the compiled shader handle back to wherever it's supposed to be stored
		m_VertexShaderDict[shader].m_ShaderStaticCombos.m_pHardwareShaders[vshIndex] = dxshader;
#else
		lookup->m_ShaderStaticCombos.m_pHardwareShaders[vshIndex] = dxshader;
#endif
	}

	Assert( dxshader );

#ifndef DYNAMIC_SHADER_COMPILE
	if( !dxshader )
	{
		Error( "!!!!!Using invalid shader combo!!!!!  Consult a programmer and tell them to build debug materialsystem.dll and stdshader*.dll.  Run with \"mat_bufferprimitives 0\" and look for CMaterial in the call stack and see what m_pDebugName is.  You are likely using a shader combo that has been skipped.\n" );
	}
#endif

	BindVertexShader( (VertexShaderHandle_t)dxshader );
}

//-----------------------------------------------------------------------------
// The low-level dx call to set the pixel shader state
//-----------------------------------------------------------------------------
void CShaderManager::SetPixelShaderState( HardwareShader_t shader, DataCacheHandle_t hCachedShader )
{
}

void CShaderManager::BindPixelShader( PixelShaderHandle_t hPixelShader )
{
	//if ( m_HardwarePixelShader != hPixelShader )
	//{
		g_pShaderAPIDx11->BindPixelShader( hPixelShader );
	//	m_HardwarePixelShader = hPixelShader;
	//}
}


//-----------------------------------------------------------------------------
// Sets a particular pixel shader as the current shader
//-----------------------------------------------------------------------------
void CShaderManager::SetPixelShader( PixelShader_t shader )
{
	if ( shader == INVALID_SHADER )
	{
		BindPixelShader( 0 );
		return;
	}

	int pshIndex = m_nPixelShaderIndex;
	Assert( pshIndex >= 0 );
	//ShaderLookupDx11_t &lookup = m_PixelShaderDict[shader];
	//Warning( "psh: %s static: %d dynamic: %d\n", m_ShaderSymbolTable.String( lookup.m_Name ),
	//	lookup.m_nStaticIndex, m_nPixelShaderIndex );
#ifdef DYNAMIC_SHADER_COMPILE
	HardwareShader_t &dxshader = m_PixelShaderDict[shader].m_ShaderStaticCombos.m_pHardwareShaders[pshIndex];
	if ( dxshader == INVALID_HARDWARE_SHADER )
	{
		// compile it since we haven't already!
		ShaderLookupDx11_t &vshLookup = m_PixelShaderDict[shader];
		dxshader = CompileShader( m_ShaderSymbolTable.String( vshLookup.m_Name ), 
			vshLookup.m_nStaticIndex, pshIndex, false );
//		Assert( dxshader != INVALID_HARDWARE_SHADER );

		if( IsX360() )
		{
			//360 does not respond well at all to bad shaders or Error() calls. So we're staying here until we get something that compiles
			while( dxshader == INVALID_HARDWARE_SHADER )
			{
				Warning( "A dynamically compiled pixel shader has failed to build. Pausing for 5 seconds and attempting rebuild.\n" );
				Sleep( 5000 );
				dxshader = CompileShader( m_ShaderSymbolTable.String( vshLookup.m_Name ), 
					vshLookup.m_nStaticIndex, pshIndex, false );
			}
		}
	}
#else
	ShaderLookupDx11_t *lookup = (ShaderLookupDx11_t *)&m_PixelShaderDict[shader];
	if ( lookup->m_Flags & SHADER_FAILED_LOAD )
	{
		Assert( 0 );
		return;
	}
#ifdef _DEBUG
	pshDebugIndex = (pshDebugIndex + 1) % MAX_SHADER_HISTORY;
	Q_strncpy( pshDebugName[pshDebugIndex], m_ShaderSymbolTable.String( lookup->m_Name ), sizeof( pshDebugName[0] ) );
#endif
	HardwareShader_t dxshader = lookup->m_ShaderStaticCombos.m_pHardwareShaders[pshIndex];
#endif

	if ( IsPC() && ( dxshader == INVALID_HARDWARE_SHADER ) && m_bCreateShadersOnDemand )
	{
#ifdef DYNAMIC_SHADER_COMPILE
		ShaderStaticCombos_t::ShaderCreationData_t *pCreationData = &m_PixelShaderDict[shader].m_ShaderStaticCombos.m_pCreationData[pshIndex];
#else
		ShaderStaticCombos_t::ShaderCreationData_t *pCreationData = &lookup->m_ShaderStaticCombos.m_pCreationData[pshIndex];
#endif

		dxshader = g_pShaderDeviceDx11->CreatePixelShader( ( DWORD * )pCreationData->ByteCode.Base(), pCreationData->ByteCode.Count() );

#ifdef DYNAMIC_SHADER_COMPILE 
		// copy the compiled shader handle back to wherever it's supposed to be stored
		m_PixelShaderDict[shader].m_ShaderStaticCombos.m_pHardwareShaders[pshIndex] = dxshader;
#else
		lookup->m_ShaderStaticCombos.m_pHardwareShaders[pshIndex] = dxshader;
#endif
	}

	AssertMsg( dxshader != INVALID_HARDWARE_SHADER, "Failed to set pixel shader." );
	BindPixelShader( (PixelShaderHandle_t)dxshader );
}

//-----------------------------------------------------------------------------
// Resets the shader state
//-----------------------------------------------------------------------------
void CShaderManager::ResetShaderState()
{
	// This will force the calls to SetVertexShader + SetPixelShader to actually set the state
	m_HardwareVertexShader = (HardwareShader_t)-1;
	m_HardwarePixelShader = (HardwareShader_t)-1;

	BindVertexShader( VERTEX_SHADER_HANDLE_INVALID );
	BindPixelShader( PIXEL_SHADER_HANDLE_INVALID );
}

//-----------------------------------------------------------------------------
// Destroy a particular vertex shader
//-----------------------------------------------------------------------------
void CShaderManager::DestroyVertexShader( VertexShader_t shader )
{
	ShaderStaticCombos_t &combos = m_VertexShaderDict[shader].m_ShaderStaticCombos;
	int i;
	for ( i = 0; i < combos.m_nCount; i++ )
	{
		if ( combos.m_pHardwareShaders[i] != INVALID_HARDWARE_SHADER )
		{
			ID3D11VertexShader* pShader = (ID3D11VertexShader* )combos.m_pHardwareShaders[i];
			UnregisterVS( pShader );
#ifdef _DEBUG
			int nRetVal = 
#endif
				pShader->Release();
			Assert( nRetVal == 0 );
		}
	}
	delete [] combos.m_pHardwareShaders;
	combos.m_pHardwareShaders = NULL;

	if ( combos.m_pCreationData != NULL )
	{
		delete [] combos.m_pCreationData;
		combos.m_pCreationData = NULL;
	}

	m_VertexShaderDict.Remove( shader );
}

//-----------------------------------------------------------------------------
// Destroy a particular pixel shader
//-----------------------------------------------------------------------------
void CShaderManager::DestroyPixelShader( PixelShader_t pixelShader )
{
	ShaderStaticCombos_t &combos = m_PixelShaderDict[pixelShader].m_ShaderStaticCombos;
	int i;
	for ( i = 0; i < combos.m_nCount; i++ )
	{
		if ( combos.m_pHardwareShaders[i] != INVALID_HARDWARE_SHADER )
		{
			ID3D11PixelShader* pShader = ( ID3D11PixelShader * )combos.m_pHardwareShaders[i];
			UnregisterPS( pShader );
#ifdef _DEBUG
			int nRetVal = 
#endif
				pShader->Release();
			Assert( nRetVal == 0 );
		}
	}
	delete [] combos.m_pHardwareShaders;
	combos.m_pHardwareShaders = NULL;

	if ( combos.m_pCreationData != NULL )
	{
		delete [] combos.m_pCreationData;
		combos.m_pCreationData = NULL;
	}

	m_PixelShaderDict.Remove( pixelShader );
}


//-----------------------------------------------------------------------------
// Destroys all shaders
//-----------------------------------------------------------------------------
void CShaderManager::DestroyAllShaders( void )
{
	for ( VertexShader_t vshIndex = m_VertexShaderDict.Head(); 
		 vshIndex != m_VertexShaderDict.InvalidIndex(); )
	{
		Assert( m_VertexShaderDict[vshIndex].m_nRefCount >= 0 );
		VertexShader_t next = m_VertexShaderDict.Next( vshIndex );
		DestroyVertexShader( vshIndex );
		vshIndex = next;
	}

	for ( PixelShader_t pshIndex = m_PixelShaderDict.Head(); 
		 pshIndex != m_PixelShaderDict.InvalidIndex(); )
	{
		Assert( m_PixelShaderDict[pshIndex].m_nRefCount >= 0 );
		PixelShader_t next = m_PixelShaderDict.Next( pshIndex );
		DestroyPixelShader( pshIndex );
		pshIndex = next;
	}

	// invalidate the file cache
	m_ShaderFileCache.Purge();
}

//-----------------------------------------------------------------------------
// print all vertex and pixel shaders along with refcounts to the console
//-----------------------------------------------------------------------------
void CShaderManager::SpewVertexAndPixelShaders( void )
{
	// only spew a populated shader file cache
	Msg( "\nShader File Cache:\n" );
	for ( int cacheIndex = m_ShaderFileCache.Head(); 
		 cacheIndex != m_ShaderFileCache.InvalidIndex();
		 cacheIndex = m_ShaderFileCache.Next( cacheIndex ) )
	{
		ShaderFileCache_t *pCache = &m_ShaderFileCache[cacheIndex];
		Msg( "Total Combos:%9d Static:%9d Dynamic:%7d SeekTable:%7d Ver:%d '%s'\n", 
			pCache->m_Header.m_nTotalCombos, 
			pCache->m_Header.m_nTotalCombos/pCache->m_Header.m_nDynamicCombos,
			pCache->m_Header.m_nDynamicCombos,
			pCache->IsOldVersion() ? 0 : pCache->m_Header.m_nNumStaticCombos,
			pCache->m_Header.m_nVersion,
			m_ShaderSymbolTable.String( pCache->m_Filename ) );
	}
	Msg( "\n" );

	// spew vertex shader dictionary
	int totalVertexShaders = 0;
	int totalVertexShaderSets = 0;
	for ( VertexShader_t vshIndex = m_VertexShaderDict.Head(); 
		 vshIndex != m_VertexShaderDict.InvalidIndex();
		 vshIndex = m_VertexShaderDict.Next( vshIndex ) )
	{
		const auto &lookup = m_VertexShaderDict[vshIndex];
		const char *pName = m_ShaderSymbolTable.String( lookup.m_Name );
		Msg( "vsh 0x%8.8x: static combo:%9d dynamic combos:%6d refcount:%4d \"%s\"\n", vshIndex,
			( int )lookup.m_nStaticIndex, ( int )lookup.m_ShaderStaticCombos.m_nCount,
			lookup.m_nRefCount, pName );
		totalVertexShaders += lookup.m_ShaderStaticCombos.m_nCount;
		totalVertexShaderSets++;
	}

	// spew pixel shader dictionary
	int totalPixelShaders = 0;
	int totalPixelShaderSets = 0;
	for ( PixelShader_t pshIndex = m_PixelShaderDict.Head(); 
		 pshIndex != m_PixelShaderDict.InvalidIndex();
		 pshIndex = m_PixelShaderDict.Next( pshIndex ) )
	{
		const auto &lookup = m_PixelShaderDict[pshIndex];
		const char *pName = m_ShaderSymbolTable.String( lookup.m_Name );
		Msg( "psh 0x%8.8x: static combo:%9d dynamic combos:%6d refcount:%4d \"%s\"\n", pshIndex,
			( int )lookup.m_nStaticIndex, ( int )lookup.m_ShaderStaticCombos.m_nCount,
			lookup.m_nRefCount, pName );
		totalPixelShaders += lookup.m_ShaderStaticCombos.m_nCount;
		totalPixelShaderSets++;
	}

	Msg( "Total unique vertex shaders: %d\n", totalVertexShaders );
	Msg( "Total vertex shader sets: %d\n", totalVertexShaderSets );
	Msg( "Total unique pixel shaders: %d\n", totalPixelShaders );
	Msg( "Total pixel shader sets: %d\n", totalPixelShaderSets );
}

CON_COMMAND( mat_spewvertexandpixelshaders, "Print all vertex and pixel shaders currently loaded to the console" )
{
	( ( CShaderManager * )ShaderManager() )->SpewVertexAndPixelShaders();
}

const char *CShaderManager::GetActiveVertexShaderName()
{
#if !defined( _DEBUG )
	return "";
#else
	if ( !m_HardwareVertexShader )
	{
		return "NULL";
	}
	return vshDebugName[vshDebugIndex];
#endif
}

const char *CShaderManager::GetActivePixelShaderName()
{
#if !defined( _DEBUG )
	return "";
#else
	if ( !m_HardwarePixelShader )
	{
		return "NULL";
	}
	return pshDebugName[pshDebugIndex];
#endif
}

#ifdef DYNAMIC_SHADER_COMPILE
void CShaderManager::FlushShaders( void )
{
	for( VertexShader_t shader = m_VertexShaderDict.Head(); 
	     shader != m_VertexShaderDict.InvalidIndex(); 
		 shader = m_VertexShaderDict.Next( shader ) )
	{
		int i;
		ShaderStaticCombos_t *combos = &m_VertexShaderDict[shader].m_ShaderStaticCombos;
		if( !( m_VertexShaderDict[shader].m_Flags & SHADER_DYNAMIC_COMPILE_IS_HLSL ) )
		{
			// don't nuke non-HLSL shaders since we don't dynamically compile them.
			continue;
		}
		for( i = 0; i < combos->m_nCount; i++ )
		{
			if( combos->m_pHardwareShaders[i] != INVALID_HARDWARE_SHADER )
			{
#ifdef _DEBUG
				int nRetVal=
#endif
					( ( ID3D11VertexShader * )combos->m_pHardwareShaders[i] )->Release();
				Assert( nRetVal == 0 );
			}
			combos->m_pHardwareShaders[i] = INVALID_HARDWARE_SHADER;
		}
	}

	for( PixelShader_t shader = m_PixelShaderDict.Head(); 
	     shader != m_PixelShaderDict.InvalidIndex(); 
		 shader = m_PixelShaderDict.Next( shader ) )
	{
		int i;
		ShaderStaticCombos_t &combos = m_PixelShaderDict[shader].m_ShaderStaticCombos;
		if( !( m_PixelShaderDict[shader].m_Flags & SHADER_DYNAMIC_COMPILE_IS_HLSL ) )
		{
			// don't nuke non-HLSL shaders since we don't dynamically compile them.
			continue;
		}
		for( i = 0; i < combos.m_nCount; i++ )
		{
			if( combos.m_pHardwareShaders[i] != INVALID_HARDWARE_SHADER )
			{
#ifdef _DEBUG
				int nRetVal =
#endif
					( ( ID3D11PixelShader * )combos.m_pHardwareShaders[i] )->Release();
				Assert( nRetVal == 0 );
			}
			combos.m_pHardwareShaders[i] = INVALID_HARDWARE_SHADER;
		}
	}

	// invalidate the file cache
	m_ShaderFileCache.Purge();
}
#endif

#ifdef DYNAMIC_SHADER_COMPILE
CON_COMMAND( mat_flushshaders, "flush all hardware shaders when using DYNAMIC_SHADER_COMPILE" )
{
	( ( CShaderManager * )ShaderManager() )->FlushShaders();
}
#endif