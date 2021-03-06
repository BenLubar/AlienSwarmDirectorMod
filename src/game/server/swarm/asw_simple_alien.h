#ifndef _DEFINED_ASW_SIMPLE_ALIEN_H
#define _DEFINED_ASW_SIMPLE_ALIEN_H
#ifdef _WIN32
#pragma once
#endif

#include "iasw_spawnable_npc.h"
#include "ai_hull.h"
#include "asw_shareddefs.h"

struct animevent_t;

struct CASW_Simple_Move_Failure
{
	trace_t trace;
	Vector vecStartPos;
	Vector vecTargetPos;
};

class CASW_Simple_Alien : public CBaseAnimating, public IASW_Spawnable_NPC
{
public:
	DECLARE_CLASS( CASW_Simple_Alien, CBaseAnimating  );	
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	CASW_Simple_Alien();
	virtual ~CASW_Simple_Alien();

	virtual void Spawn();
	virtual void Precache();
	Class_T		Classify( void ) { return (Class_T) CLASS_ASW_UNKNOWN; }

	// normal thinking
	virtual void AlienThink();
	virtual void WhatToDoNext(float delta);
	float m_fLastThinkTime;

	// enemy
	virtual bool CanSee(CBaseEntity *pEnt);
	virtual void LostEnemy();
	virtual Vector& GetChaseDestination(CBaseEntity *pEnt);
	virtual void FindNewEnemy();
	virtual CBaseEntity		*GetEnemy( void ) { return m_hEnemy; }
	virtual void SetEnemy(CBaseEntity *pEntity);
	EHANDLE m_hEnemy;

	// movement
	virtual Vector PickRandomDestination(float dist, Vector bias);
	virtual void SetMoveTarget(Vector &vecTarget);
	virtual float GetIdealSpeed() const;
	virtual float GetYawSpeed() const;
	virtual float GetIdealYaw();
	virtual void UpdateYaw(float delta);
	virtual float GetZigZagChaseDistance() const;
	virtual float GetFaceEnemyDistance() const;	
	virtual bool PerformMovement(float deltatime);
	virtual bool TryMove(const Vector &vecSrc, Vector &vecTarget, float deltatime, bool bStepMove = false);
	virtual bool ApplyGravity(Vector &vecTarget, float deltatime);
	virtual bool FailedMove();
	virtual void FinishedMovement();
	Vector m_vecMoveTarget;
	EHANDLE m_hMoveTarget;
	bool m_bMoving;
	float m_fArrivedTolerance;
	CASW_Simple_Move_Failure m_MoveFailure;
	bool m_bOnGround;
	float m_fFallSpeed;

	// state
	virtual void SetState(int iNewState);
	int m_iState;

	// attacking
	virtual void MeleeAttack( float distance, float damage, QAngle &viewPunch, Vector &shove );
	virtual CBaseEntity		*CheckTraceHullAttack( float flDist, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float forceScale = 1.0f, bool bDamageAnyNPC = false );
	virtual CBaseEntity		*CheckTraceHullAttack( const Vector &vStart, const Vector &vEnd, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float flForceScale = 1.0f, bool bDamageAnyNPC = false );
	virtual void ReachedEndOfSequence();
	virtual bool ShouldAttack();
	bool m_bAttacking;

	// sleeping
	virtual void UpdateSleeping();
	virtual void SetSleeping(bool bAsleep);
	virtual void SleepThink();
	bool m_bSleeping;

	// hull
	const Vector &		GetHullMins() const		{ return NAI_Hull::Mins(GetHullType()); }
	const Vector &		GetHullMaxs() const		{ return NAI_Hull::Maxs(GetHullType()); }
	Hull_t	GetHullType() const				{ return m_eHull; }
	void	SetHullType( Hull_t hullType )	{ m_eHull = hullType; }
	Hull_t		m_eHull;

	// health	
	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	virtual void ASWTraceBleed( float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType );
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	virtual int BloodColor() { return BLOOD_COLOR_GREEN; }
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual bool ShouldGib( const CTakeDamageInfo &info );
	virtual bool CorpseGib( const CTakeDamageInfo &info );
	virtual bool Event_Gibbed( const CTakeDamageInfo &info );
	void CorpseFade();
	virtual Vector CalcDeathForceVector( const CTakeDamageInfo &info );

