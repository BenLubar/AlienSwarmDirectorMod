#include "cbase.h"
#include "asw_spawn_entity.h"
#include "asw_gamerules.h"
#include "asw_util_shared.h"
#include "ai_basenpc.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar asw_debug_spawners;
ConVar asw_spawn_delay_alive("asw_spawn_delay_alive", "1.0", FCVAR_CHEAT, "delay on asw_spawn_*'s next think if we have enough live aliens but haven't exhausted our supply", true, 0.0f, false, 0.0f);
ConVar asw_spawn_delay_range("asw_spawn_delay_range", "1.0", FCVAR_CHEAT, "delay on asw_spawn_*'s next think if no marines are in range", true, 0.0f, false, 0.0f);
ConVar asw_spawn_delay_min("asw_spawn_delay_min", "0.01", FCVAR_CHEAT, "minimum delay between asw_spawn_* spawns", true, 0.0f, false, 0.0f);
ConVar asw_spawn_raise("asw_spawn_raise", "0", FCVAR_CHEAT, "raise aliens form asw_spawn_* by this amount before trying to spawn them.", true, 0.0f, false, 0.0f);

IMPLEMENT_AUTO_LIST(IASW_Spawn_Entity_List);

BEGIN_DATADESC(CASW_Spawn_Entity)
	DEFINE_KEYFIELD(m_AlienName, FIELD_STRING, "alien_name"),
	DEFINE_KEYFIELD(m_bWaitingForInput, FIELD_BOOLEAN, "wait_for_input"),
	DEFINE_FIELD(m_bSpawningEnabled, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bNextThinkSpawns, FIELD_BOOLEAN),
	DEFINE_KEYFIELD(m_bAllowLongThink, FIELD_BOOLEAN, "allow_long_think"),
	DEFINE_KEYFIELD(m_flStartDistance, FIELD_FLOAT, "start_distance"),
	DEFINE_KEYFIELD(m_flStopDistance, FIELD_FLOAT, "stop_distance"),

