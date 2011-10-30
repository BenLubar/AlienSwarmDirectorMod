#include "cbase.h"
#include "mod_player_performance_calculators.h"

#include "asw_director.h"
#include "asw_game_resource.h"
#include "asw_marine_resource.h"
#include "asw_marine.h"
#include "asw_player.h"
#include "mod_player_performance.h"
#include "asw_marine_profile.h"
#include "asw_alien.h"

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

ConVar mod_player_performance_health_critical_threshold("mod_player_performance_health_critical_threshold", "50", 0, "This threshold indicates if health_normal_penalty or health_critical_penalty is used to calculate health penalty.  Note: This is health lost, not health remaning");
ConVar mod_player_performance_health_normal_penalty("mod_player_performance_health_normal_penalty", "0.4", 0, "The defualt performance penalty for lost health.  This is multipled by each HP lost.");
ConVar mod_player_performance_health_critical_penalty("mod_player_performance_health_critical_penalty", "0.6", 0, "The critical performance penalty for lost health.  This is multipled by each HP lost.");
ConVar mod_player_performance_accuracy_threshold("mod_player_performance_accuracy_threshold", "92", 0, "The threshold where a players accuracy has a positive or negative impact on performance.  Above this threshold and performance gets a boost, below and there is a penalty.");
ConVar mod_player_performance_accuracy_floor("mod_player_performance_accuracy_floor", "75", 0, "The floor for caculating an accuracy penalty.  Ie, this is the lowest value that is used to assess a penalty.");
ConVar mod_player_performance_accuracy_performance_modifier("mod_player_performance_accuracy_performance_modifier", "0.8", 0, "Multipled against the players weighted accuracy to get the performance modifier. Weighted accuracy is difference between accuracy threshold and actual accuracy.");
ConVar mod_player_performance_director_stress_threshold("mod_player_performance_director_stress_threshold", "17", 0, "The threshold when stress, as measured by the director, engenders a player performance penalty.");
ConVar mod_player_performance_director_stress_penalty("mod_player_performance_director_stress_penalty", "0.7", 0, "The per stress point penalty assesed.");
ConVar mod_player_performance_playtime_par("mod_player_performance_playtime_par", "270", 0, "The average amount of time a player is expected to complete a level.");
ConVar mod_player_performance_playtime_divisor_bonus("mod_player_performance_playtime_par_bonus", "15", 0, "The boost to performance a player receives for completing a mission under par.  For each '60' seconds under par, the player's performance is increased by 1 point.");
ConVar mod_player_performance_enemy_life_time_positive_threshold("mod_player_performance_enemy_life_time_positive_threshold", "7", 0, "If the average enemy life expectency is less than this, a bonus is awarded.");
ConVar mod_player_performance_enemy_life_time_positive_bonus("mod_player_performance_enemy_life_time_positive_bonus", "1.0", 0, "The bonus to award if enemy life expectency is less than the positive threshold.");
ConVar mod_player_performance_enemy_life_time_negative_threshold("mod_player_performance_enemy_life_time_negative_threshold", "12", 0, "If the average enemy life expectency is more than this, a penalty is assesed.");
ConVar mod_player_performance_enemy_life_time_negative_penalty("mod_player_performance_enemy_life_time_negative_penalty", "1.0", 0, "The penalty to asses if enemy life expectency is more than the negative threshold.");
ConVar mod_player_performance_fastreload_bonus("mod_player_performance_fastreload_bonus", "3", 0, "The boost to performance a player receives for each fast reload.");
ConVar mod_player_performance_dodgeranger_bonus("mod_player_performance_dodgeranger_bonus", "5", 0, "The boost to performance a player receives for dodging a ranger projectile.");
ConVar mod_player_performance_meleekill_bonus("mod_player_performance_meleekill_bonus", "1.0", 0, "The boost to performance a player receives for each melee kill.");
ConVar mod_player_performance_boomer_kill_early_bonus("mod_player_performance_boomer_kill_early_bonus", "7", 0, "The boost to performance a player receives for killing a boomer before it explodes.");


void CMOD_Player_Performance_Calculator::PrintDebugString(int offset)
{	
	engine->Con_NPrintf(offset, "Player %s Rating: [%0.2f]", m_DebugName, GetDebugValue());	
}

//////////////////////////////////////////////////////////////////////////////////

