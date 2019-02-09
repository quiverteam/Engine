//====== Copyright c 1996-2007, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "tier2/p4helpers.h"
#include "tier2/tier2.h"
//#include "p4lib/ip4.h"

//////////////////////////////////////////////////////////////////////////
//
// CP4File implementation
//
//////////////////////////////////////////////////////////////////////////

CP4File::CP4File(char const *szFilename) :
	m_sFilename(szFilename)
{
}

CP4File::~CP4File()
{
}

bool CP4File::Edit(void)
{
	return true;
}

bool CP4File::Add(void)
{
	return true;
}

// Is the file in perforce?
bool CP4File::IsFileInPerforce()
{
	return false;
}


//////////////////////////////////////////////////////////////////////////
//
// CP4Factory implementation
//
//////////////////////////////////////////////////////////////////////////


CP4Factory::CP4Factory()
{
}

CP4Factory::~CP4Factory()
{
}

bool CP4Factory::SetDummyMode(bool bDummyMode)
{
	return true;
}

void CP4Factory::SetOpenFileChangeList(const char *szChangeListName)
{
}

CP4File *CP4Factory::AccessFile(char const *szFilename) const
{
	if (!m_bDummyMode)
		return new CP4File(szFilename);
	else
		return new CP4File_Dummy(szFilename);
}


// Default p4 factory
static CP4Factory s_static_p4_factory;
CP4Factory *g_p4factory = &s_static_p4_factory; // NULL before the factory constructs
