#include "btParallelCollisionDispatcher.h"

#include "BulletCollision/BroadphaseCollision/btOverlappingPairCache.h"
#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"

class btProcessOverlapTask : public btIThreadTask {
	public:
		btProcessOverlapTask(btBroadphasePair &pair, const btDispatcherInfo *info, btParallelCollisionDispatcher *pDispatcher): m_pair(pair) {
			m_pDispatcher = pDispatcher;
			m_pInfo = info;
		}

		void run() {
			btCollisionObject *obj0 = (btCollisionObject *)m_pair.m_pProxy0->m_clientObject;
			btCollisionObject *obj1 = (btCollisionObject *)m_pair.m_pProxy1->m_clientObject;

			// Check for invalid numbers being passed through
			btAssert(obj0->getWorldTransform() == obj0->getWorldTransform());
			btAssert(obj1->getWorldTransform() == obj1->getWorldTransform());

			btCollisionObjectWrapper obj0Wrap(0, obj0->getCollisionShape(), obj0, obj0->getWorldTransform(), -1, -1);
			btCollisionObjectWrapper obj1Wrap(0, obj1->getCollisionShape(), obj1, obj1->getWorldTransform(), -1, -1);

			if (!m_pair.m_algorithm) {
				m_pair.m_algorithm = m_pDispatcher->findAlgorithm(&obj0Wrap, &obj1Wrap);
			}

			if (m_pair.m_algorithm) {
				btManifoldResult contactPointResult(&obj0Wrap, &obj1Wrap);
				
				if (m_pInfo->m_dispatchFunc == btDispatcherInfo::DISPATCH_DISCRETE) {
					m_pair.m_algorithm->processCollision(&obj0Wrap, &obj1Wrap, *m_pInfo, &contactPointResult);
				} else {
					// Continuous dispatch
					m_pair.m_algorithm->calculateTimeOfImpact(obj0, obj1, *m_pInfo, &contactPointResult);
				}
			}
		}

		// Called after run() to destroy this task.
		void destroy() {
			m_pDispatcher->freeTask(this);
		}

	private:
		btBroadphasePair &m_pair;
		const btDispatcherInfo *m_pInfo;
		btParallelCollisionDispatcher *m_pDispatcher;
};

btParallelCollisionDispatcher::btParallelCollisionDispatcher(btCollisionConfiguration *pConfiguration, btThreadPool *pThreadPool) : btCollisionDispatcher(pConfiguration) {
	m_pThreadPool = pThreadPool;

	void *taskPoolMem = btAlloc(sizeof(btPoolAllocator));
	m_pTaskPool = new(taskPoolMem) btPoolAllocator(sizeof(btProcessOverlapTask), 4096);

	m_pPoolCritSect = btCreateCriticalSection();
	m_pAlgoPoolSect = btCreateCriticalSection();
}

btParallelCollisionDispatcher::~btParallelCollisionDispatcher() {
	m_pTaskPool->~btPoolAllocator();
	btFree(m_pTaskPool);

	btDeleteCriticalSection(m_pPoolCritSect);
	btDeleteCriticalSection(m_pAlgoPoolSect);
}

btPersistentManifold *btParallelCollisionDispatcher::getNewManifold(const btCollisionObject *ob0, const btCollisionObject *ob1) {
	m_pPoolCritSect->lock();
	btPersistentManifold *ret = btCollisionDispatcher::getNewManifold(ob0, ob1);
	m_pPoolCritSect->unlock();

	return ret;
}

void btParallelCollisionDispatcher::releaseManifold(btPersistentManifold *pManifold) {
	m_pPoolCritSect->lock();
	btCollisionDispatcher::releaseManifold(pManifold);
	m_pPoolCritSect->unlock();
}

void *btParallelCollisionDispatcher::allocateCollisionAlgorithm(int size) {
	m_pAlgoPoolSect->lock();
	void *ret = btCollisionDispatcher::allocateCollisionAlgorithm(size);
	m_pAlgoPoolSect->unlock();

	return ret;
}

void btParallelCollisionDispatcher::freeCollisionAlgorithm(void *ptr) {
	m_pAlgoPoolSect->lock();
	btCollisionDispatcher::freeCollisionAlgorithm(ptr);
	m_pAlgoPoolSect->unlock();
}

void *btParallelCollisionDispatcher::allocateTask(int size) {
	void *mem = NULL;
	if (m_pTaskPool->getFreeCount() > 0) {
		mem = m_pTaskPool->allocate(size);
	} else {
		// Fallback to dynamic alloc
		mem = btAlloc(size);
	}

	return mem;
}

void btParallelCollisionDispatcher::freeTask(void *ptr) {
	if (!m_pTaskPool->validPtr(ptr)) {
		btFree(ptr);
	} else {
		m_pTaskPool->freeMemory(ptr);
	}
}

class btCollisionPairCallback : public btOverlapCallback {
	public:
		btCollisionPairCallback(const btDispatcherInfo &info, btParallelCollisionDispatcher *pDispatcher) {
			m_pDispatcher = pDispatcher;
			m_pInfo = &info;
		}

		bool processOverlap(btBroadphasePair &pair) {
			btCollisionObject *obj0 = (btCollisionObject *)pair.m_pProxy0->m_clientObject;
			btCollisionObject *obj1 = (btCollisionObject *)pair.m_pProxy1->m_clientObject;
			btAssert(m_pDispatcher->needsCollision(obj0, obj1) == m_pDispatcher->needsCollision(obj1, obj0));

			if (m_pDispatcher->needsCollision(obj0, obj1)) {
				void *mem = m_pDispatcher->allocateTask(sizeof(btProcessOverlapTask));
				btProcessOverlapTask *task = new(mem) btProcessOverlapTask(pair, m_pInfo, m_pDispatcher);
				m_pDispatcher->getThreadPool()->addTask(task);
			}

			// Always return false (for whatever reason)
			return false;
		}

	private:
		const btDispatcherInfo *m_pInfo;
		btParallelCollisionDispatcher *m_pDispatcher;
};

void btParallelCollisionDispatcher::dispatchAllCollisionPairs(btOverlappingPairCache *pairCache, const btDispatcherInfo &dispatchInfo, btDispatcher *dispatcher) {
	btCollisionPairCallback cb(dispatchInfo, this);
	pairCache->processAllOverlappingPairs(&cb, dispatcher);

	m_pThreadPool->runTasks();
	m_pThreadPool->clearTasks();
}

btThreadPool *btParallelCollisionDispatcher::getThreadPool() {
	return m_pThreadPool;
}