#include "packfile.h"
#include "tier1/utlbuffer.h"
#include "tier1/lzmaDecoder.h"
#include "generichash.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar fs_monitor_read_from_pack;

//-----------------------------------------------------------------------------
// Low Level I/O routine for reading from pack files.
// Offsets all reads by the base of the pack file as needed.
// Return bytes read.
//-----------------------------------------------------------------------------
int CPackFile::ReadFromPack( int nIndex, void* buffer, int nDestBytes, int nBytes, int64 nOffset )
{
	m_mutex.Lock();

	if ( fs_monitor_read_from_pack.GetInt() == 1 || ( fs_monitor_read_from_pack.GetInt() == 2 && ThreadInMainThread() ) )
	{
		// spew info about real i/o request
		char szName[MAX_PATH];
		IndexToFilename( nIndex, szName, sizeof( szName ) );
		Msg( "Read From Pack: Sync I/O: Requested:%7d, Offset:0x%16.16x, %s\n", nBytes, m_nBaseOffset + nOffset, szName );
	}

	// Seek to the start of the read area and perform the read: TODO: CHANGE THIS INTO A CFileHandle
	m_fs->FS_fseek( m_hPackFileHandle, m_nBaseOffset + nOffset, SEEK_SET );
	int nBytesRead = m_fs->FS_fread( buffer, nDestBytes, nBytes, m_hPackFileHandle );

	m_mutex.Unlock();

	return nBytesRead;
}

