#include "cbase.h"
#include "asw_door_area.h"
#include "func_movelinear.h"
#include "asw_base_spawner.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// BenLubar: I really wish I didn't have to do this, but there are popular campaigns that have huge problems in them.
class CASW_Campaign_Fixes : public CAutoGameSystem
{
public:

	CASW_Campaign_Fixes() : CAutoGameSystem("CASW_Campaign_Fixes") {}

	virtual void LevelInitPostEntity()
	{
		const char *pszMap = STRING(gpGlobals->mapname);

		// Fixes cargo elevator leaving without all the marines if at least 4 marines are already on the elevator.
		if (!V_stricmp(pszMap, "asi-jac1-landingbay_02"))
		{
			CASW_Door_Area *pMarinesPast = dynamic_cast<CASW_Door_Area *>(gEntList.FindEntityByName(NULL, "trigger_lift_marine_check"));
			Assert(pMarinesPast);
			if (pMarinesPast)
				pMarinesPast->m_nPlayersRequired = ASW_MAX_MARINE_RESOURCES;
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
	}
};

static CASW_Campaign_Fixes g_CampaignFixes;