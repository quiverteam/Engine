//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//


#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <spawn.h>
#include <dirent.h>
#include <sys/wait.h>
#endif

#include "vstdlib/iprocessutils.h"
#include "tier1/utllinkedlist.h"
#include "tier1/utlstring.h"
#include "tier1/utlbuffer.h"
#include "tier1/tier1.h"

//-----------------------------------------------------------------------------
// At the moment, we can only run one process at a time 
//-----------------------------------------------------------------------------
class CProcessUtils : public CTier1AppSystem< IProcessUtils >
{
	typedef CTier1AppSystem< IProcessUtils > BaseClass;

public:
	CProcessUtils() : BaseClass( false ) {}

	// Inherited from IAppSystem
	virtual InitReturnVal_t Init();
	virtual void Shutdown();

	// Inherited from IProcessUtils
	virtual ProcessHandle_t StartProcess( const char *pCommandLine, bool bConnectStdPipes );
	virtual ProcessHandle_t StartProcess( int argc, const char **argv, bool bConnectStdPipes );
	virtual void CloseProcess( ProcessHandle_t hProcess );
	virtual void AbortProcess( ProcessHandle_t hProcess );
	virtual bool IsProcessComplete( ProcessHandle_t hProcess );
	virtual void WaitUntilProcessCompletes( ProcessHandle_t hProcess );
	virtual int SendProcessInput( ProcessHandle_t hProcess, char *pBuf, int nBufLen );
	virtual int GetProcessOutputSize( ProcessHandle_t hProcess );
	virtual int GetProcessOutput( ProcessHandle_t hProcess, char *pBuf, int nBufLen );
	virtual int GetProcessExitCode( ProcessHandle_t hProcess );

private:
	struct ProcessInfo_t
	{
		HANDLE m_hChildStdinRd;
		HANDLE m_hChildStdinWr;
		HANDLE m_hChildStdoutRd;
		HANDLE m_hChildStdoutWr;
		HANDLE m_hChildStderrWr;
		HANDLE m_hProcess;
		CUtlString m_CommandLine;
		CUtlBuffer m_ProcessOutput;
		unsigned int pid;
	};

	// Returns the last error that occurred
	char *GetErrorString( char *pBuf, int nBufLen );

	// creates the process, adds it to the list and writes the windows HANDLE into info.m_hProcess
	ProcessHandle_t CreateProcess( ProcessInfo_t &info, bool bConnectStdPipes );

	// Shuts down the process handle
	void ShutdownProcess( ProcessHandle_t hProcess );

	// Methods used to read	output back from a process
	int GetActualProcessOutputSize( ProcessHandle_t hProcess );
	int GetActualProcessOutput( ProcessHandle_t hProcess, char *pBuf, int nBufLen );

	CUtlFixedLinkedList< ProcessInfo_t >	m_Processes;
	ProcessHandle_t m_hCurrentProcess;
	bool m_bInitialized;
};


//-----------------------------------------------------------------------------
// Purpose: singleton accessor
//-----------------------------------------------------------------------------
static CProcessUtils s_ProcessUtils;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CProcessUtils, IProcessUtils, PROCESS_UTILS_INTERFACE_VERSION, s_ProcessUtils );


//-----------------------------------------------------------------------------
// Initialize, shutdown process system
//-----------------------------------------------------------------------------
InitReturnVal_t CProcessUtils::Init()
{
	InitReturnVal_t nRetVal = BaseClass::Init();
	if ( nRetVal != INIT_OK )
		return nRetVal;

	m_bInitialized = true;
	m_hCurrentProcess = PROCESS_HANDLE_INVALID;
	return INIT_OK;
}

void CProcessUtils::Shutdown()
{
	Assert( m_bInitialized );
	Assert( m_Processes.Count() == 0 );
	if ( m_Processes.Count() != 0 )
	{
		AbortProcess( m_hCurrentProcess );
	}
	m_bInitialized = false;
	return BaseClass::Shutdown();
}


