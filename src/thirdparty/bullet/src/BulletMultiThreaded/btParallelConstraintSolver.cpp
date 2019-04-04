/*
   Copyright (C) 2010 Sony Computer Entertainment Inc.
   All rights reserved.

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

*/


#include "btParallelConstraintSolver.h"
#include "BulletDynamics/ConstraintSolver/btContactSolverInfo.h"
#include "BulletCollision/BroadphaseCollision/btDispatcher.h"
#include "LinearMath/btPoolAllocator.h"
#include "BulletCollision/NarrowPhaseCollision/btPersistentManifold.h"

#include "btThreadPool.h"
#include "btThreading.h"

#include "LinearMath/btDefines.h"
#include "LinearMath/btScalar.h"

#ifdef USE_SIMD
#include <emmintrin.h>
#define btVecSplat(x, e) _mm_shuffle_ps(x, x, _MM_SHUFFLE(e,e,e,e))
static inline __m128 btSimdDot3(__m128 vec0, __m128 vec1)
{
	__m128 result = _mm_mul_ps(vec0, vec1);
	return _mm_add_ps(btVecSplat(result, 0), _mm_add_ps(btVecSplat(result, 1), btVecSplat(result, 2)));
}
#endif//USE_SIMD

// Small class that is duplicated many times
ATTRIBUTE_ALIGNED16(class) btMiniSolverBody {
	public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btMiniSolverBody() {

	}

	btMiniSolverBody(const btSolverBody &body) {
		m_deltaLinearVelocity = body.getDeltaLinearVelocity();
		m_deltaAngularVelocity = body.getDeltaAngularVelocity();
		m_invMass = body.internalGetInvMass()[0]; // The 3 values should be the same (right?)
	}

	void writeBack(btSolverBody &body) {
		if (body.m_originalBody) {
			body.internalGetDeltaAngularVelocity() += m_deltaAngularVelocity * body.m_angularFactor;
			body.internalGetDeltaLinearVelocity() += m_deltaLinearVelocity * body.m_linearFactor;
		}
	}

	btVector3 &getDeltaLinearVelocity() {
		return m_deltaLinearVelocity;
	}

	btSimdScalar getInvMassSIMD() {
		return btSimdScalar(m_invMass);
	}

	btScalar getInvMass() {
		return m_invMass;
	}

	SIMD_FORCE_INLINE void internalApplyImpulse(const btVector3& linearComponent, const btVector3& angularComponent, const btScalar impulseMagnitude)
	{
		m_deltaLinearVelocity += linearComponent*impulseMagnitude;
		m_deltaAngularVelocity += angularComponent*(impulseMagnitude);
	}

	public:
	btVector3		m_deltaLinearVelocity;
	btVector3		m_deltaAngularVelocity;
	btScalar		m_invMass;
};

static void resolveSingleConstraintRowGeneric(btMiniSolverBody& body1, btMiniSolverBody& body2, const btSolverConstraint& c);
static void resolveSingleConstraintRowLowerLimit(btMiniSolverBody& body1, btMiniSolverBody& body2, const btSolverConstraint& c);

