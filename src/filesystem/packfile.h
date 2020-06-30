#ifndef PACKFILE_H
#define PACKFILE_H

#include <cstdint>

#include "basefilesystem.h"
#include "tier1/refcount.h"

struct PackFileFindData_t;

// A pack file handle - essentially represents a file inside the pack file.  
// Note, there is no provision for compression here at the current time.
class CPackFileHandle
{
public:
	inline CPackFileHandle( CPackFile* pOwner, int64 nBase, unsigned int nLength, unsigned int nIndex = -1, unsigned int nFilePointer = 0 );
	inline ~CPackFileHandle();

	int				Read( void* pBuffer, int nDestSize, int nBytes );
	int				Seek( int nOffset, int nWhence );
	int				Tell() { return m_nFilePointer; }
	int				Size() { return m_nLength; }

	inline void		SetBufferSize( int nBytes );
	inline int		GetSectorSize();
	inline int64	AbsoluteBaseOffset();

protected:
	int64			m_nBase;			// Base offset of the file inside the pack file.
	unsigned int	m_nFilePointer;		// Current seek pointer (0 based from the beginning of the file).
	CPackFile*		m_pOwner;			// Pack file that owns this handle
	unsigned int	m_nLength;			// Length of this file.
	unsigned int	m_nIndex;			// Index into the pack's directory table
};

//-----------------------------------------------------------------------------

class CPackFile : public CRefCounted<CRefCountServiceMT>
{		
public:

	inline CPackFile();
	inline virtual ~CPackFile();

	// The means by which you open files:
	virtual CFileHandle *OpenFile( const char *pFileName, const char *pOptions = "rb" );

	// The two functions a pack file must provide
	virtual bool Prepare( int64 fileLen = -1, int64 nFileOfs = 0 ) = 0;
	virtual bool FindFile( const char *pFilename,  int &nIndex, int64 &nPosition, int &nLength ) = 0;

	// Used by FindFirst and FindNext
	virtual bool FindFirst( CBaseFileSystem::FindData_t *pFindData ) { return false; }
	virtual bool FindNext( CBaseFileSystem::FindData_t *pFindData ) { return false; }

	// This is the core IO routine for reading anything from a pack file, everything should go through here at some point
	virtual int ReadFromPack( int nIndex, void* buffer, int nDestBytes, int nBytes, int64 nOffset );
	
	// Returns the filename for a given file in the pack. Returns true if a filename is found, otherwise buffer is filled with "unknown"
	virtual bool IndexToFilename( int nIndex, char* buffer, int nBufferSize ) = 0;

	inline int GetSectorSize();

	virtual void SetupPreloadData() {}
	virtual void DiscardPreloadData() {}
	virtual int64 GetPackFileBaseOffset() = 0;

	// Note: threading model for pack files assumes that data
	// is segmented into pack files that aggregate files
	// meant to be read in one thread. Performance characteristics
	// tuned for that case
	CThreadFastMutex	m_mutex;

	// Path management:
	void SetPath( const CUtlSymbol &path ) { m_Path = path; }
	const CUtlSymbol& GetPath() const	{ Assert( m_Path != UTL_INVAL_SYMBOL ); return m_Path; }
	CUtlSymbol			m_Path;

	// possibly embedded pack
	int64				m_nBaseOffset;

	CUtlString			m_PackName;

	bool				m_bIsMapPath;
	bool				m_bIsVPK;
	long				m_lPackFileTime;

	int					m_refCount;
	int					m_nOpenFiles;

	FILE				*m_hPackFileHandle;	

protected:
	int64				m_FileLength;
	CBaseFileSystem		*m_fs;

	friend class		CPackFileHandle;
};

class CZipPackFile : public CPackFile
{
public:
	CZipPackFile( CBaseFileSystem* fs );
	~CZipPackFile();

	// Loads the pack file
	virtual bool Prepare( int64 fileLen = -1, int64 nFileOfs = 0 );
	virtual bool FindFile( const char *pFilename, int &nIndex, int64 &nOffset, int &nLength );
	virtual int  ReadFromPack( int nIndex, void* buffer, int nDestBytes, int nBytes, int64 nOffset  );

	int64 GetPackFileBaseOffset() { return m_nBaseOffset; }