//-----------------------------------------------------------------------------
// Returns the last error that occurred
//-----------------------------------------------------------------------------
char *CProcessUtils::GetErrorString( char *pBuf, int nBufLen )
{
#ifdef _WIN32
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, pBuf, nBufLen, NULL );
	char *p = strchr(pBuf, '\r');	// get rid of \r\n
	if(p) 
	{
		p[0] = 0;
	}
	return pBuf;
#else
	char* tmpstring = strerror(errno);
	if(tmpstring)
		strncpy(pBuf, tmpstring, nBufLen);
	else
		*pBuf = 0;
	return pBuf;
#endif
}


ProcessHandle_t CProcessUtils::CreateProcess( ProcessInfo_t &info, bool bConnectStdPipes )
{
#ifdef _WIN32
	STARTUPINFO si;
	memset(&si, 0, sizeof si);
	si.cb = sizeof(si);
	if ( bConnectStdPipes )
	{
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdInput = info.m_hChildStdinRd;
		si.hStdError = info.m_hChildStderrWr;
		si.hStdOutput = info.m_hChildStdoutWr;
	}

	PROCESS_INFORMATION pi;
	if ( ::CreateProcess( NULL, info.m_CommandLine.GetForModify(), NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &si, &pi ) )
	{
		info.m_hProcess = pi.hProcess;
		m_hCurrentProcess = m_Processes.AddToTail( info );
		return m_hCurrentProcess;
	}

	char buf[ 512 ];
	Warning( "Could not execute the command:\n   %s\n"
		"Windows gave the error message:\n   \"%s\"\n",
		info.m_CommandLine.Get(), GetErrorString( buf, sizeof(buf) ) );

	return PROCESS_HANDLE_INVALID;
#else

	/* Figure out the path by tokenizing the string */
	char* exe = strdup(info.m_CommandLine.Get());
	char* buf[256];
	char prev = 0, escaped = 0;
	int i, substr_count = 1, b = 0;

	/* Go through the command line and ensure spaces arent escaped */
	for(i = 0; exe[i]; i++)
	{
		if(isspace(exe[i]) && !escaped)
			exe[i] = '\0';
		if(exe[i] == '\0' && prev != '\0')
			++substr_count;
		/* Give a pointer if this is the start of a new substring */
		if(exe[i] && prev == '\0')
			buf[b++] = &exe[i];
		escaped = 0;
		if(exe[i] == '\\' && prev != '\\')
			escaped = 1;
		prev = exe[i];
	}

	/* Setup spawn attribs and file actions */
	posix_spawn_file_actions_t file_actions;
	posix_spawnattr_t spawnattr;
	posix_spawn_file_actions_init(&file_actions);
	posix_spawnattr_init(&spawnattr);

	/* Connect the pipes by duplicating them */
	if(info.m_hChildStdoutWr && bConnectStdPipes)
		posix_spawn_file_actions_adddup2(&file_actions, fileno((FILE*)info.m_hChildStdoutWr), fileno(stdout));
	if(info.m_hChildStderrWr && bConnectStdPipes)
		posix_spawn_file_actions_adddup2(&file_actions, fileno((FILE*)info.m_hChildStderrWr), fileno(stderr));
	if(info.m_hChildStdinRd && bConnectStdPipes)
		posix_spawn_file_actions_adddup2(&file_actions, fileno((FILE*)info.m_hChildStdinRd), fileno(stdin));

	pid_t newpid = 0;
	int ret = posix_spawn(&newpid, exe, &file_actions, &spawnattr, buf, environ);

	if(ret)
	{
		char msgbuf[512];
		Warning("Could not execute the command:\n    %s\n"
		  "Error message:\n    %s\n", info.m_CommandLine.Get(), GetErrorString(msgbuf, sizeof(msgbuf))
		);
		return PROCESS_HANDLE_INVALID;
	}
	info.pid = newpid;
	return m_Processes.AddToTail(info);
#endif
}

