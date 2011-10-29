#ifndef _INCLUDED_MOD_PLAYER_PERFORMANCE_H
#define _INCLUDED_MOD_PLAYER_PERFORMANCE_H

#include "asw_game_resource.h"

#include <vector>
using namespace std;

class CMOD_Player_Performance_Calculator;
class CBaseEntity;
class  CTakeDamageInfo;

class CMOD_Player_Performance : CAutoGameSystemPerFrame
{
	CMOD_Player_Performance();
	~CMOD_Player_Performance();
	static CMOD_Player_Performance* g_PlayerPerformanceSingleton;

	vector<double>* g_directorStressHistory;
	vector<CMOD_Player_Performance_Calculator*> g_calculators;
	
public:
	//Singleton accessor
	static CMOD_Player_Performance* PlayerPerformance();

	int m_totalRating, m_weightedRating, m_healthRating, m_accuracyRating, m_friendlyFireRating, m_directorStressRating;
	int m_playerZeroAccuracy;
	int m_previousRating;
	double m_averageStressOfPlayers, m_averageStressHistory;

	virtual void FrameUpdatePostEntityThink();

	int CalculatePerformance(){return CalculatePerformance(false);}
	int CalculatePerformance(bool isEndOfLevel);
	int CalculatePerformanceButDoNotUpdateHUD(bool isEndOfLevel);

	int CalculateHealthRating(CASW_Game_Resource *pGameResource);
	int CalculateAccuracyRating(CASW_Game_Resource *pGameResource);
	int CalculateFriendFireRating(CASW_Game_Resource *pGameResource);
	int CalculateDirectorStress(CASW_Game_Resource *pGameResource);	
	void PrintDebug();
	void WriteToHUD(const char* messagename, int rating);

	//Called by asw_director
	void OnMissionStarted();
	void Event_AlienKilled( CBaseEntity *pAlien, const CTakeDamageInfo &info );
};

#endif /* _INCLUDED_MOD_PLAYER_PERFORMANCE_H */
