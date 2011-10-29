#include "cbase.h"
#include "asw_director.h"
#include "asw_game_resource.h"
#include "asw_marine_resource.h"
#include "asw_marine.h"
#include "asw_player.h"
#include "mod_player_performance.h"
#include "mod_player_performance_calculators.h"
#include "asw_marine_profile.h"
#include <vector>



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace std;

//returned when Performance can not be calculated
#define DEFAULT_PERFORMANCE 1

//Multiplies the average accuracy.  If averageAccuracy is 25% and ACCURACY_MODIFIER is 4
//CalculateAccuracyRating will return a 100 (perfect score)
#define ACCURACY_MODIFIER 1

//Amount of Friendly Fire per Marine that's allowed before a penalty
#define FRIENDLYFIRE_HANDICAP 10.0

#define FRIENDLYFIRE_MULTIPLIER 3.0

#define MAX_DIRECTOR_STRESS_HISTORY_SIZE 2024

#define DIRECTOR_STRESS_MULTIPLIER 4.5

//Couldn't get bots or multiplayer to work properly, so change how score is calculated
#define SINGLE_PLAYER_MODE true

//This counters that effect of stress, end of levels
//usually have 'finales' which result in the players
//being 100% stressed. This reduces the overall impact
//of prolonged high stress on performance metrics.
#define STRESS_MULTIPLIER 0.3

ConVar mod_player_performance_debug("mod_player_performance_debug", "0", 0, "Displays modPlayerPerformance status on screen");

ConVar mod_player_performance_force_value("mod_player_performance_force_value", "0", 0, "Permantently sets modPlayerPerformance, overriding the calculated value.");

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
	m_previousRating = 0;

	g_calculators = new vector<CMOD_Player_Performance_Calculator*>();	
	g_calculators->push_back(new CMOD_Player_Performance_Calculator_Health(SINGLE_PLAYER_MODE));
	g_calculators->push_back(new CMOD_Player_Performance_Calculator_Accuracy(SINGLE_PLAYER_MODE));	
	g_calculators->push_back(new CMOD_Player_Performance_Calculator_DirectorStress(SINGLE_PLAYER_MODE));
	g_calculators->push_back(new CMOD_Player_Performance_Calculator_PlayTime(SINGLE_PLAYER_MODE));

	if (SINGLE_PLAYER_MODE)
		g_calculators->push_back(new CMOD_Player_Performance_Calculator_FriendlyFire(SINGLE_PLAYER_MODE));
}

CMOD_Player_Performance::~CMOD_Player_Performance()
{
	if (g_calculators)
		delete g_calculators;
}

//Couldn't figure out how to nativally hook into this event, 
//so asw_director.cpp calls 
void CMOD_Player_Performance::OnMissionStarted(){		
	for (unsigned int i = 0; i < g_calculators->size(); i++)
	{
		g_calculators->at(i)->OnMissionStarted();
	}
}

//Couldn't figure out how to nativally hook into this event, 
//so asw_director.cpp calls 
void CMOD_Player_Performance::FrameUpdatePostEntityThink()
{	
	int rating = CalculatePerformanceButDoNotUpdateHUD(false);

	//if (rating != m_previousRating) - WriteToHUD was firing before HUD was ready to receive, so HUD would
	//be out of date on level start.
		WriteToHUD("MODPlayerPerformance", rating);

	m_previousRating = rating;


	if ( mod_player_performance_debug.GetInt() > 0 )
	{		
		PrintDebug();		
	}
}

void CMOD_Player_Performance::Event_AlienKilled( CBaseEntity *pAlien, const CTakeDamageInfo &info )
{
	for (unsigned int i = 0; i < g_calculators->size(); i++)
	{
		g_calculators->at(i)->Event_AlienKilled(pAlien, info);
	}
}

int CMOD_Player_Performance::CalculatePerformance(bool isEndOfLevel)
{
	WriteToHUD("MODPlayerPerformanceDynamicContent", -1);
	return CalculatePerformanceButDoNotUpdateHUD(isEndOfLevel);
}