void CMOD_Player_Performance_Calculator_Health::OnMissionStarted()
{
	m_hasCalculatedFullHealth = false;
}
void CMOD_Player_Performance_Calculator_Health::UpdatePerformance(float * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
{
	m_averageHealth = 0;

	for ( int i=0;i<pGameResource->GetMaxMarineResources();i++ )
	{
		CASW_Marine_Resource *pMR = pGameResource->GetMarineResource(i);
		if ( !pMR )
			continue;

		CASW_Marine *pMarine = pMR->GetMarineEntity();
		if ( !pMarine || pMarine->GetHealth() <= 0 )
			continue;

		m_averageHealth += pMarine->GetHealth();				
	}

	if (!IsSinglePlayerMode())
	{
		m_averageHealth /= pGameResource->GetMaxMarineResources();
	}

	//store full health for later.
	if ((!m_hasCalculatedFullHealth) && (m_averageHealth > 0))
	{
		m_fullHealth = m_averageHealth;
		m_hasCalculatedFullHealth = true;
	}

	float healthLost = (float)(m_fullHealth - m_averageHealth);
	float penalty = 0;

	if (healthLost >= mod_player_performance_health_critical_threshold.GetInt())
	{
		penalty = healthLost * mod_player_performance_health_critical_penalty.GetFloat();
	}	
	else
	{
		 penalty= healthLost * mod_player_performance_health_normal_penalty.GetFloat();
	}

	//multiply by negative 1 to make it a penalty
	penalty *=-1;

	m_LastCalculatedValue = penalty;
	*performance += m_LastCalculatedValue;
}
void CMOD_Player_Performance_Calculator_Health::PrintExtraDebugInfo(int offset)
{
	int healthLost = m_fullHealth - m_averageHealth;
	char * threshold = "normal";

	if (healthLost >= mod_player_performance_health_critical_threshold.GetInt())
		threshold = "critical";
		
	engine->Con_NPrintf(offset, "Average Health [%i] out of [%i] [%s]", m_averageHealth, m_fullHealth, threshold);
	
}

//////////////////////////////////////////////////////////////////////////////////

void CMOD_Player_Performance_Calculator_Accuracy::UpdatePerformance(float * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
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

				//This is used to print debug info.
				m_PlayerZeroAccuracy = acc;				

				averageAccuracy += acc;

				playersThatHaveFired++;			
		}		
	}
		
	if (averageAccuracy == 0)
	{
		//No one has fired a shot, so no modification.
		m_LastCalculatedValue = 0;
		return;
	}

	averageAccuracy /= playersThatHaveFired;
	averageAccuracy *= ACCURACY_MODIFIER;

	float adjustedAccuracy = averageAccuracy - mod_player_performance_accuracy_threshold.GetInt();
	if (adjustedAccuracy < mod_player_performance_accuracy_floor.GetFloat())
		adjustedAccuracy = mod_player_performance_accuracy_floor.GetFloat();

	m_LastCalculatedValue = adjustedAccuracy * mod_player_performance_accuracy_performance_modifier.GetFloat();

	*performance += m_LastCalculatedValue;	
}
void CMOD_Player_Performance_Calculator_Accuracy::PrintExtraDebugInfo(int offset)
{
	char* suffix = "";
	if (m_PlayerZeroAccuracy < mod_player_performance_accuracy_floor.GetFloat())
		suffix = " [FLOOR]";

	engine->Con_NPrintf(offset,"Accuracy [%0.3f]%s", m_PlayerZeroAccuracy, suffix);
}

//////////////////////////////////////////////////////////////////////////////////

