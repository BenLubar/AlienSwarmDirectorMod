#include "cbase.h"
#include "asw_spawn_manager.h"
#include "asw_marine.h"
#include "asw_marine_resource.h"
#include "asw_game_resource.h"
#include "iasw_spawnable_npc.h"
#include "asw_player.h"
#include "ai_network.h"
#include "ai_waypoint.h"
#include "ai_node.h"
#include "asw_director.h"
#include "asw_util_shared.h"
#include "asw_path_utils.h"
#include "asw_trace_filter_doors.h"
#include "asw_objective_escape.h"
#include "triggers.h"
#include "datacache/imdlcache.h"
#include "ai_link.h"
#include "asw_alien.h"
#include "asw_director_control.h"
#include "asw_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CASW_Spawn_Manager g_Spawn_Manager;
CASW_Spawn_Manager* ASWSpawnManager() { return &g_Spawn_Manager; }

#define CANDIDATE_ALIEN_HULL HULL_MEDIUMBIG		// TODO: have this use the hull of the alien type we're spawning a horde of?
#define MARINE_NEAR_DISTANCE 740.0f

extern ConVar asw_skill;
extern ConVar asw_director_debug;
ConVar asw_horde_min_distance("asw_horde_min_distance", "800", FCVAR_CHEAT, "Minimum distance away from the marines the horde can spawn" );
ConVar asw_horde_max_distance("asw_horde_max_distance", "1500", FCVAR_CHEAT, "Maximum distance away from the marines the horde can spawn" );
ConVar asw_horde_size_min("asw_horde_size_min", "9", FCVAR_CHEAT, "Min horde size if we can't load alien_selection.txt" );
ConVar asw_horde_size_max("asw_horde_size_max", "14", FCVAR_CHEAT, "Max horde size if we can't load alien_selection.txt" );
ConVar asw_max_alien_batch("asw_max_alien_batch", "10", FCVAR_CHEAT, "Max number of aliens spawned in a horde batch" );
ConVar asw_batch_interval("asw_batch_interval", "5", FCVAR_CHEAT, "Time between successive batches spawning in the same spot");
ConVar asw_candidate_interval("asw_candidate_interval", "1.0", FCVAR_CHEAT, "Interval between updating candidate spawning nodes");
ConVar asw_wanderer_class( "asw_wanderer_class", "asw_drone_jumper", FCVAR_CHEAT, "Alien class used when spawning wanderers if we can't load alien_selection.txt" );
ConVar asw_horde_wanderers( "asw_horde_wanderers", "0.15", FCVAR_CHEAT, "Probability of a wanderer spawning at the same time as a horde alien.", true, 0.0f, true, 1.0f );
ConVar asw_horde_class( "asw_horde_class", "asw_drone_jumper", FCVAR_CHEAT, "Alien class used when spawning hordes" );
ConVar asw_prespawn_min_links( "asw_prespawn_min_links", "20", FCVAR_CHEAT );

CASW_Spawn_Manager::CASW_Spawn_Manager()
{
	m_nAwakeAliens = 0;
	m_nAwakeDrones = 0;
	m_pSpawnSet = NULL;
	m_pGlobalConfig = NULL;
	m_pMissionConfig = NULL;
}

CASW_Spawn_Manager::~CASW_Spawn_Manager()
{

}

// ==================================
// == Master list of alien classes ==
// ==================================

// NOTE: If you add new entries to this list, update the asw_spawner choices in swarm.fgd.
//       Do not rearrange the order or you will be changing what spawns in all the maps.

ASW_Alien_Class_Entry g_Aliens[] =
{
	ASW_Alien_Class_Entry( "asw_drone", HULL_MEDIUMBIG ),
	ASW_Alien_Class_Entry( "asw_buzzer", HULL_TINY_CENTERED ),
	ASW_Alien_Class_Entry( "asw_parasite", HULL_TINY ),
	ASW_Alien_Class_Entry( "asw_shieldbug", HULL_WIDE_SHORT ),
	ASW_Alien_Class_Entry( "asw_grub", HULL_TINY ),
	ASW_Alien_Class_Entry( "asw_drone_jumper", HULL_MEDIUMBIG ),
	ASW_Alien_Class_Entry( "asw_harvester", HULL_WIDE_SHORT ),
	ASW_Alien_Class_Entry( "asw_parasite_defanged", HULL_TINY ),
	ASW_Alien_Class_Entry( "asw_queen", HULL_LARGE_CENTERED ),
	ASW_Alien_Class_Entry( "asw_boomer", HULL_LARGE ),
	ASW_Alien_Class_Entry( "asw_ranger", HULL_MEDIUMBIG ),
	ASW_Alien_Class_Entry( "asw_mortarbug", HULL_WIDE_SHORT ),
	ASW_Alien_Class_Entry( "asw_shaman", HULL_MEDIUM ),
	ASW_Alien_Class_Entry( "asw_drone_redgiant", HULL_MEDIUMBIG ),
	ASW_Alien_Class_Entry( "asw_drone_ghost", HULL_MEDIUMBIG ),
	ASW_Alien_Class_Entry( "asw_drone_carrier", HULL_MEDIUMBIG ),
	ASW_Alien_Class_Entry( "asw_drone_summoner", HULL_MEDIUMBIG ),
};

ASSERT_INVARIANT(NELEMS(g_Aliens) == ASW_NUM_ALIEN_CLASSES);

// Array indices of drones.  Used by carnage mode.
const int g_nDroneClassEntry = 0;
const int g_nDroneJumperClassEntry = 5;

int CASW_Spawn_Manager::GetNumAlienClasses()
{
	return ASW_NUM_ALIEN_CLASSES;
}

ASW_Alien_Class_Entry* CASW_Spawn_Manager::GetAlienClass( int i )
{
	Assert( i >= 0 && i < ASW_NUM_ALIEN_CLASSES );
	return &g_Aliens[ i ];
}

void CASW_Spawn_Manager::LevelInitPreEntity()
{
	m_nAwakeAliens = 0;
	m_nAwakeDrones = 0;
	// init alien classes
	for ( int i = 0; i < GetNumAlienClasses(); i++ )
	{
		GetAlienClass( i )->m_iszAlienClass = AllocPooledString( GetAlienClass( i )->m_pszAlienClass );
	}
}

void CASW_Spawn_Manager::LevelInitPostEntity()
{
	m_vecHordePosition = vec3_origin;
	m_angHordeAngle = vec3_angle;
	m_batchInterval.Invalidate();
	m_CandidateUpdateTimer.Invalidate();
	m_iHordeToSpawn = 0;
	m_iAliensToSpawn = 0;

	m_northCandidateNodes.Purge();
	m_southCandidateNodes.Purge();

	FindEscapeTriggers();

	LoadConfig(&m_pGlobalConfig, "resource/alien_selection.txt", true);
	LoadConfig(&m_pMissionConfig, CFmtStr("resource/alien_selection_%s.txt", STRING(gpGlobals->mapname)));
}

