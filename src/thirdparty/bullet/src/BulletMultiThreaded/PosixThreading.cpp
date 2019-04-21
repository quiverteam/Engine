#include "PlatformDefinitions.h"
#include "btThreading.h"

#include "LinearMath/btScalar.h"
#include "LinearMath/btAlignedAllocator.h"

#include <new>

#ifdef USE_PTHREADS
#include <pthread.h>
#include <sys/prctl.h>

struct threadparams_t {
	btThreadFunc pFn;
	void *pArg;
	const char *pThreadName;
};

static void *ThreadFn(void *pArg) {
	if (!pArg) return NULL;

	threadparams_t *params = (threadparams_t *)pArg;
	btAssert(params->pFn);

	params->pFn(params->pArg);

	pthread_exit(0);
	return NULL;
}

class btPosixThread : public btIThread {
	public:
		btPosixThread() {
			m_bRunning = false;
			m_threadParams.pThreadName = NULL;
			m_threadParams.pArg = NULL;
			m_threadParams.pFn = NULL;

			m_threadId = 0;
		}

		~btPosixThread() {

		}

		void run() {
			int ret = pthread_create(&m_threadId, NULL, ThreadFn, &m_threadParams);
			if (ret != 0) {
				// Oh dear! The thread wasn't created.
				btDbgWarning("pthread_create failed with error %d\n", ret);
				return;
			}

			if (m_pThreadName)
				pthread_setname_np(m_threadId, m_pThreadName);

			m_bRunning = true;
		}

		void run(void *pArg) {
			m_threadParams.pArg = pArg;
			run();
		}

		void run(btThreadFunc pFn, void *pArg) {
			m_threadParams.pFn = pFn;
			run(pArg);
		}

		void waitForExit() {
			if (!m_bRunning) return;
			pthread_join(m_threadId, NULL);
		}

		void setThreadFunc(btThreadFunc pFn) {
			m_threadParams.pFn = pFn;
		}

		void setThreadArgument(void *pArg) {
			m_threadParams.pArg = pArg;
		}

		void setThreadName(const char *pName) {
			m_pThreadName = pName;
			
			if (m_bRunning)
				pthread_setname_np(m_threadId, pName);
		}
	private:
		pthread_t m_threadId;
		threadparams_t m_threadParams;

		const char *m_pThreadName;
		bool m_bRunning;
};

btIThread *btCreateThread() {
	void *mem = btAlloc(sizeof(btPosixThread));
	return new(mem) btPosixThread();
}

void btDeleteThread(btIThread *thread) {
	thread->~btIThread();
	btFree(thread);
}

class btPosixCriticalSection : public btICriticalSection {
	public:
		btPosixCriticalSection() {
			pthread_mutex_init(&m_mutex, NULL);
		}

		~btPosixCriticalSection() {
			pthread_mutex_destroy(&m_mutex);
		}

		pthread_mutex_t &getMutex() {
			return m_mutex;
		}

		bool trylock() {
			int ret = pthread_mutex_trylock(&m_mutex);
			if (ret == 0) return true;

			return false;
		}

		void lock() {
			pthread_mutex_lock(&m_mutex);
		}

		void unlock() {
			pthread_mutex_unlock(&m_mutex);
		}

	private:
		pthread_mutex_t m_mutex;
};

btICriticalSection *btCreateCriticalSection() {
	void *mem = btAlloc(sizeof(btPosixCriticalSection));
	return new(mem) btPosixCriticalSection();
}

void btDeleteCriticalSection(btICriticalSection *pCritSection) {
	pCritSection->~btICriticalSection();
	btFree(pCritSection);
}

class btPosixEvent : public btIEvent {
	public:
		btPosixEvent(bool bManualReset) {
			m_bTriggered = false;
			m_bManualReset = bManualReset;
			pthread_mutex_init(&m_mutex, NULL);
			pthread_cond_init(&m_condVar, NULL);
		}

		~btPosixEvent() {
			pthread_mutex_destroy(&m_mutex);
			pthread_cond_destroy(&m_condVar);
		}

		void trigger() {
			pthread_mutex_lock(&m_mutex);
			m_bTriggered = true;
			pthread_cond_broadcast(&m_condVar);
			pthread_mutex_unlock(&m_mutex);
		}

		void reset() {
			pthread_mutex_lock(&m_mutex);
			m_bTriggered = false;
			pthread_mutex_unlock(&m_mutex);
		}

		void wait() {
			pthread_mutex_lock(&m_mutex);
			while (!m_bTriggered)
				pthread_cond_wait(&m_condVar, &m_mutex);
			
			// Reset it if automatic reset is enabled
			if (!m_bManualReset)
				m_bTriggered = false;

			pthread_mutex_unlock(&m_mutex);
		}

	private:
		pthread_mutex_t m_mutex;
		pthread_cond_t m_condVar;
		bool m_bTriggered;

		bool m_bManualReset;
};

btIEvent *btCreateEvent(bool bManualReset) {
	void *mem = btAlloc(sizeof(btPosixEvent));
	return new(mem) btPosixEvent(bManualReset);
}

void btDeleteEvent(btIEvent *pEvent) {
	pEvent->~btIEvent();
	btFree(pEvent);
}

class btPosixConditionalVariable : public btIConditionalVariable {
	public:
		btPosixConditionalVariable() {
			pthread_cond_init(&m_variable, NULL);
		}

		~btPosixConditionalVariable() {
			pthread_cond_destroy(&m_variable);
		}

		virtual void wakeAll() {
			pthread_cond_broadcast(&m_variable);
		}

		virtual void wake() {
			pthread_cond_signal(&m_variable);
		}

		virtual void wait(btICriticalSection *pCritSection) {
			btPosixCriticalSection *pPosixSect = (btPosixCriticalSection *)pCritSection;
			pthread_mutex_t &mutex = pPosixSect->getMutex();
			pthread_cond_wait(&m_variable, &mutex);
		}

	private:
		pthread_cond_t	m_variable;
};

btIConditionalVariable *btCreateConditionalVariable() {
	void *mem = btAlloc(sizeof(btPosixConditionalVariable));
	return new(mem) btPosixConditionalVariable();
}

void btDeleteConditionalVariable(btIConditionalVariable *pVariable) {
	pVariable->~btIConditionalVariable();
	btFree(pVariable);
}
#endif
