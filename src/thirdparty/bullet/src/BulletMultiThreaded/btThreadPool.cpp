#include "btThreadPool.h"

#include "LinearMath/btDefines.h"

#include <stdio.h>

// Entry-point function for threads. Calls back into btThreadPool.
static void ThreadFunc(void *pArg) {
	btThreadPoolInfo *pThreadInfo = (btThreadPoolInfo *)pArg;
	btThreadPool *pThreadPool = pThreadInfo->pThreadPool;

	pThreadPool->threadFunction(pThreadInfo);
}

btThreadPool::btThreadPool() {
	m_bThreadsStarted = false;
	m_bThreadsShouldExit = false;
	m_bRunningTasks = false;

	m_taskArray.reserve(100); // Reserve some space just to reduce some allocations
}

btThreadPool::~btThreadPool() {
	if (m_bThreadsStarted)
		stopThreads();
}

void btThreadPool::beginThread(btThreadPoolInfo *pInfo) {
	btIThread *pThread = btCreateThread();
	
	pInfo->pThread = pThread;
	pInfo->pIdleEvent = btCreateEvent(true);
	pInfo->pStartEvent = btCreateEvent(false);
	pInfo->pThreadPool = this;
	pInfo->pTaskArr = NULL;
	pInfo->numTasks = 0;
	pThread->setThreadFunc(ThreadFunc);

	// Set the name for debugging purposes
	char name[64];
	sprintf(name, "btThreadPool thread %d", pInfo->threadId);
	pThread->setThreadName(name);

	pThread->run(pInfo);
}

void btThreadPool::endThread(btThreadPoolInfo *pInfo) {
	m_bThreadsShouldExit = true; // Tell the thread to exit

	pInfo->pStartEvent->trigger(); // Start it (the thread will exit on its own)
	pInfo->pThread->waitForExit();

	btDeleteThread(pInfo->pThread);
	btDeleteEvent(pInfo->pIdleEvent);
	btDeleteEvent(pInfo->pStartEvent);

	m_bThreadsShouldExit = false;
}

void btThreadPool::startThreads(int numThreads) {
	btAssert(numThreads > 0);
	btAssert(!m_bThreadsStarted);

	m_bThreadsStarted = true;
	m_numThreads = numThreads;

	m_pThreadInfo.resize(numThreads);

	for (int i = 0; i < numThreads; i++) {
		m_pThreadInfo[i] = (btThreadPoolInfo *)btAlloc(sizeof(btThreadPoolInfo));
		btThreadPoolInfo *pInfo = m_pThreadInfo[i];
		pInfo->threadId = i;

		beginThread(pInfo);
	}
}

void btThreadPool::stopThreads() {
	btAssert(m_bThreadsStarted);
	btAssert(!m_bRunningTasks); // Don't modify this if we're running tasks!!!

	m_bThreadsStarted = false;

	for (int i = 0; i < m_numThreads; i++) {
		endThread(m_pThreadInfo[i]);
		btFree(m_pThreadInfo[i]);
	}
}

void btThreadPool::resizeThreads(int numThreads) {
	if (numThreads == m_numThreads) return;
	btAssert(numThreads > 0); // Don't allow the user to resize the thread count to below 0 or 0 - use stopThreads instead

	btAssert(!m_bRunningTasks); // Don't modify this if we're running tasks!!!

	if (numThreads < m_numThreads) {
		for (int i = m_numThreads - 1; i >= numThreads; i--) {
			endThread(m_pThreadInfo[i]);
			btFree(m_pThreadInfo[i]);

			m_pThreadInfo.pop_back();
		}
	} else {
		// More threads!
		m_pThreadInfo.resize(numThreads); // FYI: This doesn't actually change the thread info location since threads still need it!
		
		for (int i = m_numThreads; i < numThreads; i++) {
			m_pThreadInfo[i] = (btThreadPoolInfo *)btAlloc(sizeof(btThreadPoolInfo));
			btThreadPoolInfo *pInfo = m_pThreadInfo[i];
			pInfo->threadId = i;

			beginThread(pInfo);
		}
	}
	
	m_numThreads = numThreads;
}

int btThreadPool::getNumThreads() {
	return m_numThreads;
}

void btThreadPool::addTask(btIThreadTask *pTask) {
	btAssert(!m_bRunningTasks); // Don't modify this if we're running tasks!!!

	m_taskArray.push_back(pTask);
}

void btThreadPool::clearTasks() {
	btAssert(!m_bRunningTasks); // Don't modify this if we're running tasks!!!

	for (int i = 0; i < m_taskArray.size(); i++) {
		m_taskArray[i]->destroy(); // Allow the user to deallocate the task or whatever
	}

	m_taskArray.resize(0);
}

void btThreadPool::runTasks() {
	btAssert(m_numThreads != 0);
	if (m_taskArray.size() == 0) return;

	btAssert(!m_bRunningTasks); // This class cannot be used recursively!
	m_bRunningTasks = true;

	if (m_taskArray.size() >= m_numThreads) {
		int tasksPerThread = m_taskArray.size() / m_numThreads;
		int remainder = m_taskArray.size() % m_numThreads;
		int curTask = 0;

		for (int i = 0; i < m_numThreads; i++) {
			m_pThreadInfo[i]->pTaskArr = &m_taskArray[curTask];
			m_pThreadInfo[i]->numTasks = tasksPerThread + (remainder ? 1 : 0);
			curTask += m_pThreadInfo[i]->numTasks;

			if (remainder > 0) remainder--;
		}

		// Start the threads!
		for (int i = 0; i < m_numThreads; i++) {
			m_pThreadInfo[i]->pIdleEvent->reset(); // Reset the idle event
			m_pThreadInfo[i]->pStartEvent->trigger(); // Start it
		}
	} else if (m_taskArray.size() >= m_numThreads / 2) {
		// Rare case where tasks are less than num threads
		// Probably better to just run the task on the main thread if it's only one task, but whatever.
		for (int i = 0; i < m_taskArray.size(); i++) {
			m_pThreadInfo[i]->pTaskArr = &m_taskArray[i];
			m_pThreadInfo[i]->numTasks = 1;

			m_pThreadInfo[i]->pIdleEvent->reset(); // Reset the idle event
			m_pThreadInfo[i]->pStartEvent->trigger(); // Go!
		}
	} else {
		// Not enough tasks. Run sequentially!
		for (int i = 0; i < m_taskArray.size(); i++) {
			m_taskArray[i]->run();
		}
	}

	m_bRunningTasks = false;

	waitIdle();
}

void btThreadPool::waitIdle() {
	if (!m_bThreadsStarted) return;

	for (int i = 0; i < m_numThreads; i++) {
		m_pThreadInfo[i]->pIdleEvent->wait();
	}
}

void btThreadPool::threadFunction(btThreadPoolInfo *pInfo) {
	while (true) {
		pInfo->pIdleEvent->trigger(); // Trigger idle event (tell main thread we're done working)
		pInfo->pStartEvent->wait(); // Wait on start event (main thread telling us to work on something or quit)

		if (m_bThreadsShouldExit)
			break;

		// We have some work to do!
		if (pInfo->pTaskArr) {
			for (int i = 0; i < pInfo->numTasks; i++) {
				pInfo->pTaskArr[i]->run();
			}
			
			// Nullify this stuff for next time
			pInfo->pTaskArr = NULL;
			pInfo->numTasks = 0;
		}
	}
}