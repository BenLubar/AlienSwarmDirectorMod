#include "cbase.h"
#include "mod_dynamicDifficultyModifierTrigger.h"
#include "asw_marine.h"
#include "asw_game_resource.h"
#include "asw_marine_resource.h"
#include "asw_marine_speech.h"
#include "asw_gamerules.h"
#include "asw_weapon.h"
#include "asw_util_shared.h"
#include "asw_melee_system.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(mod_dynaimic_difficulty_modifier_trigger, CMOD_Dynamic_Difficulty_Modifier_Trigger);

BEGIN_DATADESC( CMOD_Dynamic_Difficulty_Modifier_Trigger )
	//DEFINE_KEYFIELD( m_fDesiredFacing,			FIELD_FLOAT,	"DesiredFacing" ),
	//DEFINE_KEYFIELD( m_fTolerance, FIELD_FLOAT, "Tolerance" ),	
	//DEFINE_FIELD(m_bHasCheckedDifficulty, FIELD_BOOLEAN),
	//DEFINE_FIELD(m_bDifficultyThresholdReached, FIELD_BOOLEAN),
	DEFINE_KEYFIELD(m_iMinDifficultyThreshold, FIELD_INTEGER, "minDifficultyThreshold"),
	DEFINE_KEYFIELD(m_iMaxDifficultyThreshold, FIELD_INTEGER, "maxDifficultyThreshold"),
	// Function Pointers
	DEFINE_FUNCTION(PositionTouch),
	DEFINE_FUNCTION(MultiWaitOver ),

	// Outputs
	DEFINE_OUTPUT(m_OnTrigger, "OnTrigger"),
	//DEFINE_OUTPUT(m_OnMarineInPosition, "MarineInPosition"),
	//DEFINE_OUTPUT(m_OnMarineOutOfPosition, "MarineOutOfPosition"),	
	DEFINE_OUTPUT(m_OnAlwaysTriggerEasy, "OnAlwaysTriggerEasy"),
	DEFINE_OUTPUT(m_OnAlwaysTriggerMedium, "OnAlwaysTriggerMedium"),
	DEFINE_OUTPUT(m_OnAlwaysTriggerHard, "OnAlwaysTriggerHard")
	
END_DATADESC()

void CMOD_Dynamic_Difficulty_Modifier_Trigger::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	if (m_flWait == 0)
	{
		m_flWait = 0.2;
	}


	m_bHasCheckedDifficulty = false;
	m_bDifficultyThresholdReached = false;
	m_iDifficultyLevelOfMarines = -1;
	m_hMarine = NULL;

	//ASSERTSZ(m_iHealth == 0, "trigger_multiple with health");
	SetTouch( &CMOD_Dynamic_Difficulty_Modifier_Trigger::PositionTouch );
}


//-----------------------------------------------------------------------------
// Purpose: Touch function. Activates the trigger.
// Input  : pOther - The thing that touched us.
//-----------------------------------------------------------------------------
void CMOD_Dynamic_Difficulty_Modifier_Trigger::PositionTouch(CBaseEntity *pOther)
{	
	if (PassesTriggerFilters(pOther) && pOther->Classify() == CLASS_ASW_MARINE)
	{
		ActivatePositionTrigger( pOther );
	}
}

void CMOD_Dynamic_Difficulty_Modifier_Trigger::ActivatePositionTrigger(CBaseEntity *pActivator)
{
	if (GetNextThink() > gpGlobals->curtime)
		return;         // still waiting for reset time

	m_hActivator = pActivator;

	if (!m_bHasCheckedDifficulty)
	{
		m_bHasCheckedDifficulty = true;
		m_iDifficultyLevelOfMarines = GetDifficultyLevelOfMarines();
		m_bDifficultyThresholdReached = PerformDifficultyCheck();

		if (m_bDifficultyThresholdReached)		
			m_OnTrigger.FireOutput(m_hActivator, this);	

		switch (m_iDifficultyLevelOfMarines)
		{
			case 1: 
				m_OnAlwaysTriggerEasy.FireOutput(m_hActivator, this);					
				break;
			case 2:
				m_OnAlwaysTriggerMedium.FireOutput(m_hActivator, this);	
				break;
			case 3:
				m_OnAlwaysTriggerHard.FireOutput(m_hActivator, this);	
				break;
		}		
	}
			
	SetThink( &CMOD_Dynamic_Difficulty_Modifier_Trigger::MultiWaitOver );
	SetNextThink( gpGlobals->curtime + 0.1f );	
}

