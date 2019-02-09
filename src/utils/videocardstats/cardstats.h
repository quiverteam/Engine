//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
//=============================================================================

#ifndef CARDSTATS_H
#define CARDSTATS_H

#include "UtlBuffer.h"
#include "UtlVector.h"
#include "strtools.h"

int LoadFileIntoBuffer( CUtlBuffer &buf, char *pszFilename );
void WriteOutputToFile( CUtlBuffer &buf, char *pszFilename, char *pszSearchString );
void ParseHeader( CUtlBuffer &buf );
void ParseFile( CUtlBuffer &inbuf, CUtlBuffer &outbuf, char *pszSearchString, int size, int binwidth, int nofilter );
void ParseFile2( CUtlBuffer &inbuf, CUtlBuffer &outbuf, char *pszSearchString, int size, int binwidth );
void ParseFile3( CUtlBuffer &inbuf, CUtlBuffer &outbuf, char *pszSearchString, int size, int binwidth);
void TimeSeriesCPU( CUtlBuffer &inbuf, CUtlBuffer &outbuf, char *pszSearchString, int size, int binwidth);
void TimeSeriesVCard( CUtlBuffer &inbuf, CUtlBuffer &outbuf, char *pszSearchString, int size, int binwidth);
void HistogramVidCards( CUtlBuffer &inbuf, CUtlBuffer &outbuf, char *pszSearchString, int size, int binwidth);
void HistogramNetSpeed( CUtlBuffer &inbuf, CUtlBuffer &outbuf, char *pszSearchString, int size, int binwidth);
void HistogramRam( CUtlBuffer &inbuf, CUtlBuffer &outbuf, char *pszSearchString, int size, int binwidth);
void HistogramCPU( CUtlBuffer &inbuf, CUtlBuffer &outbuf, char *pszSearchString, int size, int binwidth);
void InsertResult( int nCpu, CUtlVector<int> &nCpuList, CUtlVector<int> &nQuantity );

#endif