//-----------------------------------------------------------------------------
// Options for compilation
//-----------------------------------------------------------------------------
ProcessHandle_t CProcessUtils::StartProcess( const char *pCommandLine, bool bConnectStdPipes )
{
	Assert( m_bInitialized );

	// NOTE: For the moment, we can only run one process at a time
	// although in the future, I expect to have a process queue.
	if ( m_hCurrentProcess != PROCESS_HANDLE_INVALID )
	{
		WaitUntilProcessCompletes( m_hCurrentProcess ); 
	}

	ProcessInfo_t info;
	info.m_CommandLine = pCommandLine;

#ifdef _WIN32
	if ( !bConnectStdPipes )
	{
		info.m_hChildStderrWr = INVALID_HANDLE_VALUE;
		info.m_hChildStdinRd = info.m_hChildStdinWr = INVALID_HANDLE_VALUE;
		info.m_hChildStdoutRd = info.m_hChildStdoutWr = INVALID_HANDLE_VALUE;

		return CreateProcess( info, false );
	}

    SECURITY_ATTRIBUTES saAttr; 

    // Set the bInheritHandle flag so pipe handles are inherited.
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 
 
    // Create a pipe for the child's STDOUT. 
    if ( CreatePipe( &info.m_hChildStdoutRd, &info.m_hChildStdoutWr, &saAttr, 0 ) )
	{
		if ( CreatePipe( &info.m_hChildStdinRd, &info.m_hChildStdinWr, &saAttr, 0 ) )
		{
			if ( DuplicateHandle( GetCurrentProcess(), info.m_hChildStdoutWr, GetCurrentProcess(), 
				&info.m_hChildStderrWr, 0, TRUE, DUPLICATE_SAME_ACCESS ) )
			{
//				_setmode( info.m_hChildStdoutRd, _O_TEXT );
//				_setmode( info.m_hChildStdoutWr, _O_TEXT );
//				_setmode( info.m_hChildStderrWr, _O_TEXT );

				ProcessHandle_t hProcess = CreateProcess( info, true );
				if ( hProcess != PROCESS_HANDLE_INVALID )
					return hProcess;

				CloseHandle( info.m_hChildStderrWr );
			}
			CloseHandle( info.m_hChildStdinRd );
			CloseHandle( info.m_hChildStdinWr );
		}
		CloseHandle( info.m_hChildStdoutRd );
		CloseHandle( info.m_hChildStdoutWr );
	}
	return PROCESS_HANDLE_INVALID;
#else

    info.m_hChildStderrWr = (bConnectStdPipes ? stderr : NULL);
    info.m_hChildStdoutWr = (bConnectStdPipes ? stdout : NULL);
    info.m_hChildStdinRd = (bConnectStdPipes ? stdin : NULL);
    return CreateProcess(info, false);

#endif
}


//-----------------------------------------------------------------------------
// Start up a process
//-----------------------------------------------------------------------------
ProcessHandle_t CProcessUtils::StartProcess( int argc, const char **argv, bool bConnectStdPipes )
{
	CUtlString commandLine;
	for ( int i = 0; i < argc; ++i )
	{
		commandLine += argv[i];
		if ( i != argc-1 )
		{
			commandLine += " ";
		}
	}
	return StartProcess( commandLine.Get(), bConnectStdPipes );
}


//-----------------------------------------------------------------------------
// Shuts down the process handle
//-----------------------------------------------------------------------------
void CProcessUtils::ShutdownProcess( ProcessHandle_t hProcess )
{
	ProcessInfo_t& info = m_Processes[hProcess];

#ifdef _WIN32
	CloseHandle( info.m_hChildStderrWr );
	CloseHandle( info.m_hChildStdinRd );
	CloseHandle( info.m_hChildStdinWr );
	CloseHandle( info.m_hChildStdoutRd );
	CloseHandle( info.m_hChildStdoutWr );
#else
	kill(info.pid, SIGTERM);
#endif

	m_Processes.Remove( hProcess );
}


//-----------------------------------------------------------------------------
// Closes the process
//-----------------------------------------------------------------------------
void CProcessUtils::CloseProcess( ProcessHandle_t hProcess )
{
	Assert( m_bInitialized );
	if ( hProcess != PROCESS_HANDLE_INVALID )
	{
		WaitUntilProcessCompletes( hProcess );
		ShutdownProcess( hProcess );
	}
}