void CMOD_Dynamic_Difficulty_Modifier_Trigger::MultiWaitOver( void )
{
	if (m_hTouchingEntities.Count() <= 0)
	{
		SetThink( NULL );		// no more marines inside us, so clear think and allow retrigger
		if (m_hMarine.Get())
		{			
			m_hMarine = NULL;
		}
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.1f );		// constantly check up on the marines inside us, NOTE: no more OnTriggers will fire while doing this
	}
}

int CMOD_Dynamic_Difficulty_Modifier_Trigger::GetDifficultyLevelOfMarines( void )
{
	return 2;
}

bool CMOD_Dynamic_Difficulty_Modifier_Trigger::PerformDifficultyCheck( void )
{
	if (m_iMinDifficultyThreshold <= 0 && m_iMaxDifficultyThreshold >= m_iDifficultyLevelOfMarines)
		return true;
	else if (m_iMaxDifficultyThreshold <= 0 && m_iMinDifficultyThreshold <= m_iDifficultyLevelOfMarines)
		return true;
	else if (m_iMinDifficultyThreshold <= m_iDifficultyLevelOfMarines
			&& m_iMaxDifficultyThreshold >= m_iDifficultyLevelOfMarines)
		return true;	
	else
		return false;
}







/*
class CModDynamicDifficultyModifierTrigger : public CBaseTrigger
{
public:
	DECLARE_CLASS ( CModDynamicDifficultyModifierTrigger, CBaseTrigger);

	DECLARE_DATADESC();

	//Constructor
	CModDynamicDifficultyModifierTrigger()
	{
		Msg("Hello World from Dynamic Difficulty Trigger\n");

		m_bHasCheckedDifficulty = false;
		m_bDifficultyThresholdReached = false;
		m_iMinDifficultyThreshold = 1;
		m_iMaxDifficultyThreshold = 3;
	}

	//Input function
	void CheckDifficultyThreshold (inputdata_t &inputData);
	void ParseDifficultyThreshold();

private:
		bool m_bHasCheckedDifficulty;
		bool m_bDifficultyThresholdReached;
		//1 = easy, 2 = medium, 3 = hard
		int m_iMinDifficultyThreshold;
		int m_iMaxDifficultyThreshold;

		COutputEvent m_OnGetDifficultyThreshold;
};


LINK_ENTITY_TO_CLASS(mod_dynaimic_difficulty_modifier_trigger, CModDynamicDifficultyModifierTrigger);

BEGIN_DATADESC (CModDynamicDifficultyModifierTrigger)
	// For save/load
	DEFINE_FIELD(m_bHasCheckedDifficulty, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bDifficultyThresholdReached, FIELD_BOOLEAN),

	//Link Member Variable to a Hammer keyvalue
	DEFINE_KEYFIELD(m_iMinDifficultyThreshold, FIELD_INTEGER, "minDifficultyThreshold"),
	DEFINE_KEYFIELD(m_iMaxDifficultyThreshold, FIELD_INTEGER, "maxDifficultyThreshold"),

	//Links out input name from Hammer to our input member
	DEFINE_INPUTFUNC(FIELD_VOID, "CheckDifficulty", CheckDifficultyThreshold),

	//Links our output member variable to the output name
	DEFINE_OUTPUT(m_OnGetDifficultyThreshold, "OnGetDifficultyThreshold")

END_DATADESC()

void  CModDynamicDifficultyModifierTrigger::ParseDifficultyThreshold()
{
	if (!m_bHasCheckedDifficulty)
	{
		//TODO: Actually check AI system instead of using random
		int marinesPerformance = (rand() % 3) + 1;

		Msg("Marines Performance: [%i]\n", marinesPerformance);
		
		m_bDifficultyThresholdReached = ((m_iMaxDifficultyThreshold <= marinesPerformance) && (m_iMinDifficultyThreshold >= marinesPerformance));

		m_bHasCheckedDifficulty = true;
	}
}

void CModDynamicDifficultyModifierTrigger::CheckDifficultyThreshold (inputdata_t &inputData)
{
	ParseDifficultyThreshold();

	if (m_bDifficultyThresholdReached)
		m_OnGetDifficultyThreshold.FireOutput(inputData.pActivator, this);
}

*/