//-----------------------------------------------------------------------------
// Open a file inside of a pack file.
//-----------------------------------------------------------------------------
CFileHandle *CPackFile::OpenFile( const char *pFileName, const char *pOptions )
{
	int nIndex, nLength;
	int64 nPosition;

	// find the file's location in the pack
	if ( FindFile( pFileName, nIndex, nPosition, nLength ) )
	{
		m_mutex.Lock();
		if ( m_nOpenFiles == 0 && m_hPackFileHandle == NULL )
		{
			m_hPackFileHandle = m_fs->Trace_FOpen( m_PackName, "rb", 0, NULL );
		}
		m_nOpenFiles++;
		m_mutex.Unlock();
		CPackFileHandle* ph = new CPackFileHandle( this, nPosition, nLength, nIndex );
		CFileHandle *fh = new CFileHandle( m_fs );
		fh->m_pPackFileHandle = ph;
		fh->m_nLength = nLength;

		// The default mode for fopen is text, so require 'b' for binary
		if ( strstr( pOptions, "b" ) == NULL )
		{
			fh->m_type = FT_PACK_TEXT;
		}
		else
		{
			fh->m_type = FT_PACK_BINARY;
		}

#if !defined( _RETAIL )
		fh->SetName( pFileName );
#endif
		return fh;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//	Get a directory entry from a pack's preload section
//-----------------------------------------------------------------------------
ZIP_PreloadDirectoryEntry* CZipPackFile::GetPreloadEntry( int nEntryIndex )  
{
	if ( !m_pPreloadHeader )
	{
		return NULL;
	}

	// If this entry doesn't have a corresponding preload entry, fail.
	if ( m_PackFiles[nEntryIndex].m_nPreloadIdx == INVALID_PRELOAD_ENTRY )
	{
		return NULL;
	}
	
	return &m_pPreloadDirectory[m_PackFiles[nEntryIndex].m_nPreloadIdx];
}

//-----------------------------------------------------------------------------
//	Read a file from the pack
//-----------------------------------------------------------------------------
int CZipPackFile::ReadFromPack( int nEntryIndex, void* pBuffer, int nDestBytes, int nBytes, int64 nOffset )
{
	if ( nEntryIndex >= 0 )
	{
		if ( nBytes <= 0 ) 
		{
			return 0;
		}

		// X360TBD: This is screwy, it works because m_nBaseOffset is 0 for preload capable zips
		// It comes into play for files out of the embedded bsp zip,
		// this hackery is a pre-bias expecting ReadFromPack() do a symmetric post bias, yuck.
		nOffset -= m_nBaseOffset;

		// Attempt to satisfy request from possible preload section, otherwise fall through
		// A preload entry may be compressed
		ZIP_PreloadDirectoryEntry *pPreloadEntry = GetPreloadEntry( nEntryIndex );
		if ( pPreloadEntry )
		{
			// convert the absolute pack file position to a local file position 
			int nLocalOffset = nOffset - m_PackFiles[nEntryIndex].m_nPosition;
			byte *pPreloadData = (byte*)m_pPreloadData + pPreloadEntry->DataOffset;

			CLZMA lzma;
			if ( lzma.IsCompressed( pPreloadData ) )
			{
				unsigned int actualSize = lzma.GetActualSize( pPreloadData );
				if ( nLocalOffset + nBytes <= (int)actualSize )
				{
					// satisfy from compressed preload
					if ( fs_monitor_read_from_pack.GetInt() == 1 )
					{
						Msg( "Read From Pack: [Preload] Requested:%d Compressed:%d\n", nBytes, pPreloadEntry->Length );
					}

					if ( nLocalOffset == 0 && nDestBytes >= (int)actualSize && nBytes == (int)actualSize )
					{
						// uncompress directly into caller's buffer
						lzma.Uncompress( (unsigned char *)pPreloadData, (unsigned char *)pBuffer );
						return nBytes;
					}
			
					// uncompress into temporary memory
					CUtlMemory< byte > tempMemory;
					tempMemory.EnsureCapacity( actualSize );
					lzma.Uncompress( pPreloadData, tempMemory.Base() );
					// copy only what caller expects
					V_memcpy( pBuffer, (byte*)tempMemory.Base() + nLocalOffset, nBytes );
					return nBytes;
				}
			}
			else if ( nLocalOffset + nBytes <= (int)pPreloadEntry->Length )
			{
				// satisfy from preload
				if ( fs_monitor_read_from_pack.GetInt() == 1 )
				{
					Msg( "Read From Pack: [Preload] Requested:%d Total:%d\n", nBytes, pPreloadEntry->Length );
				}

				V_memcpy( pBuffer, pPreloadData + nLocalOffset, nBytes );
				return nBytes;
			}
		}
	}

	// fell through as a direct request
	return CPackFile::ReadFromPack( nEntryIndex, pBuffer, nDestBytes, nBytes, nOffset );	
}

//-----------------------------------------------------------------------------
//	Gets size, position, and index for a file in the pack.
//-----------------------------------------------------------------------------
bool CZipPackFile::GetOffsetAndLength( const char *pFileName, int &nBaseIndex, int64 &nFileOffset, int &nLength )
{
	CZipPackFile::CPackFileEntry lookup;
	lookup.m_HashName = HashStringCaselessConventional( pFileName );

	int idx = m_PackFiles.Find( lookup );
	if ( -1 != idx  )
	{
		nFileOffset = m_PackFiles[idx].m_nPosition;
		nLength = m_PackFiles[idx].m_nLength;
		nBaseIndex = idx;
		return true;
	}

	return false;
}

bool CZipPackFile::IndexToFilename( int nIndex, char *pBuffer, int nBufferSize )
{
#if !defined( _RETAIL )
	if ( nIndex >= 0 )
	{
		m_fs->String( m_PackFiles[nIndex].m_hDebugFilename, pBuffer, nBufferSize );
		return true;
	}
#endif

	Q_strncpy( pBuffer, "unknown", nBufferSize );

	return false;
}

//-----------------------------------------------------------------------------
//	Find a file in the pack.
//-----------------------------------------------------------------------------
bool CZipPackFile::FindFile( const char *pFilename, int &nIndex, int64 &nOffset, int &nLength )
{
	char szCleanName[MAX_FILEPATH];
	Q_strncpy( szCleanName, pFilename, sizeof( szCleanName ) );
#ifdef _WIN32
	Q_strlower( szCleanName );
#endif
	Q_FixSlashes( szCleanName );
 
	if ( !Q_RemoveDotSlashes( szCleanName ) )
	{
		return false;
	}
	
    bool bFound = GetOffsetAndLength( szCleanName, nIndex, nOffset, nLength );

	nOffset += m_nBaseOffset;
	return bFound;
}


//-----------------------------------------------------------------------------
//	Set up the preload section
//-----------------------------------------------------------------------------
void CZipPackFile::SetupPreloadData()
{
	if ( m_pPreloadHeader || !m_nPreloadSectionSize )
	{
		// already loaded or not availavble
		return;
	}

	MEM_ALLOC_CREDIT_( "xZip" );

	void *pPreload = malloc( m_nPreloadSectionSize );
	if ( !pPreload )
	{
		return;
	}

	if ( IsX360() )
	{
		// 360 XZips are always dvd aligned
		Assert( ( m_nPreloadSectionSize % XBOX_DVD_SECTORSIZE ) == 0 );
		Assert( ( m_nPreloadSectionOffset % XBOX_DVD_SECTORSIZE ) == 0 );
	}

	// preload data is loaded as a single unbuffered i/o operation
	ReadFromPack( -1, pPreload, -1, m_nPreloadSectionSize, m_nPreloadSectionOffset );

	// setup the header
	m_pPreloadHeader = (ZIP_PreloadHeader *)pPreload;

	// setup the preload directory
	m_pPreloadDirectory = (ZIP_PreloadDirectoryEntry *)((byte *)m_pPreloadHeader + sizeof( ZIP_PreloadHeader ) );

	// setup the remap table
	m_pPreloadRemapTable = (unsigned short *)((byte *)m_pPreloadDirectory + m_pPreloadHeader->PreloadDirectoryEntries * sizeof( ZIP_PreloadDirectoryEntry ) );

	// set the preload data base
	m_pPreloadData = (byte *)m_pPreloadRemapTable + m_pPreloadHeader->DirectoryEntries * sizeof( unsigned short );
}

void CZipPackFile::DiscardPreloadData()
{
	if ( !m_pPreloadHeader )
	{
		// already discarded
		return;
	}

	free( m_pPreloadHeader );
	m_pPreloadHeader = NULL;
}

//-----------------------------------------------------------------------------
//	Parse the zip file to build the file directory and preload section
//-----------------------------------------------------------------------------
bool CZipPackFile::Prepare( int64 fileLen, int64 nFileOfs )
{
	if ( !fileLen || fileLen < sizeof( ZIP_EndOfCentralDirRecord ) )
	{
		// nonsense zip
		return false;
	}

	// Pack files are always little-endian
	m_swap.ActivateByteSwapping( IsX360() );

	m_FileLength = fileLen;
	m_nBaseOffset = nFileOfs;

	ZIP_EndOfCentralDirRecord rec = { 0 };

	// Find and read the central header directory from its expected position at end of the file
	bool bCentralDirRecord = false;
	int64 offset = fileLen - sizeof( ZIP_EndOfCentralDirRecord );

	// 360 can have an inompatible  
	bool bCompatibleFormat = true;
	if ( IsX360() )
	{
		// 360 has dependable exact zips, backup to handle possible xzip format
		if ( offset - XZIP_COMMENT_LENGTH >= 0 )
		{
			offset -= XZIP_COMMENT_LENGTH;
		}
	
		// single i/o operation, scanning forward
		char *pTemp = (char *)_alloca( fileLen - offset );
		ReadFromPack( -1, pTemp, -1, fileLen - offset, offset );
		while ( offset <= fileLen - sizeof( ZIP_EndOfCentralDirRecord ) )
		{
			memcpy( &rec, pTemp, sizeof( ZIP_EndOfCentralDirRecord ) );
			m_swap.SwapFieldsToTargetEndian( &rec );
			if ( rec.signature == PKID( 5, 6 ) )
			{
				bCentralDirRecord = true;
				if ( rec.commentLength >= 4 )
				{
					char *pComment = pTemp + sizeof( ZIP_EndOfCentralDirRecord );
					if ( !V_strnicmp( pComment, "XZP2", 4 ) )
					{
						bCompatibleFormat = false;
					}
				}
				break;
			}
			offset++;
			pTemp++;
		}
	}
	else
	{
		// scan entire file from expected location for central dir
		for ( ; offset >= 0; offset-- )
		{
			ReadFromPack( -1, (void*)&rec, -1, sizeof( rec ), offset );
			m_swap.SwapFieldsToTargetEndian( &rec );
			if ( rec.signature == PKID( 5, 6 ) )
			{
				bCentralDirRecord = true;
				break;
			}
		}
	}
	Assert( bCentralDirRecord );
	if ( !bCentralDirRecord )
	{
		// no zip directory, bad zip
		return false;
	}

	int numFilesInZip = rec.nCentralDirectoryEntries_Total;
	if ( numFilesInZip <= 0 )
	{
		// empty valid zip
		return true;
	}

	int firstFileIdx = 0;

	MEM_ALLOC_CREDIT();

	// read central directory into memory and parse
	CUtlBuffer zipDirBuff( 0, rec.centralDirectorySize, 0 );
	zipDirBuff.EnsureCapacity( rec.centralDirectorySize );
	zipDirBuff.ActivateByteSwapping( IsX360() );
	ReadFromPack( -1, zipDirBuff.Base(), -1, rec.centralDirectorySize, rec.startOfCentralDirOffset );
	zipDirBuff.SeekPut( CUtlBuffer::SEEK_HEAD, rec.centralDirectorySize );

	ZIP_FileHeader zipFileHeader;
	char filename[MAX_PATH];

	// Check for a preload section, expected to be the first file in the zip
	zipDirBuff.GetObjects( &zipFileHeader );
	zipDirBuff.Get( filename, zipFileHeader.fileNameLength );
	filename[zipFileHeader.fileNameLength] = '\0';
	if ( !V_stricmp( filename, PRELOAD_SECTION_NAME ) )
	{
		m_nPreloadSectionSize = zipFileHeader.uncompressedSize;
		m_nPreloadSectionOffset = zipFileHeader.relativeOffsetOfLocalHeader + 
						  sizeof( ZIP_LocalFileHeader ) + 
						  zipFileHeader.fileNameLength + 
						  zipFileHeader.extraFieldLength;
		SetupPreloadData();

		// Set up to extract the remaining files
		int nextOffset = bCompatibleFormat ? zipFileHeader.extraFieldLength + zipFileHeader.fileCommentLength : 0;
		zipDirBuff.SeekGet( CUtlBuffer::SEEK_CURRENT, nextOffset );
		firstFileIdx = 1;
	}
	else
	{
		if ( IsX360() )
		{
			// all 360 zip files are expected to have preload sections
			// only during development, maps are allowed to lack them, due to auto-conversion
			if ( !m_bIsMapPath || g_pFullFileSystem->GetDVDMode() == DVDMODE_STRICT )
			{
				Warning( "ZipFile '%s' missing preload section\n", m_PackName.String() );
			}
		}

		// No preload section, reset buffer pointer
		zipDirBuff.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
	}

	// Parse out central directory and determine absolute file positions of data.
	// Supports uncompressed zip files, with or without preload sections
	bool bSuccess = true;
	char tmpString[MAX_PATH];		
	CZipPackFile::CPackFileEntry lookup;

	m_PackFiles.EnsureCapacity( numFilesInZip );

	for ( int i = firstFileIdx; i < numFilesInZip; ++i )
	{
		zipDirBuff.GetObjects( &zipFileHeader );
		if ( zipFileHeader.signature != PKID( 1, 2 ) || zipFileHeader.compressionMethod != 0 )
		{
			Msg( "Incompatible pack file detected! %s\n", ( zipFileHeader.compressionMethod != 0 ) ? " File is compressed" : "" );
			bSuccess = false;
			break;	
		}

		Assert( zipFileHeader.fileNameLength < sizeof( tmpString ) );
		zipDirBuff.Get( (void *)tmpString, zipFileHeader.fileNameLength );
		tmpString[zipFileHeader.fileNameLength] = '\0';
		Q_FixSlashes( tmpString );

#if !defined( _RETAIL )
		lookup.m_hDebugFilename = m_fs->FindOrAddFileName( tmpString );
#endif
		lookup.m_HashName = HashStringCaselessConventional( tmpString );
		lookup.m_nLength = zipFileHeader.uncompressedSize;
		lookup.m_nPosition = zipFileHeader.relativeOffsetOfLocalHeader + 
								sizeof( ZIP_LocalFileHeader ) + 
								zipFileHeader.fileNameLength + 
								zipFileHeader.extraFieldLength;

		// track the index to this file's possible preload directory entry
		if ( m_pPreloadRemapTable )
		{
			lookup.m_nPreloadIdx = m_pPreloadRemapTable[i];
		}
		else
		{
			lookup.m_nPreloadIdx = INVALID_PRELOAD_ENTRY;
		}
		m_PackFiles.InsertNoSort( lookup );

		int nextOffset = bCompatibleFormat ? zipFileHeader.extraFieldLength + zipFileHeader.fileCommentLength : 0;
		zipDirBuff.SeekGet( CUtlBuffer::SEEK_CURRENT, nextOffset );
	}

	m_PackFiles.RedoSort();

	return bSuccess;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CZipPackFile::CZipPackFile( CBaseFileSystem* fs )
 : m_PackFiles()
{
	m_fs = fs;	
	m_pPreloadDirectory = NULL;
	m_pPreloadData = NULL;
	m_pPreloadHeader = NULL;
	m_pPreloadRemapTable = NULL;
	m_nPreloadSectionOffset = 0;
	m_nPreloadSectionSize = 0;
}

CZipPackFile::~CZipPackFile()
{
	DiscardPreloadData();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : src1 - 
//			src2 - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CZipPackFile::CPackFileLessFunc::Less( CZipPackFile::CPackFileEntry const& src1, CZipPackFile::CPackFileEntry const& src2, void *pCtx )
{
	return ( src1.m_HashName < src2.m_HashName );
}

static bool VPK_FileEntry_LessFunc( const VPKFileEntry_t* const &p1, const VPKFileEntry_t* const &p2 )
{
	return ( p1 < p2 );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CVPKFile::CVPKFile( CBaseFileSystem* fs, bool bVolumes, VPKHeader_t vpkheader, const char *pszBasePath ) :
	m_bVolumes( bVolumes ),
	m_vpkHeader( vpkheader )
{
	V_strncpy( m_szBasePath, pszBasePath, sizeof( m_szBasePath ) );
	m_fs = fs;
	m_bIsVPK = true;

	// Why does CUtlMap make me define this for a pointer type? WHY?
	m_ExtensionMap.SetLessFunc( VPK_FileEntry_LessFunc );
	m_ReverseFileMap.SetLessFunc( VPK_FileEntry_LessFunc );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CVPKFile::~CVPKFile()
{
	for ( auto archive : m_hArchiveHandles )
		m_fs->Trace_FClose( archive );

	m_pFileEntries.PurgeAndDeleteElements();

	// We can't use PurgeAndDeleteElements because we need to use delete[]
	for ( unsigned short i = 0; i < m_ExtensionMap.MaxElement(); ++i )
	{
		if ( !m_ExtensionMap.IsValidIndex( i ) )
			continue;

		delete[] m_ExtensionMap.Element( i );
	}

	for ( unsigned short i = 0; i < m_ReverseFileMap.MaxElement(); ++i )
	{
		if ( !m_ReverseFileMap.IsValidIndex( i ) )
			continue;

		delete[] m_ReverseFileMap.Element( i );
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CVPKFile::Prepare( int64 fileLen, int64 nFileOfs )
{
	const int nHeaderSize = (int)m_vpkHeader.GetHeaderSize();

	if ( nHeaderSize == 0 )
		return false;
	
	if ( fileLen < nHeaderSize )
		return false;

	// Read the directory tree and store entry file 
	m_fs->FS_fseek( m_hPackFileHandle, nHeaderSize, FILESYSTEM_SEEK_HEAD );

	while ( true )
	{
		CUtlVector< char > extension;
		ReadString( extension, m_hPackFileHandle );

		if ( extension[ 0 ] == '\0' )
			break;

		while ( true )
		{
			CUtlVector< char > base_path;
			ReadString( base_path, m_hPackFileHandle );

			if ( base_path[ 0 ] == '\0' )
				break;

			V_FixSlashes( base_path.Base() );

			const bool bPathEmpty = ( base_path[ 0 ] == ' ' && base_path[ 1 ] == '\0' );

			while ( true )
			{
				CUtlVector< char > file_name;
				ReadString( file_name, m_hPackFileHandle );

				if ( file_name[ 0 ] == '\0' )
					break;

				VPKFileEntry_t *pFileEntry = new VPKFileEntry_t;
				pFileEntry->index = m_pFileEntries.AddToTail( pFileEntry );

				VPKDirectoryEntry_t &entry = pFileEntry->entry;
				m_fs->FS_fread( &entry.CRC, sizeof( entry.CRC ), m_hPackFileHandle );
				m_fs->FS_fread( &entry.PreloadBytes, sizeof( entry.PreloadBytes ), m_hPackFileHandle );
				m_fs->FS_fread( &entry.ArchiveIndex, sizeof( entry.ArchiveIndex ), m_hPackFileHandle );
				m_fs->FS_fread( &entry.EntryOffset, sizeof( entry.EntryOffset ), m_hPackFileHandle );
				m_fs->FS_fread( &entry.EntryLength, sizeof( entry.EntryLength ), m_hPackFileHandle );
				m_fs->FS_fread( &entry.Terminator, sizeof( entry.Terminator ), m_hPackFileHandle );

				if ( entry.PreloadBytes )
				{
#if VPK_DEBUG
					Warning( "VPK file entry %s/%s.%s has preload data and will be ignored\n", base_path.Base(), file_name.Base(), extension.Base() );
#endif
					m_fs->FS_fseek( m_hPackFileHandle, entry.PreloadBytes, FILESYSTEM_SEEK_CURRENT );
				}
				else
				{
					// NOTE: These get inserted into CUtlMaps and will be deleted in CVPKFile destructor
					char *pszFullFilePath = new char[ MAX_PATH ];
					char *pszExtension = new char[ extension.Count() ];

					V_strcpy( pszExtension, extension.Base() );

					if ( bPathEmpty )
						V_snprintf( pszFullFilePath, MAX_PATH, "%s.%s", file_name.Base(), extension.Base() );
					else
						V_snprintf( pszFullFilePath, MAX_PATH, "%s%c%s.%s", base_path.Base(), CORRECT_PATH_SEPARATOR, file_name.Base(), extension.Base() );
				
					m_PathMap[ base_path.Base() ].AddToTail( pFileEntry );
					m_FileMap[ pszFullFilePath ] = pFileEntry;
					m_ReverseFileMap.Insert( pFileEntry, pszFullFilePath );
					m_ExtensionMap.Insert( pFileEntry, pszExtension );
				}
			}
		}
	}

	if ( m_bVolumes )
	{
		int volNum = 0;

		while ( true )
		{
			char volPath[ MAX_PATH ];
			V_snprintf( volPath, MAX_PATH, "%s_%03d.vpk", m_szBasePath, volNum );
			++volNum;

			FILE *archive = m_fs->Trace_FOpen( volPath, "rb", 0, NULL );
			if ( !archive )
				break;

			m_hArchiveHandles.AddToTail( archive );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
//	Find a file in the VPK.
//-----------------------------------------------------------------------------
bool CVPKFile::FindFile( const char *pFilename, int &nIndex, int64 &nOffset, int &nLength )
{
	char fixedfilename[ MAX_PATH ];
	V_strncpy( fixedfilename, pFilename, sizeof( fixedfilename ) );

	V_FixDoubleSlashes( fixedfilename );
	V_FixSlashes( fixedfilename );

	UtlSymId_t id = m_FileMap.Find( fixedfilename );
	const VPKFileEntry_t *pFileEntry = ( id == m_FileMap.InvalidIndex() ) ? nullptr : m_FileMap[ id ];

	if ( !pFileEntry )
		return false;

	const size_t headerSize = m_vpkHeader.GetHeaderSize();

	nIndex = pFileEntry->index;

	// Is this file in the vpk directory file?
	if ( pFileEntry->entry.ArchiveIndex == 0x7fff )
		nOffset = (int64)headerSize + (int64)m_vpkHeader.TreeSize + (int64)pFileEntry->entry.EntryOffset;
	else
		nOffset = (int64)pFileEntry->entry.EntryOffset;

	nLength = pFileEntry->entry.EntryLength;

	return true;
}

bool CVPKFile::FindFirst( CBaseFileSystem::FindData_t *pFindData )
{
	char extension[ FILENAME_MAX ];
	V_ExtractFileExtension( pFindData->wildCardString.Base(), extension, sizeof( extension ) );

#ifdef _WIN32
	V_strlower( extension );
#endif

	if ( extension[0] == '\0' || ( extension[0] == '*' && extension[1] == '\0' ) )
	{
		// We're looking for directories
		char path[MAX_FILEPATH];
		strncpy( path, pFindData->wildCardString.Base(), sizeof( path ) );
		V_FixSlashes( path );

		strtok( path, "*" );
		V_StripTrailingSlash( path );
#if _WIN32
		V_strlower( path );
#endif
		const int numPathMapStrings = m_PathMap.GetNumStrings();
		for ( int i = 0; i < numPathMapStrings; ++i )
		{
			const char *current_path = m_PathMap.String( i );
			if ( V_stricmp( path, current_path ) == 0 )
				continue;

			char parent_path[ MAX_PATH ];
			strncpy( parent_path, current_path, sizeof( parent_path ) );

			if ( V_StripLastDir( parent_path, sizeof( parent_path ) ) )
			{
				V_StripTrailingSlash( parent_path );

				if ( V_stricmp( parent_path, path ) == 0 )
				{
					pFindData->pfFindData.directoryList.CopyAndAddToTail( current_path );
				}
			}
		}

		const int numFileMapStrings = m_FileMap.GetNumStrings();
		for ( int i = 0; i < numFileMapStrings; ++i )
		{
			const char *current_file = m_FileMap.String( i );
			char filepath[ MAX_PATH ];
			strncpy( filepath, current_file, sizeof( filepath ) );
			V_StripFilename( filepath );

			if ( V_stricmp( filepath, path ) == 0 )
				pFindData->pfFindData.fileList.CopyAndAddToTail( current_file );
		}

		return FindNext( pFindData );
	}
	else
	{
		// We're looking for files
		char filepath[ MAX_FILEPATH ];
		V_strncpy( filepath, pFindData->wildCardString.Base(), sizeof( filepath ) );
		V_FixSlashes( filepath );

		if ( strtok( filepath, "*" ) != nullptr )
		{
			V_StripTrailingSlash( filepath );
#if _WIN32
			V_strlower( filepath );
#endif
			UtlSymId_t pathid = m_PathMap.Find( filepath );

			if ( pathid != m_PathMap.InvalidIndex() )
			{
				CUtlVector< const VPKFileEntry_t* > &fileEntryList = m_PathMap[ pathid ];

				for ( auto entry : fileEntryList )
				{
					unsigned int extid = m_ExtensionMap.Find( entry );

					if ( extid != m_ExtensionMap.InvalidIndex() )
					{
						if ( V_stricmp( m_ExtensionMap[ extid ], extension ) == 0 )
						{
							unsigned int fullpathid = m_ReverseFileMap.Find( entry );

							if ( fullpathid != m_ReverseFileMap.InvalidIndex() )
								pFindData->pfFindData.fileList.CopyAndAddToTail( m_ReverseFileMap[ fullpathid ] );
						}
					}
				}

				return FindNext( pFindData );
			}
		}
	}

	return false;
}

bool CVPKFile::FindNext( CBaseFileSystem::FindData_t *pFindData )
{
	if ( !pFindData->pfFindData.directoryList.IsEmpty() )
	{
		char *head = pFindData->pfFindData.directoryList.Head();
		pFindData->findData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

		const char *pszUnqualified = V_UnqualifiedFileName( head );
		strncpy( pFindData->findData.cFileName, pszUnqualified, sizeof( pFindData->findData.cFileName ) );
		delete head;
		pFindData->pfFindData.directoryList.Remove( 0 );
		return true;
	}

	if ( !pFindData->pfFindData.fileList.IsEmpty() )
	{
		char *head = pFindData->pfFindData.fileList.Head();
		pFindData->findData.dwFileAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;

		const char *pszUnqualified = V_UnqualifiedFileName( head );
		strncpy( pFindData->findData.cFileName, pszUnqualified, sizeof( pFindData->findData.cFileName ) );
		delete head;
		pFindData->pfFindData.fileList.Remove( 0 );
		return true;
	}

	return false;
}

int CVPKFile::ReadFromPack( int nIndex, void* buffer, int nDestBytes, int nBytes, int64 nOffset )
{
	m_mutex.Lock();

	if ( fs_monitor_read_from_pack.GetInt() == 1 || ( fs_monitor_read_from_pack.GetInt() == 2 && ThreadInMainThread() ) )
	{
		// spew info about real i/o request
		char szName[MAX_PATH];
		IndexToFilename( nIndex, szName, sizeof( szName ) );
		Msg( "Read From Pack: Sync I/O: Requested:%7d, Offset:0x%16.16x, %s\n", nBytes, m_nBaseOffset + nOffset, szName );
	}

	// Seek to the start of the read area and perform the read: TODO: CHANGE THIS INTO A CFileHandle
	FILE *vpk = nullptr;
	if ( m_bVolumes )
	{
		const int archiveIndex = m_pFileEntries[ nIndex ]->entry.ArchiveIndex;
		Assert( archiveIndex < m_hArchiveHandles.Count() );

		vpk = m_hArchiveHandles[ archiveIndex ];
	}
	else
		vpk = m_hPackFileHandle;

	m_fs->FS_fseek( vpk, m_nBaseOffset + nOffset, SEEK_SET );
	int nBytesRead = m_fs->FS_fread( buffer, nDestBytes, nBytes, vpk );
	m_mutex.Unlock();

	return nBytesRead;
}

void CVPKFile::ReadString( CUtlVector< char > &buffer, FILE *pArchiveFile )
{
	while ( true )
	{
		char &c = buffer.Element( buffer.AddToTail() );
		m_fs->FS_fread( &c, 1, pArchiveFile );

		if ( c == '\0' )
			break;
	}
}