	// animation
	virtual void HandleAnimEvent( animevent_t *pEvent );
	virtual void PlayRunningAnimation() { }
	virtual void PlayAttackingAnimation() { }
	virtual void PlayFallingAnimation() { }

	// sounds
	virtual void PainSound( const CTakeDamageInfo &info ) { }
	virtual void AlertSound() { }
	virtual void DeathSound( const CTakeDamageInfo &info ) { }
	virtual void AttackSound() { }
	float m_fNextPainSound;

	// =================================
	// IASW_Spawnable_NPC implementation
	// =================================
	void SetSpawner(CBaseEntity* spawner) { m_hSpawner = spawner; }
	EHANDLE m_hSpawner;

	virtual void SetAlienOrders(AlienOrder_t Orders, Vector vecOrderSpot, CBaseEntity* pOrderObject);
	AlienOrder_t GetAlienOrders();
	virtual void ClearAlienOrders();
	virtual void IgnoreMarines(bool bIgnoreMarines) { }

	virtual void CustomSettings(int iHealthBonus, float flHealthScale, float flSpeedScale, float flSizeScale, bool bFlammable, bool bFreezable, bool bTeslable, bool bFlinches)
	{
		m_iHealthBonus = iHealthBonus;
		m_flHealthScale = flHealthScale;
		m_flSpeedScale = flSpeedScale;
		m_flSizeScale = flSizeScale;
		m_bFlammable = bFlammable;
		m_bFreezable = bFreezable;
		m_bTeslable = bTeslable;
		m_bFlinches = bFlinches;

		SetModelScale(flSizeScale);
		SetHealthByDifficultyLevel();
	}
	virtual bool IsFlammable() { return m_bFlammable; }
	virtual bool IsFreezable() { return m_bFreezable; }
	virtual bool IsTeslable() { return m_bTeslable; }
	virtual bool IsFlincher() { return m_bFlinches; }
	int m_iHealthBonus;
	float m_flHealthScale;
	float m_flSpeedScale;
	float m_flSizeScale;
	bool m_bFlammable;
	bool m_bFreezable;
	bool m_bTeslable;
	bool m_bFlinches;

	AlienOrder_t m_AlienOrders;
	Vector m_vecAlienOrderSpot;
	EHANDLE m_AlienOrderObject;
	bool m_bIgnoreMarines;
	bool m_bFailedMoveTo;

	virtual void SetHealthByDifficultyLevel();

	// unused parts of the interface
	CAI_BaseNPC* GetNPC() { return NULL; }
	virtual bool CanStartBurrowed() { return false; }
	virtual void StartBurrowed() { }
	virtual void SetUnburrowActivity( string_t iszActivityName ) { }
	virtual void SetUnburrowIdleActivity( string_t iszActivityName ) { }
	virtual void MoveAside() { }
	virtual void ASW_Ignite( float flFlameLifetime, float flSize, CBaseEntity *pAttacker, CBaseEntity *pDamagingWeapon );
	virtual void ElectroStun( float flStunTime ) { }
	virtual void OnSwarmSensed(int iDistance) { }
	virtual void OnSwarmSenseEntity(CBaseEntity* pEnt) { }
	virtual bool AllowedToIgnite() { return m_bFlammable; }
	virtual void SetHoldoutAlien() { m_bHoldoutAlien = true; }
	virtual bool IsHoldoutAlien() { return m_bHoldoutAlien; }
	virtual void SetDirectorAlien() { m_bDirectorAlien = true; }
	virtual bool IsDirectorAlien() { return m_bDirectorAlien; }

	// debug
	virtual int DrawDebugTextOverlays();
	virtual void DrawDebugGeometryOverlays();

	bool m_bHoldoutAlien;
	bool m_bDirectorAlien;
};

enum {
	ASW_SIMPLE_ALIEN_IDLING,
	ASW_SIMPLE_ALIEN_ATTACKING,
	ASW_SIMPLE_ALIEN_DEAD,
};


#endif // _DEFINED_ASW_SIMPLE_ALIEN_H
