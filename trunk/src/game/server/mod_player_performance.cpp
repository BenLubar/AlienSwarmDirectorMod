#include "cbase.h"
#include "asw_director.h"
#include "asw_game_resource.h"
#include "asw_marine_resource.h"
#include "asw_marine.h"
#include "mod_player_performance.h"
#include <vector>


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace std;

//returned when Performance can not be calculated
#define DEFAULT_PERFORMANCE 1

//Multiplies the average accuracy.  If averageAccuracy is 25% and ACCURACY_MODIFIER is 4
//CalculateAccuracyRating will return a 100 (perfect score)
#define ACCURACY_MODIFIER 1.5

//Amount of Friendly Fire per Marine that's allowed before a penalty
#define FRIENDLYFIRE_HANDICAP 10.0

#define FRIENDLYFIRE_MULTIPLIER 3

#define MAX_DIRECTOR_STRESS_HISTORY_SIZE 2024

//Couldn't get bots or multiplayer to work properly, so change how score is calculated
#define SINGLE_PLAYER_MODE true

//This counters that effect of stress, end of levels
//usually have 'finales' which result in the players
//being 100% stressed. This reduces the overall impact
//of prolonged high stress on performance metrics.
#define STRESS_MULTIPLIER 0.3

ConVar mod_player_performance_debug("mod_player_performance_debug", "0", 0, "Displays modPlayerPerformance status on screen");

CMOD_Player_Performance* CMOD_Player_Performance::g_PlayerPerformanceSingleton = 0;

CMOD_Player_Performance* CMOD_Player_Performance::PlayerPerformance()
{	
	if (!g_PlayerPerformanceSingleton)
	{
		g_PlayerPerformanceSingleton = new CMOD_Player_Performance();
	}

	return g_PlayerPerformanceSingleton;
}

CMOD_Player_Performance::CMOD_Player_Performance( void ) : CAutoGameSystemPerFrame( "CMOD_Player_Performance" )
{
	
}

CMOD_Player_Performance::~CMOD_Player_Performance()
{
	
}

//Couldn't figure out how to nativally hook into this event, 
//so asw_director.cpp calls 
void CMOD_Player_Performance::OnMissionStarted(){	
	//probably a memory leak
	g_directorStressHistory = new vector<double>();	
}

//Couldn't figure out how to nativally hook into this event, 
//so asw_director.cpp calls 
void CMOD_Player_Performance::FrameUpdatePostEntityThink()
{
	if ( mod_player_performance_debug.GetInt() > 0 )
	{
		engine->Con_NPrintf(1,"Players Performance: %d", CalculatePerformance());
	}
}


int CMOD_Player_Performance::CalculatePerformance()
{
	//Performance is 25% health, 25% accuracy, 
	//25% friendly fire, 25% average director stress 

	CASW_Game_Resource *pGameResource = ASWGameResource();

	int totalRating, healthRating, accuracyRating, friendlyFireRating, directorStressRating = 0;

	if (!pGameResource)
	{
		Msg("Failed to calculate performance: Couldn't get ASWGameResource");
		return DEFAULT_PERFORMANCE;
	}

	healthRating = CalculateHealthRating(pGameResource);
	engine->Con_NPrintf(2,"Players Health Rating: %d", healthRating);

	accuracyRating = CalculateAccuracyRating(pGameResource);
	engine->Con_NPrintf(3,"Players Accuracy Rating: %d", accuracyRating);

	friendlyFireRating = CalculateFriendFireRating(pGameResource);
	engine->Con_NPrintf(4,"Players Friendly Fire Rating: %d", friendlyFireRating);

	directorStressRating = CalculateDirectorStress(pGameResource);
	engine->Con_NPrintf(5,"Players Director Stress Rating: %d", directorStressRating);

	if (SINGLE_PLAYER_MODE)
	{
		totalRating = healthRating + accuracyRating + directorStressRating;
		totalRating /= 3;
	}
	else
	{
		totalRating = healthRating + accuracyRating + friendlyFireRating + directorStressRating;
		totalRating /= 4;
	}
	
	engine->Con_NPrintf(7,"Players Total Rating: %d", totalRating);

	if (totalRating > 75)
		return 3;
	else if (totalRating > 55)
		return 2;
	else 
		return 1;	
}

//Return the average health of all players
int CMOD_Player_Performance::CalculateHealthRating(CASW_Game_Resource *pGameResource)
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

	if (!SINGLE_PLAYER_MODE)
	{
		averageHealth /= pGameResource->GetMaxMarineResources();
	}
	return averageHealth;
}

int CMOD_Player_Performance::CalculateAccuracyRating(CASW_Game_Resource *pGameResource){
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

				engine->Con_NPrintf(10 + i,"Player %d accuracy: %f",i, acc);

				averageAccuracy += acc;

				playersThatHaveFired++;
		}
	}


	if (averageAccuracy == 0)
	{
		//No one has fired a shot, so perfect accuracy.
		return 100;
	}

	averageAccuracy /= playersThatHaveFired;
	averageAccuracy *= ACCURACY_MODIFIER;

	if (averageAccuracy > 100.0)
		return 100;
	else
		return (int)averageAccuracy;

}

int CMOD_Player_Performance::CalculateFriendFireRating(CASW_Game_Resource *pGameResource)
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
	
	return 100-(int)averageFriendlyFire;
}

int CMOD_Player_Performance::CalculateDirectorStress(CASW_Game_Resource *pGameResource)
{
	//make room in the history if needed
	if (g_directorStressHistory->size() > MAX_DIRECTOR_STRESS_HISTORY_SIZE)
	{
		g_directorStressHistory->erase(g_directorStressHistory->begin());
	}

	double averageStressOfPlayers = 0;
	for ( int i=0;i<pGameResource->GetMaxMarineResources();i++ )
	{
		CASW_Marine_Resource *pMR = pGameResource->GetMarineResource(i);
		if ( !pMR )
			continue;

		averageStressOfPlayers += (pMR->GetIntensity()->GetCurrent() * 100.0 * STRESS_MULTIPLIER);
	}
	if (!SINGLE_PLAYER_MODE)
	{
		averageStressOfPlayers /= pGameResource->GetMaxMarineResources();
	}

	engine->Con_NPrintf(17,"Average stress: %f", averageStressOfPlayers);
	g_directorStressHistory->push_back(averageStressOfPlayers);

	double averageStressHistory = 0;
	for (int i = 0; i < g_directorStressHistory->size(); i++)
	{
		averageStressHistory += g_directorStressHistory->at(i);
	}

	averageStressHistory /= g_directorStressHistory->size();
	engine->Con_NPrintf(18,"Average historical stress: %f", averageStressHistory);
	return 100 - (int)averageStressHistory;
}
