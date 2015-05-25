#include "cbase.h"
#include "asw_door_area.h"
#include "func_movelinear.h"
#include "asw_button_area.h"
#include "asw_base_spawner.h"
#include "asw_marine_hint.h"
#include "ai_network.h"
#include "ai_link.h"
#include <filesystem.h>
#include "nav_mesh.h"
#include "asw_door.h"
#include "asw_spawner.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar asw_follow_ignore_hints("asw_follow_ignore_hints", "0.1", FCVAR_CHEAT, "If less than (by default) 10% of nodes are marine hints, use all nodes as marine hints.");

// BenLubar: I really wish I didn't have to do this, but there are popular campaigns that have huge problems in them.
class CASW_Campaign_Fixes : public CAutoGameSystem
{
public:
	CASW_Campaign_Fixes() : CAutoGameSystem("CASW_Campaign_Fixes")
	{
		m_NavMeshCRCs["dc1-omega_city"]        = 0xa7aa6e32;
		m_NavMeshCRCs["dc2-breaking_an_entry"] = 0x1faaa8ef;
		m_NavMeshCRCs["dc3-search_and_rescue"] = 0x66b4094d;
		m_NavMeshCRCs["delusion-01"]           = 0xa350fd52;
		m_NavMeshCRCs["ocs_1"]                 = 0xa5044ec4;
		m_NavMeshCRCs["ocs_2"]                 = 0x624f30e4;
		m_NavMeshCRCs["ocs_3"]                 = 0x2619ecf3;
		m_NavMeshCRCs["forestentrance"]        = 0xf0e03965;
		m_NavMeshCRCs["research7"]             = 0x5f2f7ee8;
		m_NavMeshCRCs["miningcamp"]            = 0x7fcf5fde;
		m_NavMeshCRCs["themines2"]             = 0x4174db11;

		m_NavMeshCRCs["extermination01port"]      = 0xb1220b66;
		m_NavMeshCRCs["extermination02road"]      = 0xf5a48338;
		m_NavMeshCRCs["extermination03arctic"]    = 0x2d50a7f5;
		m_NavMeshCRCs["extermination04area9800"]  = 0x3e2413d1;
		m_NavMeshCRCs["extermination05catwalks"]  = 0x9fb341c3;
		m_NavMeshCRCs["extermination06yanaurus"]  = 0x353c2df5;
		m_NavMeshCRCs["extermination08comcenter"] = 0x2f91cdb6;
		m_NavMeshCRCs["extermination09hospital"]  = 0xc85a49be;
	}

