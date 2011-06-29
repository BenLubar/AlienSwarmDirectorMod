#include "cbase.h"
#include "triggers.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CModDynamicDifficultyModifierTrigger : public CBaseTrigger
{
public:
	DECLARE_CLASS ( CModDynamicDifficultyModifierTrigger, CBaseTrigger);

	DECLARE_DATADESC();

	//Constructor
	CModDynamicDifficultyModifierTrigger()
	{
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