// Project Gauss Seidel or the equivalent Sequential Impulse
static void resolveSingleConstraintRowGenericSIMD(btMiniSolverBody& body1, btMiniSolverBody& body2, const btSolverConstraint& c)
{
#ifdef USE_SIMD
	__m128 cpAppliedImp = _mm_set1_ps(c.m_appliedImpulse);
	__m128	lowerLimit1 = _mm_set1_ps(c.m_lowerLimit);
	__m128	upperLimit1 = _mm_set1_ps(c.m_upperLimit);

	btSimdScalar deltaImpulse = _mm_sub_ps(_mm_set1_ps(c.m_rhs), _mm_mul_ps(cpAppliedImp, _mm_set1_ps(c.m_cfm)));
	__m128 deltaVel1Dotn = _mm_add_ps(btSimdDot3(c.m_contactNormal1.mVec128, body1.m_deltaLinearVelocity.mVec128), btSimdDot3(c.m_relpos1CrossNormal.mVec128, body1.m_deltaAngularVelocity.mVec128));
	__m128 deltaVel2Dotn = _mm_add_ps(btSimdDot3(c.m_contactNormal2.mVec128, body2.m_deltaLinearVelocity.mVec128), btSimdDot3(c.m_relpos2CrossNormal.mVec128, body2.m_deltaAngularVelocity.mVec128));

	deltaImpulse = _mm_sub_ps(deltaImpulse, _mm_mul_ps(deltaVel1Dotn, _mm_set1_ps(c.m_jacDiagABInv)));
	deltaImpulse = _mm_sub_ps(deltaImpulse, _mm_mul_ps(deltaVel2Dotn, _mm_set1_ps(c.m_jacDiagABInv)));

	btSimdScalar sum = _mm_add_ps(cpAppliedImp, deltaImpulse);
	btSimdScalar resultLowerLess, resultUpperLess;
	resultLowerLess = _mm_cmplt_ps(sum, lowerLimit1);
	resultUpperLess = _mm_cmplt_ps(sum, upperLimit1);
	__m128 lowMinApplied = _mm_sub_ps(lowerLimit1, cpAppliedImp);
	deltaImpulse = _mm_or_ps(_mm_and_ps(resultLowerLess, lowMinApplied), _mm_andnot_ps(resultLowerLess, deltaImpulse));
	c.m_appliedImpulse = _mm_or_ps(_mm_and_ps(resultLowerLess, lowerLimit1), _mm_andnot_ps(resultLowerLess, sum));
	__m128 upperMinApplied = _mm_sub_ps(upperLimit1, cpAppliedImp);
	deltaImpulse = _mm_or_ps(_mm_and_ps(resultUpperLess, deltaImpulse), _mm_andnot_ps(resultUpperLess, upperMinApplied));
	c.m_appliedImpulse = _mm_or_ps(_mm_and_ps(resultUpperLess, c.m_appliedImpulse), _mm_andnot_ps(resultUpperLess, upperLimit1));
	__m128	linearComponentA = _mm_mul_ps(c.m_contactNormal1.mVec128, body1.getInvMassSIMD());
	__m128	linearComponentB = _mm_mul_ps((c.m_contactNormal2).mVec128, body2.getInvMassSIMD());
	__m128 impulseMagnitude = deltaImpulse;

	body1.m_deltaLinearVelocity.mVec128 = _mm_add_ps(body1.m_deltaLinearVelocity.mVec128, _mm_mul_ps(linearComponentA, impulseMagnitude));
	body1.m_deltaAngularVelocity.mVec128 = _mm_add_ps(body1.m_deltaLinearVelocity.mVec128, _mm_mul_ps(c.m_angularComponentA.mVec128, impulseMagnitude));
	body2.m_deltaLinearVelocity.mVec128 = _mm_add_ps(body2.m_deltaLinearVelocity.mVec128, _mm_mul_ps(linearComponentB, impulseMagnitude));
	body2.m_deltaAngularVelocity.mVec128 = _mm_add_ps(body2.m_deltaLinearVelocity.mVec128, _mm_mul_ps(c.m_angularComponentB.mVec128, impulseMagnitude));
#else
	return resolveSingleConstraintRowGeneric(body1, body2, c);
#endif
}

// Project Gauss Seidel or the equivalent Sequential Impulse
static void resolveSingleConstraintRowGeneric(btMiniSolverBody& body1, btMiniSolverBody& body2, const btSolverConstraint& c)
{
	btScalar deltaImpulse = c.m_rhs - btScalar(c.m_appliedImpulse)*c.m_cfm;
	const btScalar deltaVel1Dotn = c.m_contactNormal1.dot(body1.m_deltaLinearVelocity) + c.m_relpos1CrossNormal.dot(body1.m_deltaAngularVelocity);
	const btScalar deltaVel2Dotn = c.m_contactNormal2.dot(body2.m_deltaLinearVelocity) + c.m_relpos2CrossNormal.dot(body2.m_deltaAngularVelocity);

	//	const btScalar delta_rel_vel	=	deltaVel1Dotn-deltaVel2Dotn;
	deltaImpulse -= deltaVel1Dotn*c.m_jacDiagABInv;
	deltaImpulse -= deltaVel2Dotn*c.m_jacDiagABInv;

	const btScalar sum = btScalar(c.m_appliedImpulse) + deltaImpulse;
	if (sum < c.m_lowerLimit) {
		deltaImpulse = c.m_lowerLimit - c.m_appliedImpulse;
		c.m_appliedImpulse = c.m_lowerLimit;
	} else if (sum > c.m_upperLimit) {
		deltaImpulse = c.m_upperLimit - c.m_appliedImpulse;
		c.m_appliedImpulse = c.m_upperLimit;
	} else {
		c.m_appliedImpulse = sum;
	}

	body1.internalApplyImpulse(c.m_contactNormal1*body1.getInvMass(), c.m_angularComponentA, deltaImpulse);
	body2.internalApplyImpulse(c.m_contactNormal2*body2.getInvMass(), c.m_angularComponentB, deltaImpulse);
}