void CASW_Spawn_Manager::LevelShutdownPostEntity()
{
	m_pSpawnSet = NULL; // This is a pointer to one of the two below.
	if (m_pGlobalConfig != NULL)
	{
		m_pGlobalConfig->deleteThis();
		m_pGlobalConfig = NULL;
	}
	if (m_pMissionConfig != NULL)
	{
		m_pMissionConfig->deleteThis();
		m_pMissionConfig = NULL;
	}
}

void CASW_Spawn_Manager::SelectSpawnSet()
{
	int iDifficulty = 1;
	if (ASWGameRules())
	{
		iDifficulty = clamp(ASWGameRules()->GetMissionDifficulty(), 1, 99);
	}
	if (asw_director_debug.GetBool())
	{
		Msg("Mission difficulty: %d\n", iDifficulty);
	}

#define FIND_SPAWN_SET(pConfig, pszConfigName) \
	if ((pConfig)) \
		{ \
		for (KeyValues *pSpawnSet = pConfig; pSpawnSet; pSpawnSet = pSpawnSet->GetNextTrueSubKey()) \
		{ \
			if (V_stricmp(pSpawnSet->GetName(), "SpawnSet")) \
			{ \
				Warning("Spawn Manager %s config should only have SpawnSet keys, but it has a %s key.\n", pszConfigName, pSpawnSet->GetName()); \
				continue; \
			} \
			\
			if ((!V_stricmp(pSpawnSet->GetString("Map"), "*") || !V_stricmp(pSpawnSet->GetString("Map"), STRING(gpGlobals->mapname))) && \
				pSpawnSet->GetInt("MinDifficulty", 1) <= iDifficulty && pSpawnSet->GetInt("MaxDifficulty", 99) >= iDifficulty) \
			{ \
				if (asw_director_debug.GetBool()) \
				{ \
					Msg("Spawn Manager candidate SpawnSet: %s\n", pSpawnSet->GetString("Name", "[unnamed]")); \
				} \
				m_pSpawnSet = pSpawnSet; \
			} \
		} \
	}

	FIND_SPAWN_SET(m_pGlobalConfig, "global");
	FIND_SPAWN_SET(m_pMissionConfig, "mission");
#undef FIND_SPAWN_SET
	Assert(m_pSpawnSet);
	if (!m_pSpawnSet)
	{
		Warning("Spawn Manager could not find any spawn config!!!\n");
	}
	else if (asw_director_debug.GetBool())
	{
		KeyValuesDumpAsDevMsg(m_pSpawnSet);
		if (!V_stricmp(m_pSpawnSet->GetString("Map", "*"), "*"))
		{
			Warning("Spawn Manager using default SpawnSet.\n");
		}
		Msg("Spawn Manager using SpawnSet: %s\n", m_pSpawnSet->GetString("Name", "[unnamed]"));
	}
}

void CASW_Spawn_Manager::LoadConfig(KeyValues **ppKV, const char *pszFileName, bool bWarnIfMissing)
{
	KeyValues *pKV = new KeyValues("alien_selection");
	if (pKV->LoadFromFile(filesystem, pszFileName))
	{
		*ppKV = pKV;
		return;
	}
	if (bWarnIfMissing)
	{
		Warning("Spawn Manager missing file: %s\n", pszFileName);
	}
	pKV->deleteThis();
}

void CASW_Spawn_Manager::OnAlienWokeUp( CASW_Alien *pAlien )
{
	m_nAwakeAliens++;
	if ( pAlien && pAlien->Classify() == CLASS_ASW_DRONE )
	{
		m_nAwakeDrones++;
	}
}

void CASW_Spawn_Manager::OnAlienSleeping( CASW_Alien *pAlien )
{
	m_nAwakeAliens--;
	if ( pAlien && pAlien->Classify() == CLASS_ASW_DRONE )
	{
		m_nAwakeDrones--;
	}
}

// finds all trigger_multiples linked to asw_objective_escape entities
void CASW_Spawn_Manager::FindEscapeTriggers()
{
	m_EscapeTriggers.Purge();

	// go through all escape objectives
	CBaseEntity* pEntity = NULL;
	while ( (pEntity = gEntList.FindEntityByClassname( pEntity, "asw_objective_escape" )) != NULL )
	{
		CASW_Objective_Escape* pObjective = dynamic_cast<CASW_Objective_Escape*>(pEntity);
		if ( !pObjective )
			continue;

		string_t iszEscapeTargetName = pObjective->GetEntityName();

		CBaseEntity* pOtherEntity = NULL;
		while ( (pOtherEntity = gEntList.FindEntityByClassname( pOtherEntity, "trigger_multiple" )) != NULL )
		{
			CTriggerMultiple *pTrigger = dynamic_cast<CTriggerMultiple*>( pOtherEntity );
			if ( !pTrigger )
				continue;

			bool bAdded = false;
			CBaseEntityOutput *pOutput = pTrigger->FindNamedOutput( "OnTrigger" );
			if ( pOutput )
			{
				CEventAction *pAction = pOutput->GetFirstAction();
				while ( pAction )
				{
					if ( pAction->m_iTarget == iszEscapeTargetName )
					{
						bAdded = true;
						m_EscapeTriggers.AddToTail( pTrigger );
						break;
					}
					pAction = pAction->m_pNext;
				}
			}

			if ( !bAdded )
			{
				pOutput = pTrigger->FindNamedOutput( "OnStartTouch" );
				if ( pOutput )
				{
					CEventAction *pAction = pOutput->GetFirstAction();
					while ( pAction )
					{
						if ( pAction->m_iTarget == iszEscapeTargetName )
						{
							bAdded = true;
							m_EscapeTriggers.AddToTail( pTrigger );
							break;
						}
						pAction = pAction->m_pNext;
					}
				}
			}
			
		}
	}
	Msg("Spawn manager found %d escape triggers\n", m_EscapeTriggers.Count() );
}


