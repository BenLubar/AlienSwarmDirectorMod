#include "cbase.h"

#include "../shared/swarm/asw_gamerules.h"
#include "asw_campaign_info.h"
#include "c_asw_campaign_save.h"
#include "missionchooser/iasw_mission_chooser_source.h"
#include "missionchooser/iasw_random_missions.h"
#include "missionchooser/iasw_map_builder.h"
#include "../missionchooser/imod_level_builder.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CC_MOD_Build_Mission_Map_For_Next_Mission( const CCommand &args )
{
	if ( args.ArgC() < 2 )
	{
		Msg("Usage: mod_build_map <player performance>");
		return;
	}

	if (ASWGameRules()->CampaignMissionsLeft() <= 0)
	{
		//This is the last mission.
		return;
	}

	int iPlayerPerformance =  atoi(args.Arg(1));

	CASW_Campaign_Info *pCampaign = ASWGameRules()->GetCampaignInfo();		
	if (!pCampaign)
	{
		Msg("Failed to load Campaign with CAlienSwarm::GetCampaignInfo()\n");
		return;
	}
		
	CASW_Campaign_Save *pSave = ASWGameRules()->GetCampaignSave();
	if (!pSave)
	{
		Msg("Failed to load Campaign Save with CAlienSwarm::GetCampaignInfoGetCampaignSave()\n");
		return;
	}

	int iNextMission = pSave->m_iNumMissionsComplete + 1;	
	CASW_Campaign_Info::CASW_Campaign_Mission_t* pNextMission = 
		pCampaign->GetMission(iNextMission);
	if (!pNextMission)
	{
		Msg("Failed to load next campaign mission with pCampaign->GetMission(iNextMission)\n");
		return;
	}
	
	missionchooser->modLevel_Builder()->BuildMapForMissionFromLayoutFile(
		pNextMission->m_MapName, iPlayerPerformance);
}

static ConCommand asw_build_map("mod_build_mission_map_for_next_mission", CC_MOD_Build_Mission_Map_For_Next_Mission, 0 );