#define DEFINE_ALIEN_LIMIT_FIELDS(n) \
	DEFINE_KEYFIELD(m_nMaxLiveAliens[n], FIELD_INTEGER, "max_aliens_live_" #n), \
	DEFINE_KEYFIELD(m_nMaxSpawnedAliens[n], FIELD_INTEGER, "max_aliens_spawned_" #n), \
	DEFINE_KEYFIELD(m_nMaxDirectorAliens[n], FIELD_INTEGER, "max_aliens_director_" #n)

	DEFINE_ALIEN_LIMIT_FIELDS(0),
	DEFINE_ALIEN_LIMIT_FIELDS(1),
	DEFINE_ALIEN_LIMIT_FIELDS(2),
	DEFINE_ALIEN_LIMIT_FIELDS(3),
	DEFINE_ALIEN_LIMIT_FIELDS(4),

#undef DEFINE_ALIEN_LIMIT_FIELDS
	DEFINE_FIELD(m_nLiveAliens, FIELD_INTEGER),
	DEFINE_FIELD(m_nLiveDirector, FIELD_INTEGER),
	DEFINE_FIELD(m_nSpawnedAliens, FIELD_INTEGER),
	DEFINE_FIELD(m_nDirectorAliens, FIELD_INTEGER),

	DEFINE_KEYFIELD(m_flMinSpawnInterval, FIELD_FLOAT, "min_spawn_interval"),
	DEFINE_KEYFIELD(m_flMaxSpawnInterval, FIELD_FLOAT, "max_spawn_interval"),

#define DEFINE_SPAWN_CHANCE_FIELDS(n) \
	DEFINE_KEYFIELD(m_flSpawnChance[n], FIELD_FLOAT, "spawn_chance_" #n)

	DEFINE_SPAWN_CHANCE_FIELDS(0),
	DEFINE_SPAWN_CHANCE_FIELDS(1),
	DEFINE_SPAWN_CHANCE_FIELDS(2),
	DEFINE_SPAWN_CHANCE_FIELDS(3),
	DEFINE_SPAWN_CHANCE_FIELDS(4),
	DEFINE_SPAWN_CHANCE_FIELDS(5),
	DEFINE_SPAWN_CHANCE_FIELDS(6),
	DEFINE_SPAWN_CHANCE_FIELDS(7),
	DEFINE_SPAWN_CHANCE_FIELDS(8),
	DEFINE_SPAWN_CHANCE_FIELDS(9),
	DEFINE_SPAWN_CHANCE_FIELDS(10),
	DEFINE_SPAWN_CHANCE_FIELDS(11),
	DEFINE_SPAWN_CHANCE_FIELDS(12),
	DEFINE_SPAWN_CHANCE_FIELDS(13),
	DEFINE_SPAWN_CHANCE_FIELDS(14),
	DEFINE_SPAWN_CHANCE_FIELDS(15),
	DEFINE_SPAWN_CHANCE_FIELDS(16),

#undef DEFINE_SPAWN_CHANCE_FIELDS
	DEFINE_FIELD(m_flSpawnChanceTotal, FIELD_FLOAT),

	DEFINE_KEYFIELD(m_bFlammable, FIELD_BOOLEAN, "aliens_flame"),
	DEFINE_KEYFIELD(m_bTeslable, FIELD_BOOLEAN, "aliens_stun"),
	DEFINE_KEYFIELD(m_bFreezable, FIELD_BOOLEAN, "aliens_freeze"),
	DEFINE_KEYFIELD(m_bFlinches, FIELD_BOOLEAN, "aliens_flinch"),

	DEFINE_KEYFIELD(m_flHealthScale, FIELD_FLOAT, "health_scale"),
	DEFINE_KEYFIELD(m_flSizeScale, FIELD_FLOAT, "size_scale"),
	DEFINE_KEYFIELD(m_flSpeedScale, FIELD_FLOAT, "speed_scale"),

	DEFINE_KEYFIELD(m_flMaxYaw, FIELD_FLOAT, "max_yaw"),
	DEFINE_KEYFIELD(m_flMaxForward, FIELD_FLOAT, "max_forward"),
	DEFINE_KEYFIELD(m_flMaxSide, FIELD_FLOAT, "max_side"),

	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),

	DEFINE_OUTPUT(m_OnSpawned, "OnSpawned"),
	DEFINE_OUTPUT(m_OnAnySpawned, "OnAnySpawned"),
	DEFINE_OUTPUT(m_OnDirectorSpawned, "OnDirectorSpawned"),
	DEFINE_OUTPUT(m_OnAllSpawned, "OnAllSpawned"),
	DEFINE_OUTPUT(m_OnAllSpawnedDead, "OnAllSpawnedDead"),
END_DATADESC()

// if either of these fail, the datadesc needs to be updated.
ASSERT_INVARIANT(ASW_DIFFICULTY_COUNT == 5);
ASSERT_INVARIANT(ASW_NUM_ALIEN_CLASSES == 17);

CASW_Spawn_Entity::CASW_Spawn_Entity()
{
	m_AlienName = NULL_STRING;
	m_bWaitingForInput = false;
	m_bSpawningEnabled = false;
	m_bNextThinkSpawns = false;
	m_bAllowLongThink = false;
	m_flStartDistance = -1;
	m_flStopDistance = -1;

	m_flMinSpawnInterval = 4 * 0.75f;
	m_flMaxSpawnInterval = 4 * 1.25f;

	for (int i = 0; i < ASW_DIFFICULTY_COUNT; i++)
	{
		m_nMaxLiveAliens[i] = -1;
		m_nMaxSpawnedAliens[i] = -1;
		m_nMaxDirectorAliens[i] = -1;
	}
	m_nLiveAliens = 0;
	m_nLiveDirector = 0;
	m_nSpawnedAliens = 0;
	m_nDirectorAliens = 0;

	m_flSpawnChanceTotal = 0;
	for (int i = 0; i < NELEMS(m_flSpawnChance); i++)
	{
		m_flSpawnChance[i] = 0;
	}

	m_flMaxYaw = 0;
	m_flMaxForward = 0;
	m_flMaxSide = 0;

	m_flHealthScale = 1;
	m_flSpeedScale = 1;
	m_flSizeScale = 1;
	m_bFlammable = true;
	m_bFreezable = true;
	m_bTeslable = true;
	m_bFlinches = true;
}

CASW_Spawn_Entity::~CASW_Spawn_Entity()
{
}

void CASW_Spawn_Entity::Spawn()
{
	BaseClass::Spawn();

	SetSolid(SOLID_NONE);

	if (m_flHealthScale <= 0 || m_flSpeedScale <= 0 || m_flSizeScale <= 0)
	{
		Warning("spawner at (%f %f %f) has non-positive scale (removed)\n", VectorExpand(GetAbsOrigin()));
		UTIL_Remove(this);
		return;
	}

	m_flSpawnChanceTotal = 0;
	for (int i = 0; i < NELEMS(m_flSpawnChance); i++)
	{
		if (m_flSpawnChance[i] < 0 || (m_flSpawnChance[i] != 0 && !CanSpawn(i)))
		{
			Warning("spawner at (%f %f %f) has invalid spawn chance for %s (set to 0)\n", VectorExpand(GetAbsOrigin()), ASWSpawnManager()->GetAlienClass(i)->m_pszAlienClass);
			m_flSpawnChance[i] = 0;
		}
		m_flSpawnChanceTotal += m_flSpawnChance[i];
	}
	if (m_flSpawnChanceTotal == 0)
	{
		Warning("spawner at (%f %f %f) has 0 total spawn chance (allowing all aliens with equal probability)\n", VectorExpand(GetAbsOrigin()));
		for (int i = 0; i < NELEMS(m_flSpawnChance); i++)
		{
			if (CanSpawn(i))
			{
				m_flSpawnChance[i] = 1;
				m_flSpawnChanceTotal += 1;
			}
		}
	}
	Assert(m_flSpawnChanceTotal > 0);

	Precache();

	SetNextThink(gpGlobals->curtime);
}

void CASW_Spawn_Entity::Precache()
{
	BaseClass::Precache();

	for (int i = 0; i < NELEMS(m_flSpawnChance); i++)
	{
		if (m_flSpawnChance[i] != 0)
		{
			UTIL_PrecacheOther(ASWSpawnManager()->GetAlienClass(i)->m_pszAlienClass);
		}
	}
}

void CASW_Spawn_Entity::DeathNotice(CBaseEntity *pVictim)
{
	Assert(dynamic_cast<IASW_Spawnable_NPC *>(pVictim));
	if (dynamic_cast<IASW_Spawnable_NPC *>(pVictim)->IsDirectorAlien())
	{
		m_nLiveDirector--;
	}
	else
	{
		m_nLiveAliens--;

		const int limit_index = GetAlienLimitIndex();

		if (m_nLiveAliens == 0 && m_nMaxSpawnedAliens[limit_index] >= 0 && m_nMaxSpawnedAliens[limit_index] <= m_nSpawnedAliens)
		{
			m_OnAllSpawnedDead.FireOutput(this, this);
		}
	}
	Assert(m_nLiveAliens >= 0 && m_nLiveDirector >= 0);
}

void CASW_Spawn_Entity::Enable()
{
	m_bWaitingForInput = false;
	if (!m_bNextThinkSpawns)
	{
		SetNextThink(gpGlobals->curtime);
	}
}

void CASW_Spawn_Entity::Disable()
{
	m_bNextThinkSpawns = false;
	m_bWaitingForInput = true;
}

int CASW_Spawn_Entity::GetAlienLimitIndex()
{
	int index = ASWGameRules()->GetSkillLevel() - 1;
	Assert(index >= 0 && index < ASW_DIFFICULTY_COUNT);
	return clamp(index, 0, ASW_DIFFICULTY_COUNT - 1);
}

float CASW_Spawn_Entity::CheckDistances()
{
	float distance = 0;
	if (!UTIL_ASW_NearestMarine(GetAbsOrigin(), distance))
	{
		// There are no marines, so we shouldn't spawn.
		m_bSpawningEnabled = false;
		return distance;
	}

	const float start = (m_flStartDistance >= 0) ? m_flStartDistance : COORD_EXTENT;
	const float stop  = (m_flStopDistance  >= 0) ? m_flStopDistance  : COORD_EXTENT;

	if (distance < start)
	{
		m_bSpawningEnabled = true;
	}

	if (distance > stop)
	{
		m_bSpawningEnabled = false;
	}

	// if distance >= start and distance <= stop, we keep the old value.
	return distance;
}

void CASW_Spawn_Entity::Think()
{
	if (m_bWaitingForInput)
	{
		// Don't bother thinking if we're waiting for an input. Enable will set the think again when it's called.
		return;
	}

	CheckDistances();
	if (m_bSpawningEnabled)
	{
		if (m_bNextThinkSpawns)
		{
			DoSpawn();
		}
		SetUpNextSpawn();
	}
	else
	{
		m_bNextThinkSpawns = false;
		SetNextThink(gpGlobals->curtime + asw_spawn_delay_range.GetFloat());
	}
}

void CASW_Spawn_Entity::SetUpNextSpawn()
{
	const int limit_index = GetAlienLimitIndex();

	if (m_nMaxSpawnedAliens[limit_index] >= 0 && m_nMaxSpawnedAliens[limit_index] <= m_nSpawnedAliens)
	{
		SetNextThink(TICK_NEVER_THINK);
		return;
	}

	if (m_nMaxLiveAliens[limit_index] >= 0 && m_nMaxLiveAliens[limit_index] <= m_nLiveAliens)
	{
		SetNextThink(gpGlobals->curtime + asw_spawn_delay_alive.GetFloat());
		return;
	}

	const float min = MAX(m_flMinSpawnInterval, asw_spawn_delay_min.GetFloat());
	const float max = MAX(min, m_flMaxSpawnInterval);

	m_bNextThinkSpawns = true;
	SetNextThink(gpGlobals->curtime + RandomFloat(min, max));
}

void CASW_Spawn_Entity::DoSpawn(bool bIsDirector)
{
	int choice = 0;
	float x = RandomFloat(0, m_flSpawnChanceTotal);
	for (int i = 0; i < NELEMS(m_flSpawnChance); i++)
	{
		x -= m_flSpawnChance[i];

		// we do this in a counter-intuitive way to prevent rounding errors from causing problems.
		if (m_flSpawnChance[i] != 0)
		{
			choice = i;
		}
		if (x <= 0)
		{
			break;
		}
	}

	Assert(ASWSpawnManager());
	Assert(ASWSpawnManager()->GetAlienClass(choice));
	Assert(CanSpawn(choice));
	CBaseEntity *pEnt = CreateEntityByName(ASWSpawnManager()->GetAlienClass(choice)->m_pszAlienClass);
	Assert(pEnt);

	QAngle angle = GetAbsAngles();
	angle[YAW] += RandomFloat(-m_flMaxYaw, m_flMaxYaw);
	pEnt->SetAbsAngles(angle);
	Vector forward, right, up;
	AngleVectors(angle, &forward, &right, &up);
	forward *= RandomFloat(-m_flMaxForward, m_flMaxForward);
	right *= RandomFloat(-m_flMaxSide, m_flMaxSide);
	up *= asw_spawn_raise.GetFloat();
	Vector origin = GetAbsOrigin();
	origin += forward;
	origin += right;
	origin += up;
	pEnt->SetAbsOrigin(origin);
	pEnt->SetName(m_AlienName);

	IASW_Spawnable_NPC *pSpawn = dynamic_cast<IASW_Spawnable_NPC *>(pEnt);
	Assert(pSpawn);

	CAI_BaseNPC *pNPC = pSpawn->GetNPC();
	if (pNPC)
	{
		pNPC->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);
		if (m_bAllowLongThink)
		{
			pNPC->AddSpawnFlags(SF_NPC_ALWAYSTHINK);
		}
	}

	pSpawn->SetAlienOrders(AOT_MoveToNearestMarine, vec3_origin, NULL);
	SetUpAlien(pSpawn);
	DispatchSpawn(pEnt);

	pSpawn->SetSpawner(this);
	pSpawn->CustomSettings(m_flHealthScale, m_flSpeedScale, m_flSizeScale, m_bFlammable, m_bFreezable, m_bTeslable, m_bFlinches);
	pSpawn->SetHealthByDifficultyLevel();

	const int limit_index = GetAlienLimitIndex();

	m_bNextThinkSpawns = false;
	if (bIsDirector)
	{
		pSpawn->SetDirectorAlien();
		m_nLiveDirector++;
		m_nDirectorAliens++;
		Assert(m_nMaxDirectorAliens[limit_index] < 0 || m_nDirectorAliens <= m_nMaxDirectorAliens[limit_index]);
		m_OnDirectorSpawned.FireOutput(pEnt, this);
	}
	else
	{
		m_nLiveAliens++;
		m_nSpawnedAliens++;
		Assert(m_nMaxSpawnedAliens[limit_index] < 0 || m_nSpawnedAliens <= m_nMaxSpawnedAliens[limit_index]);
		m_OnSpawned.FireOutput(pEnt, this);
	}
	Assert(m_nMaxLiveAliens[limit_index] < 0 || m_nLiveAliens + m_nDirectorAliens <= m_nMaxLiveAliens[limit_index]);

	m_OnAnySpawned.FireOutput(pEnt, this);

	if (!bIsDirector && m_nMaxSpawnedAliens[limit_index] >= 0 && m_nMaxSpawnedAliens[limit_index] <= m_nSpawnedAliens)
	{
		m_OnAllSpawned.FireOutput(this, this);
	}
}

LINK_ENTITY_TO_CLASS(asw_spawn_offscreen, CASW_Spawn_Offscreen);

BEGIN_DATADESC(CASW_Spawn_Offscreen)
	DEFINE_KEYFIELD(m_bMoveToMarines, FIELD_BOOLEAN, "move_to_marines"),
	DEFINE_KEYFIELD(m_flMinDistance, FIELD_FLOAT, "min_distance"),
END_DATADESC()

CASW_Spawn_Offscreen::CASW_Spawn_Offscreen()
{
	m_bMoveToMarines = false;
	m_flMinDistance = 720;
}

CASW_Spawn_Offscreen::~CASW_Spawn_Offscreen()
{
}

void CASW_Spawn_Offscreen::SetUpAlien(IASW_Spawnable_NPC *pAlien)
{
	if (pAlien->CanStartBurrowed())
	{
		pAlien->StartBurrowed();
	}

	if (!m_bMoveToMarines)
	{
		pAlien->SetAlienOrders(AOT_SpreadThenHibernate, vec3_origin, NULL);
		pAlien->MoveAside();
	}
}

bool CASW_Spawn_Offscreen::CanSpawn(int nAlienType)
{
	return true;
}

float CASW_Spawn_Offscreen::CheckDistances()
{
	const float distance = BaseClass::CheckDistances();

	if (distance < m_flMinDistance)
	{
		m_bSpawningEnabled = false;
	}

	return distance;
}

LINK_ENTITY_TO_CLASS(asw_spawn_burrow, CASW_Spawn_Burrow);

CASW_Spawn_Burrow::CASW_Spawn_Burrow()
{
}

CASW_Spawn_Burrow::~CASW_Spawn_Burrow()
{
}

void CASW_Spawn_Burrow::SetUpAlien(IASW_Spawnable_NPC *pAlien)
{
	pAlien->StartBurrowed();
}

bool CASW_Spawn_Burrow::CanSpawn(int nAlienType)
{
	const char *pszAlienClass = ASWSpawnManager()->GetAlienClass(nAlienType)->m_pszAlienClass;
	if (FStrEq(pszAlienClass, "asw_drone"))
	{
		return true;
	}
	if (StringHasPrefix(pszAlienClass, "asw_drone_"))
	{
		return true;
	}
	if (FStrEq(pszAlienClass, "asw_ranger"))
	{
		return true;
	}
	return false;
}

LINK_ENTITY_TO_CLASS(asw_spawn_hallway, CASW_Spawn_Hallway);

CASW_Spawn_Hallway::CASW_Spawn_Hallway()
{
}

CASW_Spawn_Hallway::~CASW_Spawn_Hallway()
{
}

void CASW_Spawn_Hallway::SetUpAlien(IASW_Spawnable_NPC *pAlien)
{
	if (dynamic_cast<CBaseEntity *>(pAlien)->Classify() == CLASS_ASW_BOOMER)
	{
		pAlien->SetUnburrowActivity(AllocPooledString("ACT_ROLL_512"));
		pAlien->SetUnburrowIdleActivity(AllocPooledString("ACT_ROLL_IDLE"));
	}
	else
	{
		pAlien->SetUnburrowActivity(AllocPooledString("ACT_SPAWN_512"));
		pAlien->SetUnburrowIdleActivity(AllocPooledString("ACT_SPAWN_IDLE"));
	}
	pAlien->StartBurrowed();
}

bool CASW_Spawn_Hallway::CanSpawn(int nAlienType)
{
	const char *pszAlienClass = ASWSpawnManager()->GetAlienClass(nAlienType)->m_pszAlienClass;
	if (FStrEq(pszAlienClass, "asw_boomer"))
	{
		return true;
	}
	if (FStrEq(pszAlienClass, "asw_mortarbug"))
	{
		return true;
	}
	return false;
}

LINK_ENTITY_TO_CLASS(asw_spawn_wall, CASW_Spawn_Wall);

BEGIN_DATADESC(CASW_Spawn_Wall)
	DEFINE_KEYFIELD(m_bVentIsHigh, FIELD_BOOLEAN, "vent_is_high"),
END_DATADESC()

CASW_Spawn_Wall::CASW_Spawn_Wall()
{
	m_bVentIsHigh = false;
}

CASW_Spawn_Wall::~CASW_Spawn_Wall()
{
}

void CASW_Spawn_Wall::SetUpAlien(IASW_Spawnable_NPC *pAlien)
{
	if (m_bVentIsHigh)
	{
		pAlien->SetUnburrowActivity(AllocPooledString("ACT_CLIMB_DOWN_WALL_128"));
		pAlien->SetUnburrowIdleActivity(AllocPooledString("ACT_CLIMB_DOWN_WALL_128_IDLE"));
	}
	else if (dynamic_cast<CBaseEntity *>(pAlien)->Classify() == CLASS_ASW_RANGER)
	{
		// surprisingly, this neither goes down nor moves 128 units in any direction.
		pAlien->SetUnburrowActivity(AllocPooledString("ACT_CLIMBDOWN_128"));
	}
	else
	{
		pAlien->SetUnburrowActivity(AllocPooledString("ACT_CLIMB_OUT_WALL"));
	}
	pAlien->StartBurrowed();
}

bool CASW_Spawn_Wall::CanSpawn(int nAlienType)
{
	const char *pszAlienClass = ASWSpawnManager()->GetAlienClass(nAlienType)->m_pszAlienClass;
	if (FStrEq(pszAlienClass, "asw_drone"))
	{
		return true;
	}
	if (StringHasPrefix(pszAlienClass, "asw_drone_"))
	{
		return true;
	}
	if (m_bVentIsHigh)
	{
		return false;
	}
	if (FStrEq(pszAlienClass, "asw_ranger"))
	{
		return true;
	}
	if (FStrEq(pszAlienClass, "asw_boomer"))
	{
		return true;
	}
	return false;
}

LINK_ENTITY_TO_CLASS(asw_spawn_railing, CASW_Spawn_Railing);

BEGIN_DATADESC(CASW_Spawn_Railing)
	DEFINE_KEYFIELD(m_bRailingIsHigh, FIELD_BOOLEAN, "railing_is_high"),
END_DATADESC()

CASW_Spawn_Railing::CASW_Spawn_Railing()
{
	m_bRailingIsHigh = false;
}

CASW_Spawn_Railing::~CASW_Spawn_Railing()
{
}

void CASW_Spawn_Railing::SetUpAlien(IASW_Spawnable_NPC *pAlien)
{
	if (m_bRailingIsHigh)
	{
		pAlien->SetUnburrowActivity(AllocPooledString("ACT_CLIMB_OFF_RAIL_76"));
		pAlien->SetUnburrowIdleActivity(AllocPooledString("ACT_CLIMB_OFF_RAIL_76_IDLE"));
	}
	else if (dynamic_cast<CBaseEntity *>(pAlien)->Classify() == CLASS_ASW_RANGER)
	{
		pAlien->SetUnburrowActivity(AllocPooledString("ACT_CLIMB_RAIL_52"));
	}
	else
	{
		pAlien->SetUnburrowActivity(AllocPooledString("ACT_CLIMB_OFF_RAIL_52"));
		pAlien->SetUnburrowIdleActivity(AllocPooledString("ACT_CLIMB_OFF_RAIL_IDLE"));
	}
	pAlien->StartBurrowed();
}

bool CASW_Spawn_Railing::CanSpawn(int nAlienType)
{
	const char *pszAlienClass = ASWSpawnManager()->GetAlienClass(nAlienType)->m_pszAlienClass;
	if (FStrEq(pszAlienClass, "asw_drone"))
	{
		return true;
	}
	if (StringHasPrefix(pszAlienClass, "asw_drone_"))
	{
		return true;
	}
	if (m_bRailingIsHigh)
	{
		return false;
	}
	if (FStrEq(pszAlienClass, "asw_ranger"))
	{
		return true;
	}
	return false;
}

LINK_ENTITY_TO_CLASS(asw_spawn_jump, CASW_Spawn_Jump);

BEGIN_DATADESC(CASW_Spawn_Jump)
	DEFINE_KEYFIELD(m_nJumpHeight, FIELD_INTEGER, "jump_height"),
END_DATADESC()

CASW_Spawn_Jump::CASW_Spawn_Jump()
{
	m_nJumpHeight = 0;
}

CASW_Spawn_Jump::~CASW_Spawn_Jump()
{
}

void CASW_Spawn_Jump::Spawn()
{
	BaseClass::Spawn();

	Assert(m_nJumpHeight >= 0 && m_nJumpHeight < 5);
	m_nJumpHeight = clamp(m_nJumpHeight, 0, 5-1);
}

void CASW_Spawn_Jump::SetUpAlien(IASW_Spawnable_NPC *pAlien)
{
	switch (m_nJumpHeight)
	{
	case 0:
		pAlien->SetUnburrowActivity(AllocPooledString("ACT_JUMP_DOWN_LOW_36"));
		pAlien->SetUnburrowIdleActivity(AllocPooledString("ACT_JUMP_DOWN_LOW_36_IDLE"));
		break;
	case 1:
		pAlien->SetUnburrowActivity(AllocPooledString("ACT_JUMP_DOWN_LOW"));
		pAlien->SetUnburrowIdleActivity(AllocPooledString("ACT_JUMP_DOWN_LOW_IDLE"));
		break;
	case 2:
		pAlien->SetUnburrowActivity(AllocPooledString("ACT_JUMP_DOWN_HIGH"));
		pAlien->SetUnburrowIdleActivity(AllocPooledString("ACT_JUMP_DOWN_HIGH_IDLE"));
		break;
	case 3:
		pAlien->SetUnburrowActivity(AllocPooledString("ACT_JUMP_DOWN_FROM_OFFSCREEN_SHORT"));
		pAlien->SetUnburrowIdleActivity(AllocPooledString("ACT_JUMP_DOWN_FROM_OFFSCREEN_SHORT_IDLE"));
		break;
	case 4:
		pAlien->SetUnburrowActivity(AllocPooledString("ACT_JUMP_DOWN_FROM_OFFSCREEN_LONG"));
		pAlien->SetUnburrowIdleActivity(AllocPooledString("ACT_JUMP_DOWN_FROM_OFFSCREEN_LONG_IDLE"));
		break;
	default:
		Assert(0);
	}
	pAlien->StartBurrowed();
}

bool CASW_Spawn_Jump::CanSpawn(int nAlienType)
{
	const char *pszAlienClass = ASWSpawnManager()->GetAlienClass(nAlienType)->m_pszAlienClass;
	if (FStrEq(pszAlienClass, "asw_drone"))
	{
		return true;
	}
	if (StringHasPrefix(pszAlienClass, "asw_drone_"))
	{
		return true;
	}
	return false;
}