void CASW_Spawn_Manager::Update()
{
	if ( m_iHordeToSpawn > 0 )
	{		
		if ( m_vecHordePosition != vec3_origin && ( !m_batchInterval.HasStarted() || m_batchInterval.IsElapsed() ) )
		{
			int iToSpawn = MIN( m_iHordeToSpawn, asw_max_alien_batch.GetInt() );
			int iSpawned = SpawnAlienBatch( asw_horde_class.GetString(), iToSpawn, m_vecHordePosition, m_angHordeAngle, 0 );
			if (asw_director_debug.GetInt() >= 4)
				Msg("spawned %d/%d %s (horde) at (%f, %f, %f)\n", iSpawned, m_iHordeToSpawn, asw_horde_class.GetString(), VectorExpand(m_vecHordePosition));
			m_iHordeToSpawn -= iSpawned;

			for (int i = 0; i < iSpawned; i++)
			{
				if (RandomFloat() < asw_horde_wanderers.GetFloat())
				{
					KeyValues *pWanderer = RandomWanderer();
					if (pWanderer)
					{
						FOR_EACH_TRUE_SUBKEY(pWanderer, pNPC)
						{
							if (V_stricmp(pNPC->GetName(), "NPC"))
							{
								Warning("Spawn Manager ignoring non-NPC key in WANDERER definition: %s\n", pNPC->GetName());
								continue;
							}

							const char *szAlienClass = pNPC->GetString("AlienClass");
							if (SpawnAlienAt(szAlienClass, m_vecHordePosition + Vector(0, 0, !V_stricmp(szAlienClass, "asw_buzzer") ? 128 : 32), m_angHordeAngle, pNPC))
							{
								if (asw_director_debug.GetInt() >= 4)
								{
									Msg("spawned %s (horde wanderer) at (%f, %f, %f)\n", szAlienClass, VectorExpand(m_vecHordePosition));
								}
							}
						}
					}
					else
					{
						const char *szAlienClass = asw_wanderer_class.GetString();
						if (SpawnAlienAt(szAlienClass, m_vecHordePosition + Vector(0, 0, !V_stricmp(szAlienClass, "asw_buzzer") ? 128 : 32), m_angHordeAngle))
						{
							if (asw_director_debug.GetInt() >= 4)
							{
								Msg("spawned %s (horde wanderer) at (%f, %f, %f)\n", szAlienClass, VectorExpand(m_vecHordePosition));
							}
						}
					}
				}
			}

			if ( m_iHordeToSpawn <= 0 )
			{
				ASWDirector()->OnHordeFinishedSpawning();
				m_vecHordePosition = vec3_origin;
			}
			else if ( iSpawned == 0 )			// if we failed to spawn any aliens, then try to find a new horde location
			{
				if ( asw_director_debug.GetBool() )
				{
					Msg( "Horde failed to spawn any aliens, trying new horde position.\n" );
				}
				if ( !FindHordePosition() )		// if we failed to find a new location, just abort this horde
				{
					if (asw_director_debug.GetBool())
					{
						Msg("Horde failed to find a new location, clearing.\n");
					}
					m_iHordeToSpawn = 0;
					ASWDirector()->OnHordeFinishedSpawning();
					m_vecHordePosition = vec3_origin;
				}
			}
			m_batchInterval.Start( asw_batch_interval.GetFloat() );
		}
		else if ( m_vecHordePosition == vec3_origin )
		{
			Msg( "Warning: Had horde to spawn but no position, clearing.\n" );
			m_iHordeToSpawn = 0;
			ASWDirector()->OnHordeFinishedSpawning();
		}
	}

	if ( asw_director_debug.GetBool() )
	{
		engine->Con_NPrintf( 9 + ASW_MAX_MARINE_RESOURCES, "SM: Batch interval: %f pos = %f %f %f\n",
			m_batchInterval.HasStarted() ? m_batchInterval.GetRemainingTime() : -1,
			VectorExpand( m_vecHordePosition ) );
	}

	if ( m_iAliensToSpawn > 0 )
	{
		if ( SpawnAlientAtRandomNode() )
			m_iAliensToSpawn--;
	}
}

void CASW_Spawn_Manager::AddAlien( bool force )
{
	// don't stock up more than 10 wanderers at once
	if ( m_iAliensToSpawn < 10 || force )
		m_iAliensToSpawn++;
}

bool CASW_Spawn_Manager::SpawnAlientAtRandomNode()
{
	UpdateCandidateNodes();

	// decide if the alien is going to come from behind or in front
	bool bNorth = RandomFloat() < 0.7f;
	if ( m_northCandidateNodes.Count() <= 0 )
	{
		bNorth = false;
	}
	else if ( m_southCandidateNodes.Count() <= 0 )
	{
		bNorth = true;
	}

	CUtlVector<CAI_Node *> &candidateNodes = bNorth ? m_northCandidateNodes : m_southCandidateNodes;

	if ( candidateNodes.Count() <= 0 )
		return false;

	int nCount = 1;
	if (m_pSpawnSet)
	{
		nCount = RandomInt(m_pSpawnSet->GetInt("MinWandererSize", 1), m_pSpawnSet->GetInt("MaxWandererSize", 1));
	}

	for (int i = 0; i < nCount; i++)
	{
		KeyValues *pWanderer = RandomWanderer();
		if (pWanderer)
		{
			FOR_EACH_TRUE_SUBKEY(pWanderer, pNPC)
			{
				if (V_stricmp(pNPC->GetName(), "NPC"))
				{
					Warning("Spawn Manager ignoring non-NPC key in WANDERER definition: %s\n", pNPC->GetName());
					continue;
				}

				if (!SpawnOneWanderer(candidateNodes, bNorth, pNPC->GetString("AlienClass"), pNPC))
				{
					return false;
				}
			}
			return true;
		}
		else
		{
			return SpawnOneWanderer(candidateNodes, bNorth, asw_wanderer_class.GetString(), NULL);
		}
	}
	return false;
}

