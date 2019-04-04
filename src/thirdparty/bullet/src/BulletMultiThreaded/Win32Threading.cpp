#include "PlatformDefinitions.h"
#include "btThreading.h"

#include "LinearMath/btScalar.h"
#include "LinearMath/btAlignedAllocator.h"

#ifdef USE_WIN32_THREADING

#include <Windows.h>
#include <new> // needed for placement new

#ifdef _MSC_VER
// Stolen from http://msdn.microsoft.com/en-us/library/xcb2z8hs%28v=vs.110%29.aspx
const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
   DWORD dwType; // Must be 0x1000.
   LPCSTR szName; // Pointer to name (in user addr space).
   DWORD dwThreadID; // Thread ID (-1=caller thread).
   DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

static void SetThreadName(DWORD dwThreadID, const char* threadName)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}
#endif // _MSC_VER

struct threadparams_t {
	btThreadFunc pFn;
	void *pArg;
};

static DWORD WINAPI ThreadFn(LPVOID lpArg) {
	if (!lpArg) btAssert(0);

	threadparams_t *params = (threadparams_t *)lpArg;
	btAssert(params->pFn);

	params->pFn(params->pArg);
	return 0;
}

class btWin32Thread : public btIThread {
	public:
		btWin32Thread() {
			m_hThread = INVALID_HANDLE_VALUE;
			m_threadParams.pFn = NULL;
			m_threadParams.pArg = NULL;
			m_pThreadName = NULL;
		}

		~btWin32Thread() {
			if (m_hThread != INVALID_HANDLE_VALUE) {
				CloseHandle(m_hThread);
				m_hThread = INVALID_HANDLE_VALUE;
			}
		}

		void run() {
			m_hThread = CreateThread(NULL, 0, ThreadFn, &m_threadParams, NORMAL_PRIORITY_CLASS, NULL);

			if (m_pThreadName)
				SetThreadName(GetThreadId(m_hThread), m_pThreadName);
		}

		void run(void *pArgument) {
			m_threadParams.pArg = pArgument;
			m_hThread = CreateThread(NULL, 0, ThreadFn, &m_threadParams, NORMAL_PRIORITY_CLASS, NULL);

			if (m_pThreadName)
				SetThreadName(GetThreadId(m_hThread), m_pThreadName);
		}

		void run(btThreadFunc pFn, void *pArgument) {
			m_threadParams.pFn = pFn;
			m_threadParams.pArg = pArgument;
			m_hThread = CreateThread(NULL, 0, ThreadFn, &m_threadParams, NORMAL_PRIORITY_CLASS, NULL);

			if (m_pThreadName)
				SetThreadName(GetThreadId(m_hThread), m_pThreadName);
		}

		void waitForExit() {
			WaitForSingleObject(m_hThread, INFINITE);
		}

		void setThreadFunc(btThreadFunc pFn) {
			m_threadParams.pFn = pFn;
		}

		void setThreadArgument(void *pArg) {
			m_threadParams.pArg = pArg;
		}

		void setThreadName(const char *name) {
			m_pThreadName = name;

			// Change it if the thread's already running
			if (m_hThread != INVALID_HANDLE_VALUE)
				SetThreadName(GetThreadId(m_hThread), name);
		}

	private:
		HANDLE m_hThread;

		threadparams_t m_threadParams;
		const char *m_pThreadName;
};

btIThread *btCreateThread() {
	void *mem = btAlloc(sizeof(btWin32Thread));
	return new(mem) btWin32Thread();
}

void btDeleteThread(btIThread *thread) {
	thread->~btIThread();
	btFree(thread);
}

class btWin32CriticalSection : public btICriticalSection {
	public:
		btWin32CriticalSection() {
			InitializeCriticalSection(&m_critSection);
		}

		~btWin32CriticalSection() {
			DeleteCriticalSection(&m_critSection);
		}

		bool trylock() {
			return TryEnterCriticalSection(&m_critSection);
		}

		void lock() {
			EnterCriticalSection(&m_critSection);
		}

		void unlock() {
			LeaveCriticalSection(&m_critSection);
		}

		CRITICAL_SECTION &getCritSection() {
			return m_critSection;
		}

	private:
		CRITICAL_SECTION m_critSection;
};

btICriticalSection *btCreateCriticalSection() {
	void *mem = btAlloc(sizeof(btWin32CriticalSection));
	return new(mem) btWin32CriticalSection();
}

void btDeleteCriticalSection(btICriticalSection *pCritSection) {
	pCritSection->~btICriticalSection();
	btFree(pCritSection);
}

class btWin32Event : public btIEvent {
	public:
		btWin32Event(bool bManualReset = false) {
			m_hEvent = CreateEvent(NULL, bManualReset, FALSE, NULL);
		}

		~btWin32Event() {
			CloseHandle(m_hEvent);
		}

		void trigger() {
			SetEvent(m_hEvent);
		}

		void reset() {
			ResetEvent(m_hEvent);
		}

		void wait() {
			WaitForSingleObject(m_hEvent, INFINITE);
		}

	private:
		HANDLE m_hEvent;
};

btIEvent *btCreateEvent(bool bManualReset) {
	void *mem = btAlloc(sizeof(btWin32Event));
	return new(mem) btWin32Event(bManualReset);
}

void btDeleteEvent(btIEvent *pEvent) {
	pEvent->~btIEvent();
	btFree(pEvent);
}

class btWin32ConditionalVariable : public btIConditionalVariable {
	public:
		btWin32ConditionalVariable() {
			InitializeConditionVariable(&m_variable);
		}

		~btWin32ConditionalVariable() {
			// How do we delete a conditional variable in win32?
		}

		virtual void wakeAll() {
			WakeAllConditionVariable(&m_variable);
		}

		virtual void wake() {
			WakeConditionVariable(&m_variable);
		}

		virtual void wait(btICriticalSection *pCritSection) {
			btWin32CriticalSection *winCritSection = (btWin32CriticalSection *)pCritSection;
			CRITICAL_SECTION &sec = winCritSection->getCritSection();
			SleepConditionVariableCS(&m_variable, &sec, INFINITE);
		}

	private:
		CONDITION_VARIABLE	m_variable;
};

btIConditionalVariable *btCreateConditionalVariable() {
	void *mem = btAlloc(sizeof(btWin32ConditionalVariable));
	return new(mem) btWin32ConditionalVariable();
}

void btDeleteConditionalVariable(btIConditionalVariable *pVariable) {
	pVariable->~btIConditionalVariable();
	btFree(pVariable);
}

#endif // USE_WIN32_THREADING