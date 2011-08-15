#include "cbase.h"
//#include "asw_objective_escape.h"
#include "mod_objective_escape.h"
#include "asw_game_resource.h"
#include "asw_marine_resource.h"
#include "asw_marine.h"
#include "triggers.h"
#include "mod_player_performance.h"

#include "missionchooser/iasw_random_missions.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( mod_objective_escape, CMOD_Objective_Escape );

BEGIN_DATADESC( CMOD_Objective_Escape )
	DEFINE_FIELD( m_hTrigger, FIELD_EHANDLE ),
	DEFINE_INPUTFUNC( FIELD_VOID, "MarineInEscapeArea", InputMarineInEscapeArea ),
END_DATADESC()

CUtlVector<CMOD_Objective_Escape*> g_aEscapeObjectives;

//CASW_Objective_Escape * m_aswEscape;

CMOD_Objective_Escape::CMOD_Objective_Escape() : CASW_Objective()
{	
	m_hTrigger = NULL;
	g_aEscapeObjectives.AddToTail( this );
	//m_aswEscape = new CASW_Objective_Escape();
}

CMOD_Objective_Escape::~CMOD_Objective_Escape()
{	
	g_aEscapeObjectives.FindAndRemove( this );
}

void CMOD_Objective_Escape::Spawn(){
	//m_aswEscape = new CASW_Objective_Escape();
}

void CMOD_Objective_Escape::InputMarineInEscapeArea( inputdata_t &inputdata ){
	Msg("Maine in Escape Area");

	CBaseTrigger* pTrig = dynamic_cast<CBaseTrigger*>(inputdata.pCaller);
	if (!pTrig)
	{
		Msg("Error: Escape objective input called by something that wasn't a trigger\n");
		return;
	}
	if (pTrig != GetTrigger() && GetTrigger()!= NULL)
	{
		Msg("Error: Escape objective input called by two different triggers.  Only 1 escape area is allowed per map.\n");
		return;
	}
	m_hTrigger = pTrig;

	CheckEscapeStatus();
}

void CMOD_Objective_Escape::CheckEscapeStatus()
{
	if (OtherObjectivesComplete() && AllLiveMarinesInExit())
	{
		//Dynamically build the map for the next mission.
		BuildMapForNextMission();

		//Fires ASW_Objective::OnObjectiveComplete
		Msg("Setting Objective to Complete\n");
		SetComplete(true);
	}
}

bool CMOD_Objective_Escape::OtherObjectivesComplete(){
	if ( !ASWGameResource() )
		return false;
	
	CASW_Game_Resource* pGameResource = ASWGameResource();
	for (int i=0;i<12;i++)
	{
		CASW_Objective* pObjective = pGameResource->GetObjective(i);
		// if another objective isn't optional and isn't complete, then we're not ready to escape
		if (pObjective && pObjective!=this
			&& !pObjective->IsObjectiveOptional() && !pObjective->IsObjectiveComplete())
		{
			Msg("Not all Objectives complete\n");
			return false;
		}
	}
	Msg("All Objectives complete\n");
	return true;
}
bool CMOD_Objective_Escape::AllLiveMarinesInExit()
{
	if ( !ASWGameResource() ||!GetTrigger() )
	{
		Msg("All Live Marines are NOT in Exit\n");
		return false;
	}
	
	CASW_Game_Resource* pGameResource = ASWGameResource();
	for (int i=0;i<pGameResource->GetMaxMarineResources();i++)
	{
		CASW_Marine_Resource* pMR = pGameResource->GetMarineResource(i);
		if (pMR && pMR->GetHealthPercent() > 0
			&& pMR->GetMarineEntity())
		{
			// we've got a live marine, check if he's in the exit area
			if (!GetTrigger()->IsTouching(pMR->GetMarineEntity()))
			{
				Msg("All Live Marines are NOT in Exit\n");
				return false;	// a live marine isn't in the exit zone
			}
		}
	}
	Msg("All Live Marines are in Exit\n");
	return true;
}

void CMOD_Objective_Escape::BuildMapForNextMission()
{
	CASW_Campaign_Info *pCampaign = CAlienSwarm::GetCampaignInfo();
	if (!pCampaign)
	{
		Msg("Failed to load Campaign with CAlienSwarm::GetCampaignInfo()\n");
		return;
	}

	CASW_Campaign_Save *pSave = CAlienSwarm::GetCampaignInfoGetCampaignSave();
	if (!pSave)
	{
		Msg("Failed to load Campaign Save with CAlienSwarm::GetCampaignInfoGetCampaignSave()\n");
		return;
	}

	int iNextMission = pSave->m_iNumMissionsComplete;
	CASW_Campaign_Mission_t* pNextMission = pCampaign->GetMission(iNextMission);
	if (!pNextMission)
	{
		Msg("Failed to load next campaign mission with pCampaign->GetMission(iNextMission)\n");
		return;
	}
	
	int iPlayerPerformance =  CMOD_Player_Performance::PlayerPerformance()->CalculatePerformance();	

	missionchooser->modLevel_Builder()->BuildMapForMissionFromLayoutFile(
		pNextMission->m_MapName.ToCStr(), iPlayerPerformance);

}

CBaseTrigger* CMOD_Objective_Escape::GetTrigger()
{
	return dynamic_cast<CBaseTrigger*>(m_hTrigger.Get());
}