#ifndef BT_PARALLELCOLLISIONDISPATCHER_H
#define BT_PARALLELCOLLISIONDISPATCHER_H

#include "BulletCollision/BroadphaseCollision/btDispatcher.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "LinearMath/btPoolAllocator.h"

#include "btThreadPool.h"
#include "btThreading.h"

class btParallelCollisionDispatcher : public btCollisionDispatcher {
	public:
		btParallelCollisionDispatcher(btCollisionConfiguration *pConfiguration, btThreadPool *pThreadPool);
		~btParallelCollisionDispatcher();

		virtual btPersistentManifold *getNewManifold(const btCollisionObject *ob1, const btCollisionObject *ob2);
		virtual void releaseManifold(btPersistentManifold *pManifold);

		virtual	void *allocateCollisionAlgorithm(int size);
		virtual	void freeCollisionAlgorithm(void *ptr);

		virtual void *allocateTask(int size);
		virtual void freeTask(void *ptr);

		virtual void dispatchAllCollisionPairs(btOverlappingPairCache *pairCache, const btDispatcherInfo &dispatchInfo, btDispatcher *dispatcher);

		btThreadPool *getThreadPool();

	private:
		btPoolAllocator *	m_pTaskPool;

		btICriticalSection *m_pPoolCritSect; // Manifold pool crit section
		btICriticalSection *m_pAlgoPoolSect; // Algorithm pool crit section
		btThreadPool *		m_pThreadPool;
};

#endif // BT_PARALLELCOLLISIONDISPATCHER_H