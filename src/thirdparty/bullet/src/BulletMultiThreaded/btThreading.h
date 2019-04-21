#ifndef BT_THREAD_H
#define BT_THREAD_H

typedef void (*btThreadFunc)(void *arg);

class btIThread {
	public:
		virtual ~btIThread() {};

		virtual void run(void *pArgument) = 0;
		virtual void waitForExit() = 0;

		virtual void setThreadFunc(btThreadFunc fn) = 0;
		virtual void setThreadArgument(void *pArg) = 0;
		virtual void setThreadName(const char *name) {}; // Debugging thread name (may not be available on some implementations)
};

btIThread *btCreateThread();
void btDeleteThread(btIThread *thread);

class btICriticalSection {
	public:
		virtual ~btICriticalSection() {};

		virtual void lock() = 0;
		virtual void unlock() = 0;
};

btICriticalSection *btCreateCriticalSection();
void btDeleteCriticalSection(btICriticalSection *pCritSection);

class btIEvent {
	public:
		virtual ~btIEvent() {};

		virtual void trigger() = 0; // Trigger this semaphore. Threads waiting on this will be released.
		virtual void reset() = 0; // Resets back to the untriggered state.
		virtual void wait() = 0; // Waits for semaphore to be triggered. Once triggered, this is set back to untriggered and function returns.
};

// TODO in posix: Actually use manual reset (always acts like manual reset = true right now)
// Beware: Linux with manual reset = false is UNTESTED!
btIEvent *btCreateEvent(bool bManualReset = false);
void btDeleteEvent(btIEvent *pSemaphore);

class btIConditionalVariable {
	public:
		virtual ~btIConditionalVariable() {};

		virtual void wakeAll() = 0; // Wake all threads waiting on this variable
		virtual void wake() = 0; // Wake one of the threads waiting on this variable

		// Unlocks a critical section and waits on this variable. When we're woken up,
		// locks the critical section again.
		virtual void wait(btICriticalSection *pCritSection) = 0;
};

btIConditionalVariable *btCreateConditionalVariable();
void btDeleteConditionalVariable(btIConditionalVariable *pVariable);

#endif // BT_THREAD_H