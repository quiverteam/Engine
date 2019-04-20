#ifndef PHYSICS_ENVIRONMENT_H
#define PHYSICS_ENVIRONMENT_H
#if defined(_MSC_VER) || (defined(__GNUC__) && __GNUC__ > 3)
	#pragma once
#endif

#include <vphysics/performance.h>
#include <vphysics/stats.h>

class btThreadPool;
class btCollisionConfiguration;
class btDispatcher;
class btBroadphaseInterface;
class btConstraintSolver;
class btSoftRigidDynamicsWorld;

class IPhysicsConstraintGroup;
class IPhysicsUserConstraint;
class IController;
class CDeleteQueue;
class CCollisionSolver;
class CObjectTracker;
class CCollisionEventListener;
class CPhysicsFluidController;
class CPhysicsDragController;
class CPhysicsEnvironment;
class CPhysicsConstraint;
class CPhysicsObject;
class CPhysicsSoftBody;

class CDebugDrawer;

// Temporary; remove later
class IPhysicsSoftBody;

class CCollisionSolver : public btOverlapFilterCallback {
	public:
		CCollisionSolver(CPhysicsEnvironment *pEnv) {m_pEnv = pEnv; m_pSolver = NULL;}
		void SetHandler(IPhysicsCollisionSolver *pSolver) {m_pSolver = pSolver;}
		virtual bool needBroadphaseCollision(btBroadphaseProxy *proxy0, btBroadphaseProxy *proxy1) const;

		bool NeedsCollision(CPhysicsObject *pObj0, CPhysicsObject *pObj1) const;
	private:
		IPhysicsCollisionSolver *m_pSolver;
		CPhysicsEnvironment *m_pEnv;
};

class CPhysicsEnvironment : public IPhysicsEnvironment32 {
public:
	CPhysicsEnvironment();
	~CPhysicsEnvironment();

	void									ChangeThreadCount(int newCount);

	// UNEXPOSED
	// Don't call this directly!
	static void								TickCallback(btDynamicsWorld *world, btScalar timestep);

	void									SetDebugOverlay(CreateInterfaceFn debugOverlayFactory);
	IVPhysicsDebugOverlay *					GetDebugOverlay();
	btIDebugDraw *							GetDebugDrawer();

	void									SetGravity(const Vector &gravityVector);
	void									GetGravity(Vector *pGravityVector) const;

	void									SetAirDensity(float density);
	float									GetAirDensity() const;
	
	IPhysicsObject *						CreatePolyObject(const CPhysCollide *pCollisionModel, int materialIndex, const Vector &position, const QAngle &angles, objectparams_t *pParams);
	IPhysicsObject *						CreatePolyObjectStatic(const CPhysCollide *pCollisionModel, int materialIndex, const Vector &position, const QAngle &angles, objectparams_t *pParams);
	// Deprecated. Use the collision interface instead.
	IPhysicsObject *						CreateSphereObject(float radius, int materialIndex, const Vector &position, const QAngle &angles, objectparams_t *pParams, bool isStatic = false);
	void									DestroyObject(IPhysicsObject *pObject);

	IPhysicsSoftBody *						CreateSoftBody();
	IPhysicsSoftBody *						CreateSoftBodyFromVertices(const Vector *vertices, int numVertices, const softbodyparams_t *pParams);
	IPhysicsSoftBody *						CreateSoftBodyRope(const Vector &pos, const Vector &end, int resolution, const softbodyparams_t *pParams);
	IPhysicsSoftBody *						CreateSoftBodyPatch(const Vector *corners, int resx, int resy, const softbodyparams_t *pParams);
	void									DestroySoftBody(IPhysicsSoftBody *pSoftBody);

	IPhysicsFluidController	*				CreateFluidController(IPhysicsObject *pFluidObject, fluidparams_t *pParams);
	void									DestroyFluidController(IPhysicsFluidController*);

	IPhysicsSpring	*						CreateSpring(IPhysicsObject *pObjectStart, IPhysicsObject *pObjectEnd, springparams_t *pParams);
	void									DestroySpring(IPhysicsSpring*);

