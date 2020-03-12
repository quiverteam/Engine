#include "shaderlib/cshader.h"
class unlitgeneric_vs40_Static_Index
{
private:
	int m_nVERTEXCOLOR;
#ifdef _DEBUG
	bool m_bVERTEXCOLOR;
#endif
public:
	void SetVERTEXCOLOR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVERTEXCOLOR = i;
#ifdef _DEBUG
		m_bVERTEXCOLOR = true;
#endif
	}
	void SetVERTEXCOLOR( bool i )
	{
		m_nVERTEXCOLOR = i ? 1 : 0;
#ifdef _DEBUG
		m_bVERTEXCOLOR = true;
#endif
	}
public:
	unlitgeneric_vs40_Static_Index( )
	{
#ifdef _DEBUG
		m_bVERTEXCOLOR = false;
#endif // _DEBUG
		m_nVERTEXCOLOR = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bVERTEXCOLOR;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nVERTEXCOLOR ) + 0;
	}
};
#define shaderStaticTest_unlitgeneric_vs40 vsh_forgot_to_set_static_VERTEXCOLOR + 0
class unlitgeneric_vs40_Dynamic_Index
{
public:
	unlitgeneric_vs40_Dynamic_Index()
	{
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
#endif // _DEBUG
		return 0;
	}
};
#define shaderDynamicTest_unlitgeneric_vs40 0