	bool	IndexToFilename( int nIndex, char *pBuffer, int nBufferSize );

protected:
	#pragma pack(1)

	typedef struct
	{
		char name[ 112 ];
		int64 filepos;
		int64 filelen;
	} packfile64_t;

	typedef struct
	{
		char id[ 4 ];
		int64 dirofs;
		int64 dirlen;
	} packheader64_t;

	typedef struct
	{
		char id[ 8 ];
		int64 packheaderpos;
		int64 originalfilesize;
	} packappenededheader_t;

	#pragma pack()

	// A Pack file directory entry:
	class CPackFileEntry
	{
	public:
		unsigned int		m_nPosition;
		unsigned int		m_nLength;
		unsigned int		m_HashName;
		unsigned short		m_nPreloadIdx;
		unsigned short		pad;
#if !defined( _RETAIL )
		FileNameHandle_t	m_hDebugFilename;
#endif
	};

	class CPackFileLessFunc
	{
	public:
		bool Less( CPackFileEntry const& src1, CPackFileEntry const& src2, void *pCtx );
	};

	// Find a file inside a pack file:
	const CPackFileEntry* FindFile( const char* pFileName );

	// Entries to the individual files stored inside the pack file.
	CUtlSortVector< CPackFileEntry, CPackFileLessFunc > m_PackFiles;

	bool						GetOffsetAndLength( const char *FileName, int &nBaseIndex, int64 &nFileOffset, int &nLength );

	// Preload Support
	void						SetupPreloadData();	
	void						DiscardPreloadData();	
	ZIP_PreloadDirectoryEntry*	GetPreloadEntry( int nEntryIndex );

	int64						m_nPreloadSectionOffset;
	unsigned int				m_nPreloadSectionSize;
	ZIP_PreloadHeader			*m_pPreloadHeader;
	unsigned short*				m_pPreloadRemapTable;
	ZIP_PreloadDirectoryEntry	*m_pPreloadDirectory;
	void*						m_pPreloadData;
	CByteswap					m_swap;
};

//From VDC - https://developer.valvesoftware.com/wiki/VPK_File_Format#Header
struct VPKHeader_t
{
	// Version 1 Info
	uint32_t Signature = 0; // Should be 0x55aa1234
	uint32_t Version = 0;

	// The size, in bytes, of the directory tree
	uint32_t TreeSize = 0;

	// Version 2 Info

	// How many bytes of file content are stored in this VPK file (0 in CSGO)
	uint32_t FileDataSectionSize = 0;

	// The size, in bytes, of the section containing MD5 checksums for external archive content
	uint32_t ArchiveMD5SectionSize = 0;

	// The size, in bytes, of the section containing MD5 checksums for content in this file (should always be 48)
	uint32_t OtherMD5SectionSize = 0;

	// The size, in bytes, of the section containing the public key and signature. This is either 0 (CSGO & The Ship) or 296 (HL2, HL2:DM, HL2:EP1, HL2:EP2, HL2:LC, TF2, DOD:S & CS:S)
	uint32_t SignatureSectionSize = 0;

	size_t GetHeaderSize() const
	{
		if ( Version == 1 )
			return 12;
		else if ( Version == 2 )
			return 28;


		return 0;
	}
};

struct VPKDirectoryEntry_t
{
	uint32_t CRC; // A 32bit CRC of the file's data.
	uint16_t PreloadBytes; // The number of bytes contained in the index file.

								 // A zero based index of the archive this file's data is contained in.
								 // If 0x7fff, the data follows the directory.
	uint16_t ArchiveIndex;

	// If ArchiveIndex is 0x7fff, the offset of the file data relative to the end of the directory (see the header for more details).
	// Otherwise, the offset of the data from the start of the specified archive.
	uint32_t EntryOffset;

	// If zero, the entire file is stored in the preload data.
	// Otherwise, the number of bytes stored starting at EntryOffset.
	uint32_t EntryLength;

	uint16_t Terminator; //This is always = 0xffff;
};

struct VPKFileEntry_t
{
	~VPKFileEntry_t()
	{
		delete[] pszFileExtension;
	}

	VPKDirectoryEntry_t entry;
	char szFullFilePath[ MAX_FILEPATH ]; // Fully formed path of file inside VPK
	char *pszFileExtension = nullptr; // File extension
	int index; // Index into CVPKFile's CUtlVector of VPKFileEntry_t*
	CUtlMemory< byte > PreloadData;
};