bool CASW_Spawn_Manager::SpawnOneWanderer(CUtlVector<CAI_Node *> &candidateNodes, bool bNorth, const char *szAlienClass, KeyValues *pNPC, bool bWait)
{
	Hull_t nHull = HULL_MEDIUMBIG;
	bool ok = GetAlienHull(szAlienClass, nHull);
	Assert(ok);
	if (!ok)
	{
		Warning("asw_spawn_manager: failed to get alien hulls (%s)\n", szAlienClass);
		return false;
	}
	const Vector &vecMins = NAI_Hull::Mins(nHull);
	const Vector &vecMaxs = NAI_Hull::Maxs(nHull);

	const int iMaxTries = 4;
	for (int i = 0; i < iMaxTries; i++)
	{
		int iChosen = RandomInt(0, candidateNodes.Count() - 1);
		CAI_Node *pNode = candidateNodes[iChosen];
		if (!pNode)
			continue;

		float flDistance = 0;
		CASW_Marine *pMarine = UTIL_ASW_NearestMarine(pNode->GetPosition(nHull), flDistance);
		if (!pMarine)
			return false;

		// check if there's a route from this node to the marine(s)
		AI_Waypoint_t *pRoute = ASWPathUtils()->BuildRoute(pNode->GetPosition(nHull), pMarine->GetAbsOrigin(), (Hull_t) nHull, NULL, 100);
		if (!pRoute)
		{
			if (asw_director_debug.GetBool())
			{
				NDebugOverlay::Cross3D(pNode->GetOrigin(), 10.0f, 255, 128, 0, true, 20.0f);
			}
			continue;
		}

		if (bNorth && UTIL_ASW_DoorBlockingRoute(pRoute, true))
		{
			DeleteRoute(pRoute);
			continue;
		}

		Vector vecSpawnPos = pNode->GetPosition(nHull) + Vector(0, 0, nHull == HULL_TINY_CENTERED ? 128 : 32);
		if (ValidSpawnPoint(vecSpawnPos, vecMins, vecMaxs, true, MARINE_NEAR_DISTANCE))
		{
			QAngle angle(0, RandomFloat(0, 360), 0);
			CBaseEntity *pEnt = SpawnAlienAt(szAlienClass, vecSpawnPos, angle, pNPC);
			if (asw_director_debug.GetBool())
			{
				NDebugOverlay::Cross3D(vecSpawnPos, 25.0f, 255, 255, 255, true, 20.0f);
				float flDist;
				CASW_Marine *pMarine = UTIL_ASW_NearestMarine(vecSpawnPos, flDist);
				if (pMarine)
				{
					NDebugOverlay::Line(pMarine->GetAbsOrigin(), vecSpawnPos, 64, 64, 64, true, 60.0f);
				}
				if (asw_director_debug.GetInt() >= 4)
				{
					Msg("%s %s (wanderer) at (%f, %f, %f)\n", pEnt ? "spawned" : "failed to spawn", szAlienClass, VectorExpand(vecSpawnPos));
				}
			}

			if (pEnt)
			{
				if (bWait)
				{
					IASW_Spawnable_NPC *pSpawnable = dynamic_cast<IASW_Spawnable_NPC *>(pEnt);
					Assert(pSpawnable);
					if (pSpawnable)
					{
						pSpawnable->SetAlienOrders(AOT_SpreadThenHibernate, vec3_origin, NULL);
						if (pSpawnable->GetNPC())
						{
							pSpawnable->GetNPC()->SetEnemy(NULL);
						}
						pSpawnable->MoveAside();
					}
				}
				DeleteRoute(pRoute);
				return true;
			}
		}
		else
		{
			if (asw_director_debug.GetBool())
			{
				NDebugOverlay::Cross3D(vecSpawnPos, 25.0f, 255, 0, 0, true, 20.0f);
			}
		}
		DeleteRoute(pRoute);
	}
	return false;
}

bool CASW_Spawn_Manager::AddHorde( int iHordeSize )
{
	m_iHordeToSpawn = iHordeSize;

	if ( m_vecHordePosition == vec3_origin )
	{
		if ( !FindHordePosition() )
		{
			Msg("Error: Failed to find horde position\n");
			return false;
		}
		else
		{
			if ( asw_director_debug.GetBool() )
			{
				NDebugOverlay::Cross3D( m_vecHordePosition, 50.0f, 255, 128, 0, true, 60.0f );
				float flDist;
				CASW_Marine *pMarine = UTIL_ASW_NearestMarine( m_vecHordePosition, flDist );
				if ( pMarine )
				{
					NDebugOverlay::Line( pMarine->GetAbsOrigin(), m_vecHordePosition, 255, 128, 0, true, 60.0f );
				}
			}
		}
	}
	return true;
}

bool CASW_Spawn_Manager::AddRandomHorde()
{
	int iMin = asw_horde_size_min.GetInt();
	int iMax = asw_horde_size_max.GetInt();
	if (m_pSpawnSet)
	{
		iMin = m_pSpawnSet->GetInt("MinHordeSize", iMin);
		iMax = m_pSpawnSet->GetInt("MaxHordeSize", iMax);
	}
	int iSize = RandomInt(iMin, iMax);
	if (AddHorde(iSize))
	{
		if (asw_director_debug.GetBool())
		{
			Msg("Created horde of size %d\n", iSize);
		}
		return true;
	}
	return false;
}

CAI_Network* CASW_Spawn_Manager::GetNetwork()
{
	return g_pBigAINet;
}

void CASW_Spawn_Manager::UpdateCandidateNodes()
{
	// don't update too frequently
	if ( m_CandidateUpdateTimer.HasStarted() && !m_CandidateUpdateTimer.IsElapsed() )
		return;

	m_CandidateUpdateTimer.Start( asw_candidate_interval.GetFloat() );

	if ( !GetNetwork() || !GetNetwork()->NumNodes() )
	{
		m_vecHordePosition = vec3_origin;
		Msg("Error: Can't spawn hordes as this map has no node network\n");
		return;
	}

	CASW_Game_Resource *pGameResource = ASWGameResource();
	if ( !pGameResource )
		return;

	Vector vecSouthMarine = vec3_origin;
	Vector vecNorthMarine = vec3_origin;
	for ( int i=0;i<pGameResource->GetMaxMarineResources();i++ )
	{
		CASW_Marine_Resource *pMR = pGameResource->GetMarineResource(i);
		if ( !pMR )
			continue;

		CASW_Marine *pMarine = pMR->GetMarineEntity();
		if ( !pMarine || pMarine->GetHealth() <= 0 )
			continue;

		if ( vecSouthMarine == vec3_origin || vecSouthMarine.y > pMarine->GetAbsOrigin().y )
		{
			vecSouthMarine = pMarine->GetAbsOrigin();
		}
		if ( vecNorthMarine == vec3_origin || vecNorthMarine.y < pMarine->GetAbsOrigin().y )
		{
			vecNorthMarine = pMarine->GetAbsOrigin();
		}
	}
	if ( vecSouthMarine == vec3_origin || vecNorthMarine == vec3_origin )		// no live marines
		return;
	
	int iNumNodes = GetNetwork()->NumNodes();
	m_northCandidateNodes.Purge();
	m_southCandidateNodes.Purge();
	for ( int i=0 ; i<iNumNodes; i++ )
	{
		CAI_Node *pNode = GetNetwork()->GetNode( i );
		if ( !pNode || pNode->GetType() != NODE_GROUND )
			continue;

		Vector vecPos = pNode->GetPosition( CANDIDATE_ALIEN_HULL );
		
		// find the nearest marine to this node
		float flDistance = 0;
		CASW_Marine *pMarine = dynamic_cast<CASW_Marine*>(UTIL_ASW_NearestMarine( vecPos, flDistance ));
		if ( !pMarine )
			return;

		if ( flDistance > asw_horde_max_distance.GetFloat() || flDistance < asw_horde_min_distance.GetFloat() )
			continue;

		// check node isn't in an exit trigger
		bool bInsideEscapeArea = false;
		for ( int d=0; d<m_EscapeTriggers.Count(); d++ )
		{
			if ( m_EscapeTriggers[d]->CollisionProp()->IsPointInBounds( vecPos ) )
			{
				bInsideEscapeArea = true;
				break;
			}
		}
		if ( bInsideEscapeArea )
			continue;

		if ( vecPos.y >= vecSouthMarine.y )
		{
			if ( asw_director_debug.GetInt() == 3 )
			{
				NDebugOverlay::Box( vecPos, -Vector( 5, 5, 5 ), Vector( 5, 5, 5 ), 32, 32, 128, 10, 60.0f );
			}
			m_northCandidateNodes.AddToTail( pNode );
		}
		if ( vecPos.y <= vecNorthMarine.y )
		{
			m_southCandidateNodes.AddToTail( pNode );
			if ( asw_director_debug.GetInt() == 3 )
			{
				NDebugOverlay::Box( vecPos, -Vector( 5, 5, 5 ), Vector( 5, 5, 5 ), 128, 32, 32, 10, 60.0f );
			}
		}
	}
}