	virtual void LevelInitPostEntity()
	{
		const char *pszMap = STRING(gpGlobals->mapname);

		// Fixes cargo elevator leaving without all the marines if at least 4 marines are already on the elevator.
		// Fixes the train leaving without all the marines if at least 4 marines are already on the train.
		FOR_EACH_VEC(IASW_Use_Area_List::AutoList(), i)
		{
			CASW_Door_Area *pArea = dynamic_cast<CASW_Door_Area *>(IASW_Use_Area_List::AutoList()[i]);
			if (pArea)
			{
				Assert(pArea->m_nPlayersRequired == 1 || pArea->m_nPlayersRequired == 4 || pArea->m_nPlayersRequired == ASW_MAX_MARINE_RESOURCES);
				if (pArea->m_nPlayersRequired > 1)
				{
					pArea->m_nPlayersRequired = ASW_MAX_MARINE_RESOURCES;
				}
			}
		}

		// Fixes the last segment of a bridge on deima being extended before the marines get there.
		if (!V_stricmp(pszMap, "asi-jac2-deima"))
		{
			CFuncMoveLinear *pBridgeGate = dynamic_cast<CFuncMoveLinear *>(gEntList.FindEntityByName(NULL, "move_door_bridge"));
			Assert(pBridgeGate);
			if (pBridgeGate)
			{
				CBaseEntityOutput *pOutput = pBridgeGate->FindNamedOutput("OnFullyOpen");
				Assert(pOutput);
				if (pOutput)
				{
					Assert(pOutput->NumberOfElements() == 2);
					pOutput->DeleteAllElements();
				}
			}
		}

		// Fixes the tech marine requirement never being turned off after the last hack.
		if (!V_stricmp(pszMap, "dc1-omega_city"))
		{
			CASW_Button_Area *pEndButton = dynamic_cast<CASW_Button_Area *>(gEntList.FindEntityByName(NULL, "end_button"));
			Assert(pEndButton);
			if (pEndButton)
			{
				CBaseEntityOutput *pOutput = pEndButton->FindNamedOutput("OnButtonHackCompleted");
				Assert(pOutput);
				if (pOutput)
				{
					pOutput->ParseEventAction("asw_tech_marine_req,DisableTechMarineReq,,0,1");
				}
			}
		}

		// Fixes the tech marine requirement never being turned off after the last hack and the impossible-to-pass door that has node connections through it.
		if (!V_stricmp(pszMap, "dc2-breaking_an_entry"))
		{
			CASW_Button_Area *pEndButton = NULL;
			while ((pEndButton = dynamic_cast<CASW_Button_Area *>(gEntList.FindEntityByClassname(pEndButton, "trigger_asw_button_area"))) != NULL)
			{
				if (!V_stricmp(STRING(pEndButton->m_szPanelPropName), "elevator_pc"))
				{
					CBaseEntityOutput *pOutput = pEndButton->FindNamedOutput("OnButtonHackCompleted");
					Assert(pOutput);
					if (pOutput)
					{
						pOutput->ParseEventAction("asw_tech_marine_req,DisableTechMarineReq,,0,1");
					}
					break;
				}
			}

			CBaseEntity *pController = CreateEntityByName("info_node_link_controller");
			Assert(pController);
			if (pController)
			{
				pController->KeyValue("mins", "-96 -32 -36");
				pController->KeyValue("maxs", "96 32 192");
				pController->KeyValue("initialstate", "0");
				pController->SetAbsOrigin(Vector(48, 3216, 32));
				DispatchSpawn(pController);
			}
		}

		// Fixes the tech marine requirement never being turned off after the last hack.
		if (!V_stricmp(pszMap, "dc3-search_and_rescue"))
		{
			CASW_Button_Area *pC4PlantedButton = dynamic_cast<CASW_Button_Area *>(gEntList.FindEntityByName(NULL, "c4_planted_button"));
			Assert(pC4PlantedButton);
			if (pC4PlantedButton)
			{
				CBaseEntityOutput *pOutput = pC4PlantedButton->FindNamedOutput("OnButtonHackCompleted");
				Assert(pOutput);
				if (pOutput)
				{
					pOutput->ParseEventAction("asw_tech_marine_req,DisableTechMarineReq,,0,1");
				}
			}
		}

		// Fixes a door in Area9800 closing automatically but not opening automatically.
		if (!V_stricmp(pszMap, "area9800_lz"))
		{
			CASW_Door *pDoor = dynamic_cast<CASW_Door *>(gEntList.FindEntityByName(NULL, "door2"));
			Assert(pDoor);
			if (pDoor)
			{
				pDoor->KeyValue("returndelay", "-1");
			}
		}

		// Fixes the shamans having more than an order of magnitude more health than on vanilla Alien Swarm.
		if (!V_stricmp(pszMap, "as_sci2_sewer"))
		{
			CASW_Spawner *pSpawner = NULL;
			while ((pSpawner = dynamic_cast<CASW_Spawner *>(gEntList.FindEntityByName(pSpawner, "Shamans_boss"))) != NULL)
			{
				Assert(pSpawner->m_flLegacyHealthScale == 20);
				pSpawner->m_flLegacyHealthScale = 1;
			}
		}

		// Fixes maps before the brutal update assuming there would always be exactly 4 difficulty levels.
		bool bShouldFixSkillLevels = true;
		CASW_Base_Spawner *pSpawner = NULL;
		while ((pSpawner = dynamic_cast<CASW_Base_Spawner *>(gEntList.FindEntityByClassname(pSpawner, "asw_spawner"))) != NULL)
		{
			bShouldFixSkillLevels = bShouldFixSkillLevels && pSpawner->m_iMaxSkillLevel < 5;
		}
		while ((pSpawner = dynamic_cast<CASW_Base_Spawner *>(gEntList.FindEntityByClassname(pSpawner, "asw_holdout_spawner"))) != NULL)
		{
			bShouldFixSkillLevels = bShouldFixSkillLevels && pSpawner->m_iMaxSkillLevel < 5;
		}
		if (bShouldFixSkillLevels)
		{
			while ((pSpawner = dynamic_cast<CASW_Base_Spawner *>(gEntList.FindEntityByClassname(pSpawner, "asw_spawner"))) != NULL)
			{
				if (pSpawner->m_iMaxSkillLevel == 4)
					pSpawner->m_iMaxSkillLevel = 5;
			}
			while ((pSpawner = dynamic_cast<CASW_Base_Spawner *>(gEntList.FindEntityByClassname(pSpawner, "asw_holdout_spawner"))) != NULL)
			{
				if (pSpawner->m_iMaxSkillLevel == 4)
					pSpawner->m_iMaxSkillLevel = 5;
			}
		}

		// Fixes maps without marine hints being confusing for marines. The hints are added in ai_initutils.cpp.
		Assert( MarineHintManager() );
		if ( MarineHintManager() && MarineHintManager()->m_LastResortHints.Count() )
		{
			CUtlVector<HintData_t *> &hints = MarineHintManager()->m_LastResortHints;
			FOR_EACH_VEC_BACK( hints, i )
			{
				HintData_t *pHint = hints[i];
				Assert( pHint );
				Assert( g_pBigAINet );
				int nNode = g_pBigAINet->NearestNodeToPoint( pHint->m_vecPosition, false );
				CAI_Node *pNode = g_pBigAINet->GetNode( nNode );
				Assert( pNode );

				bool bAccessible = false;

				if ( pNode )
				{
					// We consider nodes "accessible" if a marine can walk from there to any other node.
					FOR_EACH_VEC( pNode->m_Links, j )
					{
						if ( pNode->m_Links[j]->m_iAcceptedMoveTypes[HULL_HUMAN] & bits_CAP_MOVE_GROUND )
						{
							bAccessible = true;
							break;
						}
					}
				}

				if ( !bAccessible )
				{
					hints.FastRemove( i );
					delete pHint;
				}
			}

			if ( hints.Count() * asw_follow_ignore_hints.GetFloat() > MarineHintManager()->m_Hints.Count() )
			{
				// We have less than 10% marine hints. The mapper either forgot to add hints or added a few and forgot about it. Use the nodes instead.
				MarineHintManager()->m_Hints.AddVectorToTail( hints );
				hints.Purge();

				// Reset the indices because we may have removed some from the middle.
				FOR_EACH_VEC( MarineHintManager()->m_Hints, i )
				{
					MarineHintManager()->m_Hints[i]->m_nHintIndex = i;
				}
			}
			else
			{
				hints.PurgeAndDeleteElements();
			}
		}

		// Fixes maps with known terrible navmeshes by reverting to pre-navmesh navigation.
		if (m_NavMeshCRCs.Defined(pszMap))
		{
			CFmtStr pszNavMeshName("maps/%s.nav", pszMap);
			CUtlBuffer buf; // FIXME: I'm really lazy and this reads the entire file at once just so I can compute a 4-byte checksum of it.
			                // However, the navmesh already has to fit in memory, and this happens before the level finishes loading, so it shouldn't matter.
			if (filesystem->ReadFile(pszNavMeshName, "GAME", buf))
			{
				CRC32_t crc = CRC32_ProcessSingleBuffer(buf.Base(), buf.TellMaxPut());
				buf.Clear();

#ifdef DEBUG
				DevMsg("CRC of %s navmesh is 0x%08x\n", pszMap, crc);
#endif
				if (crc == m_NavMeshCRCs[m_NavMeshCRCs.Find(pszMap)])
				{
					TheNavMesh->Reset();
				}
			}
		}
	}

	CUtlStringMap<CRC32_t> m_NavMeshCRCs;
};

static CASW_Campaign_Fixes g_CampaignFixes;