class CVPKFile : public CPackFile
{
public:
	CVPKFile( CBaseFileSystem* fs, bool bVolumes, VPKHeader_t vpkheader, const char *pszBasePath );
	~CVPKFile();

	// Loads the pack file
	virtual bool Prepare( int64 fileLen = -1, int64 nFileOfs = 0 ) override;
	virtual bool FindFile( const char *pFilename, int &nIndex, int64 &nOffset, int &nLength ) override;
	virtual bool FindFirst( CBaseFileSystem::FindData_t *pFindData ) override;
	virtual bool FindNext( CBaseFileSystem::FindData_t *pFindData ) override;
	virtual int  ReadFromPack( int nIndex, void* buffer, int nDestBytes, int nBytes, int64 nOffset  ) override;

	int64 GetPackFileBaseOffset() override { return m_nBaseOffset; }

	bool IndexToFilename( int nIndex, char *pBuffer, int nBufferSize ) override;

	inline bool	UsesVolumes()	{ return m_bVolumes; }

private:

	// Reads NULL terminated string from archive (from VPK's directory file)
	void ReadString( CUtlVector< char > &buffer, FILE *pArchiveFile );

	const bool m_bVolumes;
	char m_szBasePath[ MAX_PATH ]; // VPK file path without _dir suffix, and without .vpk extension
	const VPKHeader_t m_vpkHeader;
	CUtlVector< VPKFileEntry_t* > m_pFileEntries;
	CUtlVector< FILE* > m_hArchiveHandles;

protected:

	CUtlStringMap< CUtlVector< const VPKFileEntry_t* > > m_PathMap; // Maps base path to list of file entries
	CUtlStringMap< const VPKFileEntry_t* > m_FileMap; // Maps fully formed file path to a particular file entry
};

// Pack file handle implementation:
                 
inline CPackFileHandle::CPackFileHandle( CPackFile* pOwner, int64 nBase, unsigned int nLength, unsigned int nIndex, unsigned int nFilePointer )
{
	m_pOwner = pOwner;
	m_nBase = nBase;
	m_nLength = nLength;
	m_nIndex = nIndex;
	m_nFilePointer = nFilePointer;
	pOwner->AddRef();
}

inline CPackFileHandle::~CPackFileHandle()
{
	m_pOwner->m_mutex.Lock();
	--m_pOwner->m_nOpenFiles;
	if ( m_pOwner->m_nOpenFiles == 0 && m_pOwner->m_bIsMapPath )
	{
		m_pOwner->m_fs->Trace_FClose( m_pOwner->m_hPackFileHandle );
		m_pOwner->m_hPackFileHandle = NULL;
	}
	m_pOwner->Release();
	m_pOwner->m_mutex.Unlock();
}

inline void CPackFileHandle::SetBufferSize( int nBytes ) 
{
	m_pOwner->m_fs->FS_setbufsize( m_pOwner->m_hPackFileHandle, nBytes );
}

inline int CPackFileHandle::GetSectorSize() 
{ 
	return m_pOwner->GetSectorSize(); 
}

inline int64 CPackFileHandle::AbsoluteBaseOffset() 
{ 
	return m_pOwner->GetPackFileBaseOffset() + m_nBase;
}

// Pack file implementation:
inline CPackFile::CPackFile()
{
	m_FileLength = 0;
	m_hPackFileHandle = NULL;
	m_fs = NULL;
	m_nBaseOffset = 0;
	m_bIsMapPath = false;
	m_lPackFileTime = 0L;
	m_refCount = 0;
	m_nOpenFiles = 0;
}

inline CPackFile::~CPackFile()
{
	if ( m_nOpenFiles )
	{
		Error( "Closing pack file with open files!\n" );
	}

	if ( m_hPackFileHandle )
	{
		m_fs->Trace_FClose( m_hPackFileHandle );
		m_hPackFileHandle = NULL;
	}

	m_fs->m_ZipFiles.FindAndRemove( this );
}


inline int CPackFile::GetSectorSize()
{
	if ( m_hPackFileHandle )
	{
		return m_fs->FS_GetSectorSize( m_hPackFileHandle );
	}
	else
	{
		return -1;
	}
}

#endif // PACKFILE_H