bool CASW_Spawn_Manager::FindHordePosition()
{
	// need to find a suitable place from which to spawn a horde
	// this place should:
	//   - be far enough away from the marines so the whole horde can spawn before the marines get there
	//   - should have a clear path to the marines
	
	UpdateCandidateNodes();

	// decide if the horde is going to come from behind or in front
	bool bNorth = RandomFloat() < 0.7f;
	if ( m_northCandidateNodes.Count() <= 0 )
	{
		bNorth = false;
	}
	else if ( m_southCandidateNodes.Count() <= 0 )
	{
		bNorth = true;
	}

	CUtlVector<CAI_Node *> &candidateNodes = bNorth ? m_northCandidateNodes : m_southCandidateNodes;

	if ( candidateNodes.Count() <= 0 )
	{
		if ( asw_director_debug.GetBool() )
		{
			Msg( "  Failed to find horde pos as there are no candidate nodes\n" );
		}
		return false;
	}

	Hull_t nHull = HULL_MEDIUMBIG;
	bool ok = GetAlienHull(asw_horde_class.GetString(), nHull);
	Assert( ok );
	ok;

	int iMaxTries = 3;
	for ( int i=0 ; i<iMaxTries ; i++ )
	{
		int iChosen = RandomInt( 0, candidateNodes.Count() - 1);
		CAI_Node *pNode = candidateNodes[iChosen];
		if ( !pNode )
			continue;

		float flDistance = 0;
		CASW_Marine *pMarine = UTIL_ASW_NearestMarine( pNode->GetPosition( nHull ), flDistance );
		if ( !pMarine )
		{
			if ( asw_director_debug.GetBool() )
			{
				Msg( "  Failed to find horde pos as there is no nearest marine\n" );
			}
			return false;
		}

		// check if there's a route from this node to the marine(s)
		AI_Waypoint_t *pRoute = ASWPathUtils()->BuildRoute( pNode->GetPosition( nHull ), pMarine->GetAbsOrigin(), nHull, NULL, 100 );
		if ( !pRoute )
		{
			if ( asw_director_debug.GetInt() >= 2 )
			{
				Msg( "  Discarding horde node %d as there's no route.\n", iChosen );
			}
			continue;
		}

		if ( bNorth && UTIL_ASW_DoorBlockingRoute( pRoute, true ) )
		{
			if ( asw_director_debug.GetInt() >= 2 )
			{
				Msg( "  Discarding horde node %d as there's a door in the way.\n", iChosen );
			}
			DeleteRoute( pRoute );
			continue;
		}
		
		m_vecHordePosition = pNode->GetPosition( nHull ) + Vector( 0, 0, 32 );

		// spawn facing the nearest marine
		Vector vecDir = pMarine->GetAbsOrigin() - m_vecHordePosition;
		vecDir.z = 0;
		vecDir.NormalizeInPlace();
		VectorAngles( vecDir, m_angHordeAngle );

		if ( asw_director_debug.GetInt() >= 2 )
		{
			Msg( "  Accepting horde node %d.\n", iChosen );
		}
		DeleteRoute( pRoute );
		return true;
	}

	if ( asw_director_debug.GetBool() )
	{
		Msg( "  Failed to find horde pos as we tried 3 times to build routes to possible locations, but failed\n" );
	}

	return false;
}

bool CASW_Spawn_Manager::LineBlockedByGeometry( const Vector &vecSrc, const Vector &vecEnd )
{
	trace_t tr;
	UTIL_TraceLine( vecSrc,
		vecEnd, MASK_SOLID_BRUSHONLY, 
		NULL, COLLISION_GROUP_NONE, &tr );

	return ( tr.fraction != 1.0f );
}

bool CASW_Spawn_Manager::GetAlienHull( const char *szAlienClass, Hull_t &nHull )
{
	int nCount = GetNumAlienClasses();
	for ( int i = 0 ; i < nCount; i++ )
	{
		if ( !Q_stricmp( szAlienClass, GetAlienClass( i )->m_pszAlienClass ) )
		{
			nHull = GetAlienClass( i )->m_nHullType;
			return true;
		}
	}
	return false;
}

bool CASW_Spawn_Manager::GetAlienHull( string_t iszAlienClass, Hull_t &nHull )
{
	int nCount = GetNumAlienClasses();
	for ( int i = 0 ; i < nCount; i++ )
	{
		if ( iszAlienClass == GetAlienClass( i )->m_iszAlienClass )
		{
			nHull = GetAlienClass( i )->m_nHullType;
			return true;
		}
	}
	return false;
}

bool CASW_Spawn_Manager::GetAlienBounds( const char *szAlienClass, Vector &vecMins, Vector &vecMaxs )
{
	Hull_t nHull;
	if ( GetAlienHull( szAlienClass, nHull ) )
	{
		vecMins = NAI_Hull::Mins( nHull );
		vecMaxs = NAI_Hull::Maxs( nHull );
		return true;
	}
	return false;
}

bool CASW_Spawn_Manager::GetAlienBounds( string_t iszAlienClass, Vector &vecMins, Vector &vecMaxs )
{
	Hull_t nHull;
	if ( GetAlienHull( iszAlienClass, nHull ) )
	{
		vecMins = NAI_Hull::Mins( nHull );
		vecMaxs = NAI_Hull::Maxs( nHull );
		return true;
	}
	return false;
}