static void resolveSingleConstraintRowLowerLimitSIMD(btMiniSolverBody& body1, btMiniSolverBody& body2, const btSolverConstraint& c)
{
#ifdef USE_SIMD
	__m128 cpAppliedImp = _mm_set1_ps(c.m_appliedImpulse);
	__m128	lowerLimit1 = _mm_set1_ps(c.m_lowerLimit);
	__m128	upperLimit1 = _mm_set1_ps(c.m_upperLimit);
	btSimdScalar deltaImpulse = _mm_sub_ps(_mm_set1_ps(c.m_rhs), _mm_mul_ps(_mm_set1_ps(c.m_appliedImpulse), _mm_set1_ps(c.m_cfm)));
	__m128 deltaVel1Dotn = _mm_add_ps(btSimdDot3(c.m_contactNormal1.mVec128, body1.m_deltaLinearVelocity.mVec128), btSimdDot3(c.m_relpos1CrossNormal.mVec128, body1.m_deltaAngularVelocity.mVec128));
	__m128 deltaVel2Dotn = _mm_add_ps(btSimdDot3(c.m_contactNormal2.mVec128, body2.m_deltaLinearVelocity.mVec128), btSimdDot3(c.m_relpos2CrossNormal.mVec128, body2.m_deltaAngularVelocity.mVec128));
	deltaImpulse = _mm_sub_ps(deltaImpulse, _mm_mul_ps(deltaVel1Dotn, _mm_set1_ps(c.m_jacDiagABInv)));
	deltaImpulse = _mm_sub_ps(deltaImpulse, _mm_mul_ps(deltaVel2Dotn, _mm_set1_ps(c.m_jacDiagABInv)));
	btSimdScalar sum = _mm_add_ps(cpAppliedImp, deltaImpulse);
	btSimdScalar resultLowerLess, resultUpperLess;
	resultLowerLess = _mm_cmplt_ps(sum, lowerLimit1);
	resultUpperLess = _mm_cmplt_ps(sum, upperLimit1);
	__m128 lowMinApplied = _mm_sub_ps(lowerLimit1, cpAppliedImp);
	deltaImpulse = _mm_or_ps(_mm_and_ps(resultLowerLess, lowMinApplied), _mm_andnot_ps(resultLowerLess, deltaImpulse));
	c.m_appliedImpulse = _mm_or_ps(_mm_and_ps(resultLowerLess, lowerLimit1), _mm_andnot_ps(resultLowerLess, sum));
	__m128	linearComponentA = _mm_mul_ps(c.m_contactNormal1.mVec128, body1.getInvMassSIMD());
	__m128	linearComponentB = _mm_mul_ps(c.m_contactNormal2.mVec128, body2.getInvMassSIMD());
	__m128 impulseMagnitude = deltaImpulse;
	body1.m_deltaLinearVelocity.mVec128 = _mm_add_ps(body1.m_deltaLinearVelocity.mVec128, _mm_mul_ps(linearComponentA, impulseMagnitude));
	body1.m_deltaAngularVelocity.mVec128 = _mm_add_ps(body1.m_deltaAngularVelocity.mVec128, _mm_mul_ps(c.m_angularComponentA.mVec128, impulseMagnitude));
	body2.m_deltaLinearVelocity.mVec128 = _mm_add_ps(body2.m_deltaLinearVelocity.mVec128, _mm_mul_ps(linearComponentB, impulseMagnitude));
	body2.m_deltaAngularVelocity.mVec128 = _mm_add_ps(body2.m_deltaAngularVelocity.mVec128, _mm_mul_ps(c.m_angularComponentB.mVec128, impulseMagnitude));
#else
	return resolveSingleConstraintRowLowerLimit(body1, body2, c);
#endif
}

