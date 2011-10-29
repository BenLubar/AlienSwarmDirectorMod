#include "cbase.h"
#include "mod_player_performance_calculators.h"

#include "asw_director.h"
#include "asw_game_resource.h"
#include "asw_marine_resource.h"
#include "asw_marine.h"
#include "asw_player.h"
#include "mod_player_performance.h"
#include "asw_marine_profile.h"

#include <vector>
using namespace std;

//Multiplies the average accuracy.  If averageAccuracy is 25% and ACCURACY_MODIFIER is 4
//CalculateAccuracyRating will return a 100 (perfect score)
#define ACCURACY_MODIFIER 1

//Amount of Friendly Fire per Marine that's allowed before a penalty
#define FRIENDLYFIRE_HANDICAP 10.0

#define FRIENDLYFIRE_MULTIPLIER 3.0

#define MAX_DIRECTOR_STRESS_HISTORY_SIZE 2024

#define DIRECTOR_STRESS_MULTIPLIER 4.5

//This counters that effect of stress, end of levels
//usually have 'finales' which result in the players
//being 100% stressed. This reduces the overall impact
//of prolonged high stress on performance metrics.
#define STRESS_MULTIPLIER 0.3

void CMOD_Player_Performance_Calculator::PrintDebugString(int offset)
{	
	engine->Con_NPrintf(offset, "Player %s Rating: %d", m_DebugName, GetDebugValue());	
}

//////////////////////////////////////////////////////////////////////////////////

void CMOD_Player_Performance_Calculator_Health::UpdatePerformance(int * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
{
	int averageHealth = 0;

	for ( int i=0;i<pGameResource->GetMaxMarineResources();i++ )
	{
		CASW_Marine_Resource *pMR = pGameResource->GetMarineResource(i);
		if ( !pMR )
			continue;

		CASW_Marine *pMarine = pMR->GetMarineEntity();
		if ( !pMarine || pMarine->GetHealth() <= 0 )
			continue;

		averageHealth += pMarine->GetHealth();
	}

	if (!IsSinglePlayerMode())
	{
		averageHealth /= pGameResource->GetMaxMarineResources();
	}

	//return averageHealth;
	
}

//////////////////////////////////////////////////////////////////////////////////

void CMOD_Player_Performance_Calculator_Accuracy::PrintExtraDebugInfo(int offset)
{
	engine->Con_NPrintf(offset,"Player 1 Accuracy [%0.3f]", m_PlayerZeroAccuracy);
}

void CMOD_Player_Performance_Calculator_Accuracy::UpdatePerformance(int * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
{
	float averageAccuracy = 0;
	float playersThatHaveFired = 0; 

	for ( int i=0;i<pGameResource->GetMaxMarineResources();i++ )
	{
		CASW_Marine_Resource *pMR = pGameResource->GetMarineResource(i);
		if ( !pMR )
			continue;

		float acc = 0;
		if (pMR->m_iPlayerShotsFired > 0)
		{				
				acc = float(pMR->m_iPlayerShotsFired - pMR->m_iPlayerShotsMissed) / float(pMR->m_iPlayerShotsFired);
				acc *= 100.0f;

				m_PlayerZeroAccuracy = acc;				

				averageAccuracy += acc;

				playersThatHaveFired++;			
		}
	}


	if (averageAccuracy == 0)
	{
		//No one has fired a shot, so perfect accuracy.
//		return 100;
	}

	averageAccuracy /= playersThatHaveFired;
	averageAccuracy *= ACCURACY_MODIFIER;

	//if (averageAccuracy > 100.0)
	//	return 100;
	//else
	//	return (int)averageAccuracy;
	
}

//////////////////////////////////////////////////////////////////////////////////

void CMOD_Player_Performance_Calculator_FriendlyFire::UpdatePerformance(int * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
{
	float averageFriendlyFire = 0;

	for ( int i=0;i<pGameResource->GetMaxMarineResources();i++ )
	{
		CASW_Marine_Resource *pMR = pGameResource->GetMarineResource(i);
		if ( !pMR )
			continue;

		if (pMR->m_fFriendlyFireDamageDealt > FRIENDLYFIRE_HANDICAP)
			averageFriendlyFire += pMR->m_fFriendlyFireDamageDealt - FRIENDLYFIRE_HANDICAP;
		else
			averageFriendlyFire += pMR->m_fFriendlyFireDamageDealt;
	}

	if (averageFriendlyFire > 0)
	{
		averageFriendlyFire /= pGameResource->GetMaxMarineResources();
		averageFriendlyFire *= FRIENDLYFIRE_MULTIPLIER;
	}
	
	//return 100-(int)averageFriendlyFire;
}

//////////////////////////////////////////////////////////////////////////////////


void CMOD_Player_Performance_Calculator_DirectorStress::PrintExtraDebugInfo(int offset)
{
	engine->Con_NPrintf(offset, "Current Stress [%0.3f] Historical Stress: [%0.3f]", m_averageStressOfPlayers, m_averageStressHistory);	
}

void CMOD_Player_Performance_Calculator_DirectorStress::OnMissionStarted()
{
	//probably a memory leak
	g_directorStressHistory = new vector<double>();	
}

void CMOD_Player_Performance_Calculator_DirectorStress::UpdatePerformance(int * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
{
	//make room in the history if needed
	if (g_directorStressHistory->size() > MAX_DIRECTOR_STRESS_HISTORY_SIZE)
	{
		g_directorStressHistory->erase(g_directorStressHistory->begin());
	}

	m_averageStressOfPlayers = 0;
	m_averageStressHistory = 0;

	for ( int i=0;i<pGameResource->GetMaxMarineResources();i++ )
	{
		CASW_Marine_Resource *pMR = pGameResource->GetMarineResource(i);
		if ( !pMR )
			continue;

		m_averageStressOfPlayers += (pMR->GetIntensity()->GetCurrent() * 100.0 * STRESS_MULTIPLIER);
	}
	if (!IsSinglePlayerMode())
	{
		m_averageStressOfPlayers /= pGameResource->GetMaxMarineResources();
	}

	g_directorStressHistory->push_back(m_averageStressOfPlayers);

	m_averageStressOfPlayers = 0;
	for (unsigned int i = 0; i < g_directorStressHistory->size(); i++)
	{
		m_averageStressHistory += g_directorStressHistory->at(i);
	}

	m_averageStressHistory /= g_directorStressHistory->size();
	
	//return 100 - (int)(m_averageStressHistory * DIRECTOR_STRESS_MULTIPLIER);
}

//////////////////////////////////////////////////////////////////////////////////

void CMOD_Player_Performance_Calculator_PlayTime::UpdatePerformance(int * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
{
	if (!isEndOfLevel)
		return;
}