// spawn a group of aliens at the target point
int CASW_Spawn_Manager::SpawnAlienBatch( const char* szAlienClass, int iNumAliens, const Vector &vecPosition, const QAngle &angFacing, float flMarinesBeyondDist )
{

	int iSpawned = 0;
	bool bCheckGround = true;
	Vector vecMins = NAI_Hull::Mins(HULL_MEDIUMBIG);
	Vector vecMaxs = NAI_Hull::Maxs(HULL_MEDIUMBIG);
	bool ok = GetAlienBounds( szAlienClass, vecMins, vecMaxs );
	Assert( ok );
	ok;

	float flAlienWidth = vecMaxs.x - vecMins.x;
	float flAlienDepth = vecMaxs.y - vecMins.y;

	// spawn one in the middle
	if ( ValidSpawnPoint( vecPosition, vecMins, vecMaxs, bCheckGround, flMarinesBeyondDist ) )
	{
		if ( SpawnAlienAt( szAlienClass, vecPosition, angFacing ) )
			iSpawned++;
	}

	// try to spawn a 5x5 grid of aliens, starting at the centre and expanding outwards
	Vector vecNewPos = vecPosition;
	for ( int i=1; i<=5 && iSpawned < iNumAliens; i++ )
	{
		QAngle angle = angFacing;
		angle[YAW] += RandomFloat( -20, 20 );
		// spawn aliens along top of box
		for ( int x=-i; x<=i && iSpawned < iNumAliens; x++ )
		{
			vecNewPos = vecPosition;
			vecNewPos.x += x * flAlienWidth;
			vecNewPos.y -= i * flAlienDepth;
			if ( !LineBlockedByGeometry( vecPosition, vecNewPos) && ValidSpawnPoint( vecNewPos, vecMins, vecMaxs, bCheckGround, flMarinesBeyondDist ) )
			{
				if ( SpawnAlienAt( szAlienClass, vecNewPos, angle ) )
					iSpawned++;
			}
		}

		// spawn aliens along bottom of box
		for ( int x=-i; x<=i && iSpawned < iNumAliens; x++ )
		{
			vecNewPos = vecPosition;
			vecNewPos.x += x * flAlienWidth;
			vecNewPos.y += i * flAlienDepth;
			if ( !LineBlockedByGeometry( vecPosition, vecNewPos) && ValidSpawnPoint( vecNewPos, vecMins, vecMaxs, bCheckGround, flMarinesBeyondDist ) )
			{
				if ( SpawnAlienAt( szAlienClass, vecNewPos, angle ) )
					iSpawned++;
			}
		}

		// spawn aliens along left of box
		for ( int y=-i + 1; y<i && iSpawned < iNumAliens; y++ )
		{
			vecNewPos = vecPosition;
			vecNewPos.x -= i * flAlienWidth;
			vecNewPos.y += y * flAlienDepth;
			if ( !LineBlockedByGeometry( vecPosition, vecNewPos) && ValidSpawnPoint( vecNewPos, vecMins, vecMaxs, bCheckGround, flMarinesBeyondDist ) )
			{
				if ( SpawnAlienAt( szAlienClass, vecNewPos, angle ) )
					iSpawned++;
			}
		}

		// spawn aliens along right of box
		for ( int y=-i + 1; y<i && iSpawned < iNumAliens; y++ )
		{
			vecNewPos = vecPosition;
			vecNewPos.x += i * flAlienWidth;
			vecNewPos.y += y * flAlienDepth;
			if ( !LineBlockedByGeometry( vecPosition, vecNewPos) && ValidSpawnPoint( vecNewPos, vecMins, vecMaxs, bCheckGround, flMarinesBeyondDist ) )
			{
				if ( SpawnAlienAt( szAlienClass, vecNewPos, angle ) )
					iSpawned++;
			}
		}
	}

	return iSpawned;
}

CBaseEntity* CASW_Spawn_Manager::SpawnAlienAt(const char* szAlienClass, const Vector& vecPos, const QAngle &angle, KeyValues *pDef)
{
	Hull_t nHull = HULL_MEDIUMBIG;
	GetAlienHull(szAlienClass, nHull);

	CBaseEntity	*pEntity = NULL;	
	pEntity = CreateEntityByName( szAlienClass );
	CAI_BaseNPC	*pNPC = dynamic_cast<CAI_BaseNPC*>(pEntity);

	if ( pNPC )
	{
		pNPC->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );		
	}

	// Strip pitch and roll from the spawner's angles. Pass only yaw to the spawned NPC.
	QAngle angles = angle;
	angles.x = 0.0;
	angles.z = 0.0;	
	pEntity->SetAbsOrigin( vecPos );	
	pEntity->SetAbsAngles( angles );
	if ( nHull != HULL_TINY_CENTERED )
		UTIL_DropToFloor( pEntity, MASK_NPCSOLID );

	IASW_Spawnable_NPC* pSpawnable = dynamic_cast<IASW_Spawnable_NPC*>( pEntity );
	Assert( pSpawnable );
	if ( !pSpawnable )
	{
		Warning("NULL Spawnable Ent in CASW_Spawn_Manager::SpawnAlienAt! AlienClass = %s\n", szAlienClass);
		UTIL_Remove(pEntity);
		return NULL;
	}

	// have drones and rangers unburrow by default, so we don't worry so much about them spawning onscreen
	if ( nHull == HULL_MEDIUMBIG )
	{			
		pSpawnable->StartBurrowed();
		pSpawnable->SetUnburrowIdleActivity( NULL_STRING );
		pSpawnable->SetUnburrowActivity( NULL_STRING );
	}

	if (pSpawnable && pDef)
	{
		float flHealthScale = pDef->GetFloat("HealthScale", 1);
		if (flHealthScale <= 0)
		{
			flHealthScale = 1;
		}
		float flSpeedScale = pDef->GetFloat("SpeedScale", 1);
		if (flSpeedScale <= 0)
		{
			flSpeedScale = 1;
		}
		float flSizeScale = pDef->GetFloat("SizeScale", 1);
		if (flSizeScale <= 0)
		{
			flSizeScale = 1;
		}
		pSpawnable->CustomSettings(flHealthScale, flSpeedScale, flSizeScale, pDef->GetBool("Flammable", true), pDef->GetBool("Freezable", true), pDef->GetBool("Teslable", true), pDef->GetBool("Flinches", true));
	}

	DispatchSpawn( pEntity );	
	pEntity->Activate();	

	// give our aliens the orders
	pSpawnable->SetAlienOrders(AOT_MoveToNearestMarine, vec3_origin, NULL);

	if ( g_pDirectorControl )
	{
		g_pDirectorControl->m_OnDirectorSpawnedAlien.FireOutput( pEntity, g_pDirectorControl );
	}

	return pEntity;
}

