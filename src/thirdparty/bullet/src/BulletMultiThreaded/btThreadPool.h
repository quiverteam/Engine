#ifndef BT_THREADPOOL_H
#define BT_THREADPOOL_H

#include "btThreading.h"
#include "LinearMath/btAlignedObjectArray.h"

class btIThreadTask {
	public:
		virtual void run() = 0;
		virtual void destroy() {}; // Destroys this task. Called on main thread after run (duh)
};

class btThreadPool;

struct btThreadPoolInfo {
	btIThread *pThread;
	btIEvent *pIdleEvent;
	btIEvent *pStartEvent;
	btThreadPool *pThreadPool;
	int threadId;

	// Task information
	btIThreadTask **pTaskArr;
	int numTasks;
};

class btThreadPool {
	public:
		btThreadPool();
		~btThreadPool();

		void startThreads(int numThreads);
		void stopThreads();
		void resizeThreads(int numThreads);

		int getNumThreads();

		void addTask(btIThreadTask *pTask);
		void clearTasks(); // Clear task queue

		int getNumTasks() {
			return m_taskArray.size();
		}

		// WARNING: Do not delete the elements here! This will be done in clearTasks()
		btIThreadTask *getTask(int i) {
			return m_taskArray[i];
		}

		void runTasks(); // Runs the threads until task pool is empty
		void waitIdle(); // Waits until thread pool is idle (no more tasks)

		// Internal functions (do not call these)

		void threadFunction(btThreadPoolInfo *pInfo);

	private:
		void beginThread(btThreadPoolInfo *pInfo);
		void endThread(btThreadPoolInfo *pInfo);

		btAlignedObjectArray<btThreadPoolInfo *> m_pThreadInfo;
		//btThreadPoolInfo **	m_pThreadInfo;
		int					m_numThreads;
		bool				m_bThreadsStarted;
		bool				m_bThreadsShouldExit;
		bool				m_bRunningTasks;

		btAlignedObjectArray<btIThreadTask *> m_taskArray; // FIXME: We don't need an aligned array.
};

#endif // BT_THREADPOOL_H