//-----------------------------------------------------------------------------
// Aborts the process
//-----------------------------------------------------------------------------
void CProcessUtils::AbortProcess( ProcessHandle_t hProcess )
{
	Assert( m_bInitialized );
	if ( hProcess != PROCESS_HANDLE_INVALID )
	{
		if ( !IsProcessComplete( hProcess ) )
		{
			ProcessInfo_t& info = m_Processes[hProcess];
#ifdef _WIN32
			TerminateProcess( info.m_hProcess, 1 );
#else
			kill(info.pid, SIGTERM);
#endif
		}
		ShutdownProcess( hProcess );
	}
}


//-----------------------------------------------------------------------------
// Returns true if the process is complete
//-----------------------------------------------------------------------------
bool CProcessUtils::IsProcessComplete( ProcessHandle_t hProcess )
{
	Assert( m_bInitialized );
	Assert( hProcess != PROCESS_HANDLE_INVALID );

	if ( m_hCurrentProcess != hProcess )
		return true;

#ifdef _WIN32
	HANDLE h = m_Processes[hProcess].m_hProcess;
	return ( WaitForSingleObject( h, 0 ) != WAIT_TIMEOUT );
#else
	int stat;
	waitpid(m_Processes[hProcess].pid, &stat, 0);
	return WIFEXITED(stat);
#endif
}


//-----------------------------------------------------------------------------
// Methods used to write input into a process
//-----------------------------------------------------------------------------
int CProcessUtils::SendProcessInput( ProcessHandle_t hProcess, char *pBuf, int nBufLen )
{
	// Unimplemented yet
	Assert( 0 );
	return 0;
}


//-----------------------------------------------------------------------------
// Methods used to read	output back from a process
//-----------------------------------------------------------------------------
int CProcessUtils::GetActualProcessOutputSize( ProcessHandle_t hProcess )
{
	Assert( hProcess != PROCESS_HANDLE_INVALID );

	ProcessInfo_t& info = m_Processes[ hProcess ];

#ifdef _WIN32
	if ( info.m_hChildStdoutRd == INVALID_HANDLE_VALUE )
		return 0;

	DWORD dwCount = 0;
	if ( !PeekNamedPipe( info.m_hChildStdoutRd, NULL, NULL, NULL, &dwCount, NULL ) )
	{
		char buf[ 512 ];
		Warning( "Could not read from pipe associated with command %s\n"
			"Windows gave the error message:\n   \"%s\"\n",
			info.m_CommandLine.Get(), GetErrorString( buf, sizeof(buf) ) );
		return 0;
	}

	// Add 1 for auto-NULL termination
	return ( dwCount > 0 ) ? (int)dwCount + 1 : 0;
#else
	return 0; // STUB for now
#endif
}

int CProcessUtils::GetActualProcessOutput( ProcessHandle_t hProcess, char *pBuf, int nBufLen )
{
	ProcessInfo_t& info = m_Processes[ hProcess ];

#ifdef _WIN32
	if ( info.m_hChildStdoutRd == INVALID_HANDLE_VALUE )
		return 0;

	DWORD dwCount = 0;
	DWORD dwRead = 0;

	// FIXME: Is there a way of making pipes be text mode so we don't get /n/rs back?
	char *pTempBuf = (char*)_alloca( nBufLen );
	if ( !PeekNamedPipe( info.m_hChildStdoutRd, NULL, NULL, NULL, &dwCount, NULL ) )
	{
		char buf[ 512 ];
		Warning( "Could not read from pipe associated with command %s\n"
			"Windows gave the error message:\n   \"%s\"\n",
			info.m_CommandLine.Get(), GetErrorString( buf, sizeof(buf) ) );
		return 0;
	}

	dwCount = min( dwCount, (DWORD)nBufLen - 1 );
	ReadFile( info.m_hChildStdoutRd, pTempBuf, dwCount, &dwRead, NULL);
	
	// Convert /n/r -> /n
	int nActualCountRead = 0;
	for ( unsigned int i = 0; i < dwRead; ++i )
	{
		char c = pTempBuf[i];
		if ( c == '\r' )
		{
			if ( ( i+1 < dwRead ) && ( pTempBuf[i+1] == '\n' ) )
			{
				pBuf[nActualCountRead++] = '\n';
				++i;
				continue;
			}
		}

		pBuf[nActualCountRead++] = c;
	}

	return nActualCountRead;
#else
	return 0; // STUB for now
#endif
}