bool CASW_Spawn_Manager::ValidSpawnPoint( const Vector &vecPosition, const Vector &vecMins, const Vector &vecMaxs, bool bCheckGround, float flMarineNearDistance )
{
	// check if we can fit there
	trace_t tr;
	UTIL_TraceHull( vecPosition,
		vecPosition + Vector( 0, 0, 1 ),
		vecMins,
		vecMaxs,
		MASK_NPCSOLID,
		NULL,
		COLLISION_GROUP_NONE,
		&tr );

	if( tr.fraction != 1.0 )
		return false;

	// check there's ground underneath this point
	if ( bCheckGround )
	{
		UTIL_TraceHull( vecPosition + Vector( 0, 0, 1 ),
			vecPosition - Vector( 0, 0, 128 ),
			vecMins,
			vecMaxs,
			MASK_NPCSOLID,
			NULL,
			COLLISION_GROUP_NONE,
			&tr );

		if( tr.fraction == 1.0 )
			return false;
	}

	if ( flMarineNearDistance > 0 )
	{
		CASW_Game_Resource* pGameResource = ASWGameResource();
		float distance = 0.0f;
		for ( int i=0 ; i < pGameResource->GetMaxMarineResources() ; i++ )
		{
			CASW_Marine_Resource* pMR = pGameResource->GetMarineResource(i);
			if ( pMR && pMR->GetMarineEntity() && pMR->GetMarineEntity()->GetHealth() > 0 )
			{
				distance = pMR->GetMarineEntity()->GetAbsOrigin().DistTo( vecPosition );
				if ( distance < flMarineNearDistance )
				{
					return false;
				}
			}
		}
	}

	return true;
}

void CASW_Spawn_Manager::DeleteRoute( AI_Waypoint_t *pWaypointList )
{
	while ( pWaypointList )
	{
		AI_Waypoint_t *pPrevWaypoint = pWaypointList;
		pWaypointList = pWaypointList->GetNext();
		delete pPrevWaypoint;
	}
}

bool CASW_Spawn_Manager::PreSpawnAliens(float flSpawnScale)
{
	int iNumNodes = GetNetwork()->NumNodes();
	CUtlVector<CASW_Open_Area *> aAreas;
	for (int i = 0; i < iNumNodes; i++)
	{
		CAI_Node *pNode = GetNetwork()->GetNode(i);
		if (!pNode || pNode->GetType() != NODE_GROUND)
			continue;

		CASW_Open_Area *pArea = FindNearbyOpenArea(pNode->GetOrigin(), HULL_WIDE_SHORT);
		if (pArea && pArea->m_nTotalLinks >= asw_prespawn_min_links.GetInt())
		{
			// test if there's room to spawn a shieldbug at that spot
			if (ValidSpawnPoint(pArea->m_pNode->GetPosition(HULL_WIDE_SHORT), NAI_Hull::Mins(HULL_WIDE_SHORT), NAI_Hull::Maxs(HULL_WIDE_SHORT), true))
			{
				FOR_EACH_VEC(aAreas, i)
				{
					CASW_Open_Area *pOtherArea = aAreas[i];
					if (pOtherArea->m_pNode == pArea->m_pNode)
					{
						delete pArea;
						pArea = NULL;
						break;
					}
				}

				if (pArea)
				{
					aAreas.AddToTail(pArea);
				}
			}
			else
			{
				delete pArea;
			}
		}
	}

	if (aAreas.Count() < 6)
	{
		Warning("Director: Could not find enough places to spawn aliens.\n");
		aAreas.PurgeAndDeleteElements();
		return false;
	}

	for (int i = ceil(aAreas.Count() * RandomFloat(0.75f, 1.25f) * (4 + asw_skill.GetInt()) / 5.0f * flSpawnScale); i > 0; i--)
	{
		CASW_Open_Area *pArea = aAreas[RandomInt(0, aAreas.Count() - 1)];
		Assert(pArea);
		if (!pArea)
			continue;

		KeyValues *pWanderer = RandomWanderer();
		if (pWanderer)
		{
			FOR_EACH_TRUE_SUBKEY(pWanderer, pNPC)
			{
				if (V_stricmp(pNPC->GetName(), "NPC"))
				{
					Warning("Spawn Manager ignoring non-NPC key in WANDERER definition: %s\n", pNPC->GetName());
					continue;
				}

				if (!SpawnOneWanderer(pArea->m_aAreaNodes, false, pNPC->GetString("AlienClass"), pNPC, true))
				{
					return false;
				}
			}
			return true;
		}
		else
		{
			return SpawnOneWanderer(pArea->m_aAreaNodes, false, asw_wanderer_class.GetString(), NULL, true);
		}
	}
	aAreas.PurgeAndDeleteElements();
	return true;
}

