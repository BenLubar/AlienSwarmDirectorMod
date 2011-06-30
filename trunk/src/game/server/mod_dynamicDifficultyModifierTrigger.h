#ifndef _INCLUDED_MOD_DYNAMICDIFFICULTYMODIFIERTRIGGER_H
#define _INCLUDED_MOD_DYNAMICDIFFICULTYMODIFIERTRIGGER_H

#include "triggers.h"

//Just like a CASW_Marine_Position_Trigger
//but only fires if the players are within the 
//difficulty threshold.
class CMOD_Dynamic_Difficulty_Modifier_Trigger : public CBaseTrigger
{
	DECLARE_CLASS ( CMOD_Dynamic_Difficulty_Modifier_Trigger, CBaseTrigger);
public:		
	void PositionTouch(CBaseEntity *pOther);
	void ActivatePositionTrigger(CBaseEntity *pActivator);
	void Spawn( void );	
	void MultiWaitOver( void );

	DECLARE_DATADESC();
	
	//float m_fDesiredFacing;
	//float m_fTolerance;

	bool m_bHasCheckedDifficulty;
	bool m_bDifficultyThresholdReached;
	//0 = none, 1 = easy, 2 = medium, 3 = hard
	int m_iMinDifficultyThreshold;
	int m_iMaxDifficultyThreshold;

	int m_iDifficultyLevelOfMarines;

	

	// Outputs
	COutputEvent m_OnTrigger;
	//COutputEvent m_OnMarineInPosition;
	//COutputEvent m_OnMarineOutOfPosition;
	COutputEvent m_OnAlwaysTriggerEasy;
	COutputEvent m_OnAlwaysTriggerMedium;
	COutputEvent m_OnAlwaysTriggerHard;

	EHANDLE m_hMarine;
private:
	bool PerformDifficultyCheck(void);
	int GetDifficultyLevelOfMarines(void);
};




#endif