static void resolveSingleConstraintRowLowerLimit(btMiniSolverBody& body1, btMiniSolverBody& body2, const btSolverConstraint& c)
{
	btScalar deltaImpulse = c.m_rhs - btScalar(c.m_appliedImpulse)*c.m_cfm;
	const btScalar deltaVel1Dotn = c.m_contactNormal1.dot(body1.m_deltaLinearVelocity) + c.m_relpos1CrossNormal.dot(body1.m_deltaAngularVelocity);
	const btScalar deltaVel2Dotn = c.m_contactNormal2.dot(body2.m_deltaLinearVelocity) + c.m_relpos2CrossNormal.dot(body2.m_deltaAngularVelocity);

	deltaImpulse -= deltaVel1Dotn*c.m_jacDiagABInv;
	deltaImpulse -= deltaVel2Dotn*c.m_jacDiagABInv;
	const btScalar sum = btScalar(c.m_appliedImpulse) + deltaImpulse;
	if (sum < c.m_lowerLimit) {
		deltaImpulse = c.m_lowerLimit - c.m_appliedImpulse;
		c.m_appliedImpulse = c.m_lowerLimit;
	} else {
		c.m_appliedImpulse = sum;
	}
	body1.internalApplyImpulse(c.m_contactNormal1*body1.getInvMass(), c.m_angularComponentA, deltaImpulse);
	body2.internalApplyImpulse(c.m_contactNormal2*body2.getInvMass(), c.m_angularComponentB, deltaImpulse);
}

class btSolveConstraintTask : public btIThreadTask {
	public:
		// Guaranteed that the constraint will only be available to us (threadsafe)
		btSolveConstraintTask(btParallelConstraintSolver *pSolver, int constraintType, btSolverBody &bodyA, btSolverBody &bodyB, btSolverConstraint *pConstraint, bool useSimd) {
			m_pSolver = pSolver;
			m_constraintType = constraintType;
			m_bodyA = bodyA;
			m_bodyB = bodyB;
			m_pConstraint = pConstraint;
			btAssert(pConstraint != NULL);

			m_useSimd = useSimd;
		}

		void run() {
			switch (m_constraintType) {
				// generic
				case 0: {
					if (m_useSimd) {
						resolveSingleConstraintRowGenericSIMD(m_bodyA, m_bodyB, *m_pConstraint);
					} else {
						resolveSingleConstraintRowGeneric(m_bodyA, m_bodyB, *m_pConstraint);
					}
					break;
				}

				// lower limit
				case 1: {
					if (m_useSimd) {
						resolveSingleConstraintRowLowerLimitSIMD(m_bodyA, m_bodyB, *m_pConstraint);
					} else {
						resolveSingleConstraintRowLowerLimit(m_bodyA, m_bodyB, *m_pConstraint);
					}
					break;
				}
			}
		}

		void destroy() {
			m_pSolver->freeTask(this);
		}

	public:
		btParallelConstraintSolver *m_pSolver;

		btMiniSolverBody m_bodyA;
		btMiniSolverBody m_bodyB;
		btSolverConstraint *m_pConstraint;

		bool m_useSimd;

		// 0=user, 
		int m_constraintType;
};

btParallelConstraintSolver::btParallelConstraintSolver(btThreadPool *pThreadPool) {
	m_pThreadPool = pThreadPool;

	void *taskPoolMem = btAlloc(sizeof(btPoolAllocator));
	m_pTaskPool = new(taskPoolMem)btPoolAllocator(sizeof(btSolveConstraintTask), 4096);
}

btParallelConstraintSolver::~btParallelConstraintSolver() {
	m_pTaskPool->~btPoolAllocator();
	btFree(m_pTaskPool);
}

void *btParallelConstraintSolver::allocateTask(int size) {
	void *ret = NULL;
	
	if (m_pTaskPool->getFreeCount() > 0) {
		ret = m_pTaskPool->allocate();
	} else {
		ret = btAlloc(size);
	}

	return ret;
}

void btParallelConstraintSolver::freeTask(void *ptr) {
	if (m_pTaskPool->validPtr(ptr)) {
		m_pTaskPool->freeMemory(ptr);
	} else {
		btFree(ptr);
	}
}