// heuristic to find reasonably open space - searches for areas with high node connectivity
CASW_Open_Area* CASW_Spawn_Manager::FindNearbyOpenArea( const Vector &vecSearchOrigin, int nSearchHull )
{
	CBaseEntity *pStartEntity = gEntList.FindEntityByClassname( NULL, "info_player_start" );
	int iNumNodes = g_pBigAINet->NumNodes();
	CAI_Node *pHighestConnectivity = NULL;
	int nHighestLinks = 0;
	for ( int i=0 ; i<iNumNodes; i++ )
	{
		CAI_Node *pNode = g_pBigAINet->GetNode( i );
		if ( !pNode || pNode->GetType() != NODE_GROUND )
			continue;

		Vector vecPos = pNode->GetOrigin();
		if ( vecPos.DistToSqr( vecSearchOrigin ) > Square(400.0f) )
			continue;

		// discard if node is too near start location
		if ( pStartEntity && vecPos.DistToSqr( pStartEntity->GetAbsOrigin() ) < Square(1400.0f) )  // NOTE: assumes all start points are clustered near one another
			continue;

		// discard if node is inside an escape area
		bool bInsideEscapeArea = false;
		for ( int d=0; d<m_EscapeTriggers.Count(); d++ )
		{
			if ( m_EscapeTriggers[d]->CollisionProp()->IsPointInBounds( vecPos ) )
			{
				bInsideEscapeArea = true;
				break;
			}
		}
		if ( bInsideEscapeArea )
			continue;

		// count links that drones could follow
		int nLinks = pNode->NumLinks();
		int nValidLinks = 0;
		for ( int k = 0; k < nLinks; k++ )
		{
			CAI_Link *pLink = pNode->GetLinkByIndex( k );
			if ( !pLink )
				continue;

			if ( !( pLink->m_iAcceptedMoveTypes[nSearchHull] & bits_CAP_MOVE_GROUND ) )
				continue;

			nValidLinks++;
		}
		if ( nValidLinks > nHighestLinks )
		{
			nHighestLinks = nValidLinks;
			pHighestConnectivity = pNode;
		}
		if ( asw_director_debug.GetBool() )
		{
			NDebugOverlay::Text( vecPos, UTIL_VarArgs( "%d", nValidLinks ), false, 10.0f );
		}
	}

	if ( !pHighestConnectivity )
		return NULL;

	// now, starting at the new node, find all nearby nodes with a minimum connectivity
	CASW_Open_Area *pArea = new CASW_Open_Area();
	pArea->m_vecOrigin = pHighestConnectivity->GetOrigin();
	pArea->m_pNode = pHighestConnectivity;
	int nMinLinks = nHighestLinks * 0.3f;
	nMinLinks = MAX( nMinLinks, 4 );

	pArea->m_aAreaNodes.AddToTail( pHighestConnectivity );
	if ( asw_director_debug.GetBool() )
	{
		Msg( "minLinks = %d\n", nMinLinks );
	}
	pArea->m_nTotalLinks = 0;
	for ( int i=0 ; i<iNumNodes; i++ )
	{
		CAI_Node *pNode = g_pBigAINet->GetNode( i );
		if ( !pNode || pNode->GetType() != NODE_GROUND )
			continue;

		Vector vecPos = pNode->GetOrigin();
		float flDist = vecPos.DistToSqr( pArea->m_vecOrigin );
		if ( flDist > Square(400.0f) )
			continue;

		// discard if node is inside an escape area
		bool bInsideEscapeArea = false;
		for ( int d=0; d<m_EscapeTriggers.Count(); d++ )
		{
			if ( m_EscapeTriggers[d]->CollisionProp()->IsPointInBounds( vecPos ) )
			{
				bInsideEscapeArea = true;
				break;
			}
		}
		if ( bInsideEscapeArea )
			continue;

		// count links that drones could follow
		int nLinks = pNode->NumLinks();
		int nValidLinks = 0;
		for ( int k = 0; k < nLinks; k++ )
		{
			CAI_Link *pLink = pNode->GetLinkByIndex( k );
			if ( !pLink )
				continue;

			if ( !( pLink->m_iAcceptedMoveTypes[nSearchHull] & bits_CAP_MOVE_GROUND ) )
				continue;

			nValidLinks++;
		}
		if ( nValidLinks >= nMinLinks )
		{
			pArea->m_aAreaNodes.AddToTail( pNode );
			pArea->m_nTotalLinks += nValidLinks;
		}
	}
	// highlight and measure bounds
	Vector vecAreaMins = Vector( FLT_MAX, FLT_MAX, FLT_MAX );
	Vector vecAreaMaxs = Vector( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	
	for ( int i = 0; i < pArea->m_aAreaNodes.Count(); i++ )
	{
		vecAreaMins = VectorMin( vecAreaMins, pArea->m_aAreaNodes[i]->GetOrigin() );
		vecAreaMaxs = VectorMax( vecAreaMaxs, pArea->m_aAreaNodes[i]->GetOrigin() );
		
		if ( asw_director_debug.GetBool() )
		{
			if ( i == 0 )
			{
				NDebugOverlay::Cross3D( pArea->m_aAreaNodes[i]->GetOrigin(), 20.0f, 255, 255, 64, true, 10.0f );
			}
			else
			{
				NDebugOverlay::Cross3D( pArea->m_aAreaNodes[i]->GetOrigin(), 10.0f, 255, 128, 0, true, 10.0f );
			}
		}
	}

	Vector vecArea = ( vecAreaMaxs - vecAreaMins );
	pArea->m_flArea = vecArea.x * vecArea.y;

	if ( asw_director_debug.GetBool() )
	{
		Msg( "area mins = %f %f %f\n", VectorExpand( vecAreaMins ) );
		Msg( "area maxs = %f %f %f\n", VectorExpand( vecAreaMaxs ) );
		NDebugOverlay::Box( vec3_origin, vecAreaMins, vecAreaMaxs, 255, 128, 128, 10, 10.0f );	
		Msg( "Total links = %d Area = %f\n", pArea->m_nTotalLinks, pArea->m_flArea );
	}

	return pArea;
}

KeyValues *CASW_Spawn_Manager::RandomWanderer()
{
	if (!m_pSpawnSet)
	{
		return NULL;
	}

	float flWeight = 0;

	FOR_EACH_TRUE_SUBKEY(m_pSpawnSet, pWanderer)
	{
		if (V_stricmp(pWanderer->GetName(), "WANDERER"))
		{
			continue;
		}

		flWeight += pWanderer->GetFloat("SelectionWeight", 1);
	}

	flWeight = RandomFloat(0, flWeight);

	KeyValues *pSelected = NULL;
	FOR_EACH_TRUE_SUBKEY(m_pSpawnSet, pWanderer)
	{
		if (V_stricmp(pWanderer->GetName(), "WANDERER"))
		{
			continue;
		}

		pSelected = pWanderer;
		flWeight -= pWanderer->GetFloat("SelectionWeight", 1);
		if (flWeight <= 0)
		{
			break;
		}
	}
	return pSelected;
}

// creates a batch of aliens at the mouse cursor
void asw_alien_batch_f( const CCommand& args )
{
	MDLCACHE_CRITICAL_SECTION();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	// find spawn point
	CASW_Player* pPlayer = ToASW_Player(UTIL_GetCommandClient());
	if (!pPlayer)
		return;
	CASW_Marine *pMarine = pPlayer->GetMarine();
	if (!pMarine)
		return;
	trace_t tr;
	Vector forward;

	AngleVectors( pMarine->EyeAngles(), &forward );
	UTIL_TraceLine(pMarine->EyePosition(),
		pMarine->EyePosition() + forward * 300.0f,MASK_SOLID, 
		pMarine, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 0.0 )
	{
		// trace to the floor from this spot
		Vector vecSrc = tr.endpos;
		tr.endpos.z += 12;
		UTIL_TraceLine( vecSrc + Vector(0, 0, 12),
			vecSrc - Vector( 0, 0, 512 ) ,MASK_SOLID, 
			pMarine, COLLISION_GROUP_NONE, &tr );
		
		ASWSpawnManager()->SpawnAlienBatch( "asw_parasite", 25, tr.endpos, vec3_angle );
	}
	
	CBaseEntity::SetAllowPrecache( allowPrecache );
}
static ConCommand asw_alien_batch("asw_alien_batch", asw_alien_batch_f, "Creates a batch of aliens at the cursor", FCVAR_GAMEDLL | FCVAR_CHEAT);


void asw_alien_horde_f( const CCommand& args )
{
	if ( args.ArgC() < 2 )
	{
		Msg("supply horde size!\n");
		return;
	}
	if ( !ASWSpawnManager()->AddHorde( atoi(args[1]) ) )
	{
		Msg("Failed to add horde\n");
	}
}
static ConCommand asw_alien_horde("asw_alien_horde", asw_alien_horde_f, "Creates a horde of aliens somewhere nearby", FCVAR_GAMEDLL | FCVAR_CHEAT);
