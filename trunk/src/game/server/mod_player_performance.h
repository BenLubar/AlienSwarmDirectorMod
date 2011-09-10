#ifndef _INCLUDED_MOD_PLAYER_PERFORMANCE_H
#define _INCLUDED_MOD_PLAYER_PERFORMANCE_H

#include "asw_game_resource.h"
#include <vector>
using namespace std;

class CMOD_Player_Performance : CAutoGameSystemPerFrame
{
	CMOD_Player_Performance();
	~CMOD_Player_Performance();
	static CMOD_Player_Performance* g_PlayerPerformanceSingleton;

	vector<double>* g_directorStressHistory;
	
public:
	//Singleton accessor
	static CMOD_Player_Performance* PlayerPerformance();

	int m_totalRating, m_weightedRating, m_healthRating, m_accuracyRating, m_friendlyFireRating, m_directorStressRating;
	int m_playerZeroAccuracy;
	double m_averageStressOfPlayers, m_averageStressHistory;

	virtual void FrameUpdatePostEntityThink();

	int CalculatePerformance();

	int CalculateHealthRating(CASW_Game_Resource *pGameResource);
	int CalculateAccuracyRating(CASW_Game_Resource *pGameResource);
	int CalculateFriendFireRating(CASW_Game_Resource *pGameResource);
	int CalculateDirectorStress(CASW_Game_Resource *pGameResource);	
	void PrintDebug();

	//Called by asw_director
	void OnMissionStarted();
};

#endif /* _INCLUDED_MOD_PLAYER_PERFORMANCE_H */