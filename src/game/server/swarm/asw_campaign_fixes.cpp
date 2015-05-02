#include "cbase.h"
#include "asw_door_area.h"
#include "func_movelinear.h"
#include "asw_button_area.h"
#include "asw_base_spawner.h"
#include "asw_marine_hint.h"
#include "ai_network.h"
#include "ai_link.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar asw_follow_ignore_hints("asw_follow_ignore_hints", "0.1", FCVAR_CHEAT, "If less than (by default) 10% of nodes are marine hints, use all nodes as marine hints.");

// BenLubar: I really wish I didn't have to do this, but there are popular campaigns that have huge problems in them.
class CASW_Campaign_Fixes : public CAutoGameSystem
{
public:

	CASW_Campaign_Fixes() : CAutoGameSystem("CASW_Campaign_Fixes") {}

	virtual void LevelInitPostEntity()
	{
		const char *pszMap = STRING(gpGlobals->mapname);

		// Fixes cargo elevator leaving without all the marines if at least 4 marines are already on the elevator.
		if (!V_stricmp(pszMap, "asi-jac1-landingbay_02") || !V_stricmp(pszMap, "asi-jac1-landingbay_pract"))
		{
			CASW_Door_Area *pMarinesPast = dynamic_cast<CASW_Door_Area *>(gEntList.FindEntityByName(NULL, "trigger_lift_marine_check"));
			Assert(pMarinesPast);
			if (pMarinesPast)
			{
				pMarinesPast->m_nPlayersRequired = ASW_MAX_MARINE_RESOURCES;
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

		// Fixes the tech marine requirement never being turned off after the last hack.
		if (!V_stricmp(pszMap, "dc2-breaking_an_entry"))
		{
			CASW_Button_Area *pEndButton = NULL;
			while ((pEndButton = dynamic_cast<CASW_Button_Area *>(gEntList.FindEntityByClassname(pEndButton, "trigger_asw_button_area"))))
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
	}
};

static CASW_Campaign_Fixes g_CampaignFixes;