int CProcessUtils::GetProcessOutputSize( ProcessHandle_t hProcess )
{
	Assert( m_bInitialized );
	if ( hProcess == PROCESS_HANDLE_INVALID )
		return 0;

	return GetActualProcessOutputSize( hProcess ) + m_Processes[hProcess].m_ProcessOutput.TellPut();
}


int CProcessUtils::GetProcessOutput( ProcessHandle_t hProcess, char *pBuf, int nBufLen )
{
	Assert( m_bInitialized );

	if ( hProcess == PROCESS_HANDLE_INVALID )
		return 0;

	ProcessInfo_t &info = m_Processes[hProcess];
	int nCachedBytes = info.m_ProcessOutput.TellPut();
	int nBytesRead = 0;
	if ( nCachedBytes )
	{
		nBytesRead = min( nBufLen-1, nCachedBytes );
		info.m_ProcessOutput.Get( pBuf, nBytesRead );
		pBuf[ nBytesRead ] = 0;
		nBufLen -= nBytesRead;
		pBuf += nBytesRead;
		if ( info.m_ProcessOutput.GetBytesRemaining() == 0 )
		{
			info.m_ProcessOutput.Purge();
		}

		if ( nBufLen <= 1 )
			return nBytesRead;
	}

	// Auto-NULL terminate
	int nActualCountRead = GetActualProcessOutput( hProcess, pBuf, nBufLen );
	pBuf[nActualCountRead] = 0;
	return nActualCountRead + nBytesRead + 1;
}


//-----------------------------------------------------------------------------
// Returns the exit code for the process. Doesn't work unless the process is complete
//-----------------------------------------------------------------------------
int CProcessUtils::GetProcessExitCode( ProcessHandle_t hProcess )
{
	Assert( m_bInitialized );
	ProcessInfo_t &info = m_Processes[hProcess];

#ifdef _WIN32
	DWORD nExitCode;
	BOOL bOk = GetExitCodeProcess( info.m_hProcess, &nExitCode );
	if ( !bOk || nExitCode == STILL_ACTIVE )
		return -1;
	return nExitCode;
#else
	int stat;
	waitpid(info.pid, &stat, 0);
	return WEXITSTATUS(stat);
#endif
}


//-----------------------------------------------------------------------------
// Waits until a process is complete
//-----------------------------------------------------------------------------
void CProcessUtils::WaitUntilProcessCompletes( ProcessHandle_t hProcess )
{
	Assert( m_bInitialized );

	// For the moment, we can only run one process at a time
	if ( ( hProcess == PROCESS_HANDLE_INVALID ) || ( m_hCurrentProcess != hProcess ) )
		return;

	ProcessInfo_t &info = m_Processes[ hProcess ];

#ifdef _WIN32
	if ( info.m_hChildStdoutRd == INVALID_HANDLE_VALUE )
	{
		WaitForSingleObject( info.m_hProcess, INFINITE );
	}
	else
	{
		// NOTE: The called process can block during writes to stderr + stdout
		// if the pipe buffer is empty. Therefore, waiting INFINITE is not
		// possible here. We must queue up messages received to allow the 
		// process to continue
		while ( WaitForSingleObject( info.m_hProcess, 100 ) == WAIT_TIMEOUT )
		{
			int nLen = GetActualProcessOutputSize( hProcess );
			if ( nLen > 0 )
			{
				int nPut = info.m_ProcessOutput.TellPut();
				info.m_ProcessOutput.EnsureCapacity( nPut + nLen );
				int nBytesRead = GetActualProcessOutput( hProcess, (char*)info.m_ProcessOutput.PeekPut(), nLen );
				info.m_ProcessOutput.SeekPut( CUtlBuffer::SEEK_HEAD, nPut + nBytesRead );
			}
		}
	}
#else
	int stat = 0;
	do
	{
		waitpid(info.pid, &stat, 0);
	} while(WIFEXITED(stat));
#endif
	m_hCurrentProcess = PROCESS_HANDLE_INVALID;
}