int CMOD_Player_Performance::CalculatePerformanceButDoNotUpdateHUD(bool isEndOfLevel)
{
	//Performance is 25% health, 25% accuracy, 
	//25% friendly fire, 25% average director stress 

	//if (mod_player_performance_force_value.GetInt() > 0)
	//{
		//Don't return here, go ahead and calculate the value in case debug is on.
	//}

	CASW_Game_Resource *pGameResource = ASWGameResource();

	if (!pGameResource)
	{
		Msg("Failed to calculate performance: Couldn't get ASWGameResource");
		return DEFAULT_PERFORMANCE;
	}

	/*
	m_healthRating = CalculateHealthRating(pGameResource);	
	m_accuracyRating = CalculateAccuracyRating(pGameResource);	
	m_friendlyFireRating = CalculateFriendFireRating(pGameResource);	
	m_directorStressRating = CalculateDirectorStress(pGameResource);
	
	if (SINGLE_PLAYER_MODE)
	{
		m_totalRating = m_healthRating + m_accuracyRating + m_directorStressRating;
		m_totalRating /= 3;
	}
	else
	{
		m_totalRating = m_healthRating + m_accuracyRating + m_friendlyFireRating + m_directorStressRating;
		m_totalRating /= 4;
	}
	
	*/

	int m_totalRating = 100;
	for (unsigned int i = 0; i < g_calculators->size(); i++)
	{
		g_calculators->at(i)->UpdatePerformance(&m_totalRating, isEndOfLevel, pGameResource);
	}
	
	if (m_totalRating > 80)
		m_weightedRating = 3;
	else if (m_totalRating > 60)
		m_weightedRating = 2;
	else 
		m_weightedRating = 1;	

	if (mod_player_performance_force_value.GetInt() > 0)
	{		
		return mod_player_performance_force_value.GetInt();
	}
	else
	{	
		return m_weightedRating;
	}
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

				m_playerZeroAccuracy = acc;				

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

	m_averageStressOfPlayers = 0;
	for ( int i=0;i<pGameResource->GetMaxMarineResources();i++ )
	{
		CASW_Marine_Resource *pMR = pGameResource->GetMarineResource(i);
		if ( !pMR )
			continue;

		m_averageStressOfPlayers += (pMR->GetIntensity()->GetCurrent() * 100.0 * STRESS_MULTIPLIER);
	}
	if (!SINGLE_PLAYER_MODE)
	{
		m_averageStressOfPlayers /= pGameResource->GetMaxMarineResources();
	}


	
	g_directorStressHistory->push_back(m_averageStressOfPlayers);

	m_averageStressHistory = 0;
	for (unsigned int i = 0; i < g_directorStressHistory->size(); i++)
	{
		m_averageStressHistory += g_directorStressHistory->at(i);
	}

	m_averageStressHistory /= g_directorStressHistory->size();
	
	return 100 - (int)(m_averageStressHistory * DIRECTOR_STRESS_MULTIPLIER);
}

void CMOD_Player_Performance::PrintDebug()
{
	int screenOffset = 0;

	engine->Con_NPrintf(screenOffset++,"Players Performance: %d", m_weightedRating);
	for (unsigned int i = 0; i < g_calculators->size(); i++)	
	{
		g_calculators->at(i)->PrintDebugString( screenOffset++ );		
	}	
	screenOffset++;
	for (unsigned int i = 0; i < g_calculators->size(); i++)
	{
		if (g_calculators->at(i)->HasExtraDebugInfo())
		{
			g_calculators->at(i)->PrintExtraDebugInfo(screenOffset++);			
		}
	}
	
	/*
	engine->Con_NPrintf(2,"Players Health Rating: %d", m_healthRating);
	engine->Con_NPrintf(3,"Players Accuracy Rating: %d", m_accuracyRating);
	if (!SINGLE_PLAYER_MODE)
	{
		engine->Con_NPrintf(4,"Players Friendly Fire Rating: %d", m_friendlyFireRating);
	}
	engine->Con_NPrintf(5,"Players Director Stress Rating: %d", m_directorStressRating);
	engine->Con_NPrintf(7,"Players Total Rating: %d", m_totalRating);	

	engine->Con_NPrintf(11,"Player %d accuracy: %f", m_playerZeroAccuracy);
	engine->Con_NPrintf(12,"Average stress: %f", m_averageStressOfPlayers);
	engine->Con_NPrintf(13,"Average historical stress: %f", m_averageStressHistory);
	*/

	if (mod_player_performance_force_value.GetInt() > 0)
	{
		engine->Con_NPrintf(screenOffset++,"Player Forced Rating: ON [%d]", mod_player_performance_force_value.GetInt());
	}
	else
	{
		engine->Con_NPrintf(screenOffset++,"Player Forced Rating: OFF");
	}	
}

void CMOD_Player_Performance::WriteToHUD(const char* messagename, int rating)
{	
	for ( int i=0;i<ASWGameResource()->GetMaxMarineResources();i++ )
	{
		CASW_Marine_Resource *pMR = ASWGameResource()->GetMarineResource(i);
		if ( !pMR )
			continue;
				
		CASW_Player *pPlayer = pMR->GetCommander();
	
		if ( pPlayer && pPlayer->GetMarine() )
		{		
			CSingleUserRecipientFilter user( pPlayer );
			UserMessageBegin( user, messagename );
			WRITE_SHORT( rating );
			MessageEnd();			
		}	
	}
}