void btParallelConstraintSolver::solveSingleIterationParallel(int iteration, const btContactSolverInfo &infoGlobal) {
	int numNonContactPool = m_tmpSolverNonContactConstraintPool.size();
	int numConstraintPool = m_tmpSolverContactConstraintPool.size();
	int numFrictionPool = m_tmpSolverContactFrictionConstraintPool.size();

	// Resolve all joint constraints (user constraints)
	for (int i = 0; i < numNonContactPool; i++) {
		btSolverConstraint &constraint = m_tmpSolverNonContactConstraintPool[m_orderNonContactConstraintPool[i]];
		if (iteration < constraint.m_overrideNumSolverIterations) {
			btSolveConstraintTask *pTask = (btSolveConstraintTask *)allocateTask(sizeof(btSolveConstraintTask));
			pTask = new(pTask) btSolveConstraintTask(this, 0, m_tmpSolverBodyPool[constraint.m_solverBodyIdA], m_tmpSolverBodyPool[constraint.m_solverBodyIdB], &constraint, infoGlobal.m_solverMode & SOLVER_SIMD);
			m_pThreadPool->addTask(pTask);
		}
	}

	if (iteration < infoGlobal.m_numIterations) {
		//-------------------------
		// CONTACT CONSTRAINTS
		//-------------------------
		for (int i = 0; i < numConstraintPool; i++) {
			btSolverConstraint &solveManifold = m_tmpSolverContactConstraintPool[m_orderTmpConstraintPool[i]];

			btSolveConstraintTask *pTask = (btSolveConstraintTask *)allocateTask(sizeof(btSolveConstraintTask));
			pTask = new(pTask)btSolveConstraintTask(this, 1, m_tmpSolverBodyPool[solveManifold.m_solverBodyIdA], m_tmpSolverBodyPool[solveManifold.m_solverBodyIdB], &solveManifold, infoGlobal.m_solverMode & SOLVER_SIMD);
			m_pThreadPool->addTask(pTask);
		}
	}

	m_pThreadPool->runTasks();

	// Pull the data back
	for (int i = 0; i < m_pThreadPool->getNumTasks(); i++) {
		btSolveConstraintTask *pTask = (btSolveConstraintTask *)m_pThreadPool->getTask(i);
		pTask->m_bodyA.writeBack(m_tmpSolverBodyPool[pTask->m_pConstraint->m_solverBodyIdA]);
		pTask->m_bodyB.writeBack(m_tmpSolverBodyPool[pTask->m_pConstraint->m_solverBodyIdB]);
	}

	m_pThreadPool->clearTasks();
}

btScalar btParallelConstraintSolver::solveGroupCacheFriendlyIterations(btCollisionObject **bodies, int numBodies, btPersistentManifold **manifolds, int numManifolds, btTypedConstraint **constraints, int numConstraints, const btContactSolverInfo &infoGlobal, btIDebugDraw *debugDrawer) {
	// Resolve penetrations for contacts
	solveGroupCacheFriendlySplitImpulseIterations(bodies, numBodies, manifolds, numManifolds, constraints, numConstraints, infoGlobal, debugDrawer);

	// If the override is greater than the global num iterations, use it instead
	int maxIterations = m_maxOverrideNumSolverIterations > infoGlobal.m_numIterations ? m_maxOverrideNumSolverIterations : infoGlobal.m_numIterations;

	for (int iteration = 0; iteration < maxIterations; iteration++){
		solveSingleIterationParallel(iteration, infoGlobal);
	}

	return 0.f;
}

btScalar btParallelConstraintSolver::solveGroup(btCollisionObject **bodies, int numBodies, btPersistentManifold **manifolds, int numManifolds, btTypedConstraint **constraints, 
											int numConstraints, const btContactSolverInfo &infoGlobal, btIDebugDraw *debugDrawer, btDispatcher *dispatcher) {
	solveGroupCacheFriendlySetup(bodies, numBodies, manifolds, numManifolds, constraints, numConstraints, infoGlobal, debugDrawer);

	solveGroupCacheFriendlyIterations(bodies, numBodies, manifolds, numManifolds, constraints, numConstraints, infoGlobal, debugDrawer);

	solveGroupCacheFriendlyFinish(bodies, numBodies, infoGlobal);

	// Unused return value
	return btScalar(0);
}

void btParallelConstraintSolver::waitUntilFinished() {

}