	IPhysicsConstraint *					CreateRagdollConstraint(IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_ragdollparams_t &ragdoll);
	IPhysicsConstraint *					CreateHingeConstraint(IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_hingeparams_t &hinge);
	IPhysicsConstraint *					CreateFixedConstraint(IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_fixedparams_t &fixed);
	IPhysicsConstraint *					CreateSlidingConstraint(IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_slidingparams_t &sliding);
	IPhysicsConstraint *					CreateBallsocketConstraint(IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_ballsocketparams_t &ballsocket);
	IPhysicsConstraint *					CreatePulleyConstraint(IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_pulleyparams_t &pulley);
	IPhysicsConstraint *					CreateLengthConstraint(IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_lengthparams_t &length);
	IPhysicsConstraint *					CreateGearConstraint(IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_gearparams_t &gear);
	IPhysicsConstraint *					CreateUserConstraint(IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, IPhysicsUserConstraint *pConstraint);

	void									DestroyConstraint(IPhysicsConstraint *pConstraint);

	IPhysicsConstraintGroup *				CreateConstraintGroup(const constraint_groupparams_t &groupParams);
	void									DestroyConstraintGroup(IPhysicsConstraintGroup *pGroup);

	IPhysicsShadowController *				CreateShadowController(IPhysicsObject *pObject, bool allowTranslation, bool allowRotation);
	void									DestroyShadowController(IPhysicsShadowController *pController);

	IPhysicsPlayerController *				CreatePlayerController(IPhysicsObject *pObject);
	void									DestroyPlayerController(IPhysicsPlayerController *pController);

	IPhysicsMotionController *				CreateMotionController(IMotionEvent *pHandler);
	void									DestroyMotionController(IPhysicsMotionController *pController);

	IPhysicsVehicleController *				CreateVehicleController(IPhysicsObject *pVehicleBodyObject, const vehicleparams_t &params, unsigned int nVehicleType, IPhysicsGameTrace *pGameTrace);
	void									DestroyVehicleController(IPhysicsVehicleController *pController);

	void									SetCollisionSolver(IPhysicsCollisionSolver *pSolver);

	void									Simulate(float deltaTime);
	bool									IsInSimulation() const;

	float									GetSimulationTimestep() const;
	void									SetSimulationTimestep(float timestep);

	float									GetSimulationTime() const;
	void									ResetSimulationClock();

	float									GetNextFrameTime() const;

	void									SetCollisionEventHandler(IPhysicsCollisionEvent *pCollisionEvents);
	void									SetObjectEventHandler(IPhysicsObjectEvent *pObjectEvents);
	void									SetConstraintEventHandler(IPhysicsConstraintEvent *pConstraintEvents);

	void									SetQuickDelete(bool bQuick);

	int										GetActiveObjectCount() const;
	void									GetActiveObjects(IPhysicsObject **pOutputObjectList) const;

	const IPhysicsObject **					GetObjectList(int *pOutputObjectCount) const;
	int										GetObjectCount() const;
	bool									TransferObject(IPhysicsObject *pObject, IPhysicsEnvironment *pDestinationEnvironment);

	void									CleanupDeleteList();
	void									EnableDeleteQueue(bool enable);

	bool									Save(const physsaveparams_t &params);
	void									PreRestore(const physprerestoreparams_t &params);
	bool									Restore(const physrestoreparams_t &params);
	void									PostRestore();

	bool									IsCollisionModelUsed(CPhysCollide *pCollide) const;
	
	void									TraceRay(const Ray_t &ray, unsigned int fMask, IPhysicsTraceFilter *pTraceFilter, trace_t *pTrace);
	void									SweepCollideable(const CPhysCollide *pCollide, const Vector &vecAbsStart, const Vector &vecAbsEnd, const QAngle &vecAngles, unsigned int fMask, IPhysicsTraceFilter *pTraceFilter, trace_t *pTrace);
	void									SweepConvex(const CPhysConvex *pConvex, const Vector &vecAbsStart, const Vector &vecAbsEnd, const QAngle &vecAngles, unsigned int fMask, IPhysicsTraceFilter *pTraceFilter, trace_t *pTrace);

	void									GetPerformanceSettings(physics_performanceparams_t *pOutput) const;
	void									SetPerformanceSettings(const physics_performanceparams_t *pSettings);

	void									ReadStats(physics_stats_t *pOutput);
	void									ClearStats();

	unsigned int							GetObjectSerializeSize(IPhysicsObject *pObject) const;
	void									SerializeObjectToBuffer(IPhysicsObject *pObject, unsigned char *pBuffer, unsigned int bufferSize);
	IPhysicsObject *						UnserializeObjectFromBuffer(void *pGameData, unsigned char *pBuffer, unsigned int bufferSize, bool enableCollisions);

	void									EnableConstraintNotify(bool bEnable);
	void									DebugCheckContacts();
public:
	// Unexposed functions
	btSoftRigidDynamicsWorld *				GetBulletEnvironment();

	float									GetInvPSIScale();
	int										GetSimPSI() { return m_simPSI; }
	float									GetSubStepTime() { return m_subStepTime; }
	int										GetNumSubSteps() { return m_numSubSteps; }
	int										GetCurSubStep() { return m_curSubStep; }

	void									BulletTick(btScalar timeStep);
	CPhysicsDragController *				GetDragController();
	CCollisionSolver *						GetCollisionSolver();

	physics_performanceparams_t &			GetPerformanceSettings() { return m_perfparams; }
	const physics_performanceparams_t &		GetPerformanceSettings() const { return m_perfparams; }
	btVector3								GetMaxLinearVelocity() const;
	btVector3								GetMaxAngularVelocity() const;

	btThreadPool *							GetSharedThreadPool() const { return m_pSharedThreadPool; }

	void									DoCollisionEvents(float dt);

	void									HandleConstraintBroken(CPhysicsConstraint *pConstraint); // Call this if you're a constraint that was just disabled/broken.
	void									HandleFluidStartTouch(CPhysicsFluidController *pController, CPhysicsObject *pObject);
	void									HandleFluidEndTouch(CPhysicsFluidController *pController, CPhysicsObject *pObject);
	void									HandleObjectEnteredTrigger(CPhysicsObject *pTrigger, CPhysicsObject *pObject);
	void									HandleObjectExitedTrigger(CPhysicsObject *pTrigger, CPhysicsObject *pObject);

	btSoftBodyWorldInfo &					GetSoftBodyWorldInfo() { return m_softBodyWorldInfo; }

	// Soft body functions we'll expose at a later time...
private:
	bool									m_inSimulation;
	bool									m_bUseDeleteQueue;
	bool									m_bConstraintNotify;
	bool									m_deleteQuick;
	float									m_timestep;
	float									m_invPSIScale;
	int										m_simPSICurrent;
	int										m_simPSI;
	int										m_numSubSteps;
	int										m_curSubStep;
	float									m_subStepTime;

	btThreadPool *							m_pSharedThreadPool;
	btCollisionConfiguration *				m_pBulletConfiguration;
	btCollisionDispatcher *					m_pBulletDispatcher;
	btBroadphaseInterface *					m_pBulletBroadphase;
	btConstraintSolver *					m_pBulletSolver;
	btSoftRigidDynamicsWorld *				m_pBulletEnvironment;
	btOverlappingPairCallback *				m_pBulletGhostCallback;
	btSoftBodyWorldInfo						m_softBodyWorldInfo;

	CUtlVector<IPhysicsObject *>			m_objects;
	CUtlVector<IPhysicsObject *>			m_deadObjects;
	
	CUtlVector<IPhysicsSoftBody *>			m_softBodies;
	CUtlVector<CPhysicsFluidController *>	m_fluids;
	CUtlVector<IController *>				m_controllers;

	CCollisionEventListener *				m_pCollisionListener;
	CCollisionSolver *						m_pCollisionSolver;
	CDeleteQueue *							m_pDeleteQueue;
	CObjectTracker *						m_pObjectTracker;
	CPhysicsDragController *				m_pPhysicsDragController;
	IVPhysicsDebugOverlay *					m_pDebugOverlay;

	IPhysicsCollisionEvent *				m_pCollisionEvent;
	IPhysicsConstraintEvent *				m_pConstraintEvent;
	IPhysicsObjectEvent *					m_pObjectEvent;

	physics_performanceparams_t				m_perfparams;
	physics_stats_t							m_stats;

	CDebugDrawer *							m_debugdraw;
};

#endif // PHYSICS_ENVIRONMENT_H
