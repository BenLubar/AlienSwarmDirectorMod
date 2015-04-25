#ifndef _INCLUDED_ASW_SPAWN_ENTITY_H
#define _INCLUDED_ASW_SPAWN_ENTITY_H
#ifdef _WIN32
#pragma once
#endif

#include "iasw_spawnable_npc.h"
#include "asw_spawn_manager.h"

// TODO: define this somewhere more centralized and use it from there.
#define ASW_DIFFICULTY_COUNT 5

DECLARE_AUTO_LIST(IASW_Spawn_Entity_List);

abstract_class CASW_Spawn_Entity : public CBaseEntity, public IASW_Spawn_Entity_List
{
public:
	DECLARE_CLASS(CASW_Spawn_Entity, CBaseEntity);
	DECLARE_DATADESC();

	CASW_Spawn_Entity();
	virtual ~CASW_Spawn_Entity();

	IMPLEMENT_AUTO_LIST_GET();

	virtual void Spawn();
	virtual void Precache();
	virtual void Think();
	virtual void DeathNotice(CBaseEntity *pVictim);
	virtual void SetUpAlien(IASW_Spawnable_NPC *pAlien) = 0;
	virtual bool CanSpawn(int nAlienType) = 0;
	void Enable();
	void Disable();

	// inputs
	inline void InputEnable(inputdata_t &inputdata) { Enable(); }
	inline void InputDisable(inputdata_t &inputdata) { Disable(); }
	inline void InputToggle(inputdata_t &inputdata) { if (m_bWaitingForInput) { Enable(); } else { Disable(); } }

protected:
	inline int GetAlienLimitIndex();
	virtual float CheckDistances();
	void SetUpNextSpawn();
	void DoSpawn(bool bIsDirector = false);

	friend class CASW_Director;

	string_t m_AlienName;
	bool m_bWaitingForInput;
	bool m_bSpawningEnabled;
	bool m_bNextThinkSpawns;
	bool m_bAllowLongThink;
	float m_flStartDistance;
	float m_flStopDistance;

	float m_flMinSpawnInterval;
	float m_flMaxSpawnInterval;

	// easy, normal, hard, insane, brutal
	int m_nMaxLiveAliens[ASW_DIFFICULTY_COUNT];
	int m_nMaxSpawnedAliens[ASW_DIFFICULTY_COUNT];
	int m_nMaxDirectorAliens[ASW_DIFFICULTY_COUNT];
	int m_nLiveAliens;
	int m_nLiveDirector;
	int m_nSpawnedAliens;
	int m_nDirectorAliens;

	float m_flSpawnChance[ASW_NUM_ALIEN_CLASSES];
	float m_flSpawnChanceTotal;

	float m_flMaxYaw;
	float m_flMaxForward;
	float m_flMaxSide;

	// Alien Swarm: SP spawner flags
	float m_flHealthScale;
	float m_flSpeedScale;
	float m_flSizeScale;
	bool m_bFlammable;
	bool m_bFreezable;
	bool m_bTeslable;
	bool m_bFlinches;

	// outputs
	COutputEvent m_OnSpawned;
	COutputEvent m_OnAnySpawned;
	COutputEvent m_OnDirectorSpawned;
	COutputEvent m_OnAllSpawned;
	COutputEvent m_OnAllSpawnedDead;
};

class CASW_Spawn_Offscreen : public CASW_Spawn_Entity
{
public:
	DECLARE_CLASS(CASW_Spawn_Offscreen, CASW_Spawn_Entity);
	DECLARE_DATADESC();

	CASW_Spawn_Offscreen();
	virtual ~CASW_Spawn_Offscreen();

	virtual void SetUpAlien(IASW_Spawnable_NPC *pAlien);
	virtual bool CanSpawn(int nAlienType);

protected:
	virtual float CheckDistances();

	bool m_bMoveToMarines;
	float m_flMinDistance;
};

class CASW_Spawn_Burrow : public CASW_Spawn_Entity
{
public:
	DECLARE_CLASS(CASW_Spawn_Burrow, CASW_Spawn_Entity);

	CASW_Spawn_Burrow();
	virtual ~CASW_Spawn_Burrow();

	virtual void SetUpAlien(IASW_Spawnable_NPC *pAlien);
	virtual bool CanSpawn(int nAlienType);
};

class CASW_Spawn_Hallway : public CASW_Spawn_Entity
{
public:
	DECLARE_CLASS(CASW_Spawn_Hallway, CASW_Spawn_Entity);

	CASW_Spawn_Hallway();
	virtual ~CASW_Spawn_Hallway();

	virtual void SetUpAlien(IASW_Spawnable_NPC *pAlien);
	virtual bool CanSpawn(int nAlienType);
};

class CASW_Spawn_Wall : public CASW_Spawn_Entity
{
public:
	DECLARE_CLASS(CASW_Spawn_Wall, CASW_Spawn_Entity);
	DECLARE_DATADESC();

	CASW_Spawn_Wall();
	virtual ~CASW_Spawn_Wall();

	virtual void SetUpAlien(IASW_Spawnable_NPC *pAlien);
	virtual bool CanSpawn(int nAlienType);

protected:
	bool m_bVentIsHigh;
};

class CASW_Spawn_Railing : public CASW_Spawn_Entity
{
public:
	DECLARE_CLASS(CASW_Spawn_Railing, CASW_Spawn_Entity);
	DECLARE_DATADESC();

	CASW_Spawn_Railing();
	virtual ~CASW_Spawn_Railing();

	virtual void SetUpAlien(IASW_Spawnable_NPC *pAlien);
	virtual bool CanSpawn(int nAlienType);

protected:
	bool m_bRailingIsHigh;
};

class CASW_Spawn_Jump : public CASW_Spawn_Entity
{
public:
	DECLARE_CLASS(CASW_Spawn_Jump, CASW_Spawn_Entity);
	DECLARE_DATADESC();

	CASW_Spawn_Jump();
	virtual ~CASW_Spawn_Jump();

	virtual void Spawn();
	virtual void SetUpAlien(IASW_Spawnable_NPC *pAlien);
	virtual bool CanSpawn(int nAlienType);

protected:
	int m_nJumpHeight;
};

#endif /* _INCLUDED_ASW_SPAWN_ENTITY_H */