//NOT USED - GAME IS ASSUMED TO BE SINGLE PLAYER
void CMOD_Player_Performance_Calculator_FriendlyFire::UpdatePerformance(float * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
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

void CMOD_Player_Performance_Calculator_DirectorStress::OnMissionStarted()
{
	//probably a memory leak
	g_directorStressHistory = new vector<float>();	
}

void CMOD_Player_Performance_Calculator_DirectorStress::UpdatePerformance(float * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
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

	m_averageStressHistory = 0;
	for (unsigned int i = 0; i < g_directorStressHistory->size(); i++)
	{
		m_averageStressHistory += g_directorStressHistory->at(i);
	}

	m_averageStressHistory /= g_directorStressHistory->size();
	
	if (m_averageStressHistory > mod_player_performance_director_stress_threshold.GetInt())
	{
		m_LastCalculatedValue = m_averageStressHistory * mod_player_performance_director_stress_penalty.GetFloat();
	}
	else
	{
		m_LastCalculatedValue = 0.0f;
	}	
}

void CMOD_Player_Performance_Calculator_DirectorStress::PrintExtraDebugInfo(int offset)
{
	engine->Con_NPrintf(offset, "Current Stress: [%0.3f] Historical Stress: [%0.3f]", m_averageStressOfPlayers, m_averageStressHistory);	
}

//////////////////////////////////////////////////////////////////////////////////

void CMOD_Player_Performance_Calculator_PlayTime::OnMissionStarted(){
	m_MissionStartTime = gpGlobals->curtime;
	m_LastCalculatedValue = 0.0f;
}

void CMOD_Player_Performance_Calculator_PlayTime::UpdatePerformance(float * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
{
	//This is only factored in at the end of the mission, because I don't know a good
	//way to track mission progress, as the layouts are random.
	if (!isEndOfLevel)
		return;

	float totalPlayTime = gpGlobals->curtime - m_MissionStartTime;
	int timeAgainstPar = mod_player_performance_playtime_par.GetInt() - totalPlayTime;

	m_LastCalculatedValue =  timeAgainstPar / (float)mod_player_performance_playtime_divisor_bonus.GetInt();

	*performance +=m_LastCalculatedValue;
}

void CMOD_Player_Performance_Calculator_PlayTime::PrintExtraDebugInfo(int offset)
{
	float totalPlayTime = gpGlobals->curtime - m_MissionStartTime;	
	engine->Con_NPrintf(offset, "Total Mission Play Time: [%0.3f]", totalPlayTime);	
}

//////////////////////////////////////////////////////////////////////////////////

void CMOD_Player_Performance_Calculator_AlienLifeTime::OnMissionStarted()
{
	m_totalAlienLifeTime = 0;
	m_numberOfAliensKilled = 0;
	m_LastCalculatedValue = 0;
}

void CMOD_Player_Performance_Calculator_AlienLifeTime::Event_AlienKilled( CBaseEntity *pAlien, const CTakeDamageInfo &info )
{	
	float total_alive_time = gpGlobals->curtime - pAlien->GetModSpawnTime();	

	 m_totalAlienLifeTime += total_alive_time;
	 m_numberOfAliensKilled++;
}
void CMOD_Player_Performance_Calculator_AlienLifeTime::UpdatePerformance(float * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
{
	if (m_numberOfAliensKilled == 0 )
		return;

	float averageLifeExpectency = m_totalAlienLifeTime / m_numberOfAliensKilled;

	if (averageLifeExpectency <= mod_player_performance_enemy_life_time_positive_threshold.GetFloat())
	{
		m_LastCalculatedValue = averageLifeExpectency * mod_player_performance_enemy_life_time_positive_bonus.GetFloat();
	}
	else if (averageLifeExpectency >= mod_player_performance_enemy_life_time_negative_threshold.GetFloat())
	{
		m_LastCalculatedValue = averageLifeExpectency * mod_player_performance_enemy_life_time_negative_penalty.GetFloat();
	}	
	else
	{
		m_LastCalculatedValue =  0.0f;
	}

	*performance +=m_LastCalculatedValue;
}

void CMOD_Player_Performance_Calculator_AlienLifeTime::PrintExtraDebugInfo(int offset)
{
	engine->Con_NPrintf(offset, "Aliens Killed [%i] Total Life Time: [%0.3f]", m_numberOfAliensKilled, m_totalAlienLifeTime);	
}

//////////////////////////////////////////////////////////////////////////////////

CMOD_Player_Performance_Calculator_FastReload::CMOD_Player_Performance_Calculator_FastReload(bool bIsSignlePlayerMode)
		:CMOD_Player_Performance_Calculator(bIsSignlePlayerMode)
{
	m_DebugName = "Fast Reload";		

	ListenForGameEvent("fast_reload");	
}

CMOD_Player_Performance_Calculator_FastReload::~CMOD_Player_Performance_Calculator_FastReload()
{
	StopListeningForAllEvents();
}

void CMOD_Player_Performance_Calculator_FastReload::OnMissionStarted()
{
	m_NumberOfFastReloads = 0;	
}

void CMOD_Player_Performance_Calculator_FastReload::FireGameEvent(IGameEvent * event)
{
	const char * type = event->GetName();
	
	if ( Q_strcmp(type, "fast_reload") == 0 )
	{
		m_NumberOfFastReloads++;
	}	
}

void CMOD_Player_Performance_Calculator_FastReload::UpdatePerformance(float * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
{
	m_LastCalculatedValue = (float)(m_NumberOfFastReloads * mod_player_performance_fastreload_bonus.GetInt());

	*performance += m_LastCalculatedValue;
}

void CMOD_Player_Performance_Calculator_FastReload::PrintExtraDebugInfo(int offset)
{
	engine->Con_NPrintf(offset, "Number of Fast Reloads: [%i]", m_NumberOfFastReloads);	
}

//////////////////////////////////////////////////////////////////////////////////

void CMOD_Player_Performance_Calculator_DodgeRanger::OnMissionStarted()
{
	m_HasDodgedRanger = false;
	m_LastCalculatedValue = 0;
}

void CMOD_Player_Performance_Calculator_DodgeRanger::UpdatePerformance(float * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
{
	if (!m_HasDodgedRanger)
	{
		for ( int i=0;i<pGameResource->GetMaxMarineResources();i++ )
		{
			CASW_Marine_Resource *pMR = pGameResource->GetMarineResource(i);
			if ( !pMR )
				continue;

			CASW_Marine *pMarine = pMR->GetMarineEntity();
			if ( !pMarine || pMarine->GetHealth() <= 0 )
				continue;

			if (!pMarine->GetMarineResource())
				continue;

			if (pMarine->GetMarineResource()->m_bDodgedRanger)
			{
				m_HasDodgedRanger = true;
				m_LastCalculatedValue = mod_player_performance_dodgeranger_bonus.GetInt();
			}
		}
	}

	*performance +=m_LastCalculatedValue;
}

//////////////////////////////////////////////////////////////////////////////////

void CMOD_Player_Performance_Calculator_MeleeKills::OnMissionStarted()
{
	m_numberOfAliensKilled = 0;
	m_numberOfMeleeKills = 0;

	m_LastCalculatedValue = 0.0f;
	
}

void CMOD_Player_Performance_Calculator_MeleeKills::Event_AlienKilled( CBaseEntity *pAlien, const CTakeDamageInfo &info )
{		
	 m_numberOfAliensKilled++;

	 if (!Q_strcmp(info.GetAmmoName(), "asw_marine"))
	 {
		 m_numberOfMeleeKills++;	
	 }
}

void CMOD_Player_Performance_Calculator_MeleeKills::UpdatePerformance(float * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
{
	if (m_numberOfAliensKilled > 0 )
	{
		//bonus = ration of [number of melee kills]/[number of total kills] * [meleekill bonus]

		m_LastCalculatedValue = 
			(float)m_numberOfMeleeKills * 
			mod_player_performance_meleekill_bonus.GetFloat();
	}	

	*performance +=m_LastCalculatedValue;
}

void CMOD_Player_Performance_Calculator_MeleeKills::PrintExtraDebugInfo(int offset)
{
	engine->Con_NPrintf(offset, "Melee Kills [%i] Total Kills: [%i]", m_numberOfMeleeKills, m_numberOfAliensKilled);	
}

//////////////////////////////////////////////////////////////////////////////////

void CMOD_Player_Performance_Calculator_BoomerKillEarly::OnMissionStarted()
{
	m_HasBoomerKillEarly = false;
	m_LastCalculatedValue = 0;
}

void CMOD_Player_Performance_Calculator_BoomerKillEarly::UpdatePerformance(float * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource)
{
	if (!m_HasBoomerKillEarly)
	{
		for ( int i=0;i<pGameResource->GetMaxMarineResources();i++ )
		{
			CASW_Marine_Resource *pMR = pGameResource->GetMarineResource(i);
			if ( !pMR )
				continue;

			CASW_Marine *pMarine = pMR->GetMarineEntity();
			if ( !pMarine || pMarine->GetHealth() <= 0 )
				continue;

			if (!pMarine->GetMarineResource())
				continue;

			if (pMarine->GetMarineResource()->m_bKilledBoomerEarly)
			{
				m_HasBoomerKillEarly = true;
				m_LastCalculatedValue = mod_player_performance_boomer_kill_early_bonus.GetInt();
			}
		}
	}

	*performance +=m_LastCalculatedValue;
}




