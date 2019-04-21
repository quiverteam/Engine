#ifndef BT_THREADPOOL_H
#define BT_THREADPOOL_H

#include "Threading.h"

class btIThreadTask {
	public:
		virtual void run() = 0;
};

class btThreadPool {
	public:
		btThreadPool(int numThreads);

	private:
		btIThread *m_pThreads;
		int m_numThreads;
};

#endif // BT_THREADPOOL_H