#ifndef _INCLUDED_MOD_PLAYER_CALCULATORS_H
#define _INCLUDED_MOD_PLAYER_CALCULATORS_H

class CASW_Game_Resource;
class CBaseEtnity;
class CTakeDamageInfo;

#include <vector>
using namespace std;

class CMOD_Player_Performance_Calculator{
protected:
	CMOD_Player_Performance_Calculator(bool bIsSignlePlayerMode){
		m_bIsSinglePlayerMode = bIsSignlePlayerMode;
		m_LastCalculatedValue = 0;
	}

	int m_LastCalculatedValue;
	bool m_bIsSinglePlayerMode;
	char * m_DebugName;

public:
	virtual void OnMissionStarted(){}
	virtual void Event_AlienKilled( CBaseEntity *pAlien, const CTakeDamageInfo &info ){};

	virtual void UpdatePerformance(int * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource) = 0;	
	bool IsSinglePlayerMode(){return m_bIsSinglePlayerMode;}

	virtual void PrintDebugString(int offset);
	virtual int GetDebugValue(){return m_LastCalculatedValue;}	
	virtual bool HasExtraDebugInfo(){ return false;}
	virtual void PrintExtraDebugInfo(int offset){}		
};

class CMOD_Player_Performance_Calculator_Health : public CMOD_Player_Performance_Calculator
{
public:	
	CMOD_Player_Performance_Calculator_Health(bool bIsSignlePlayerMode)
		:CMOD_Player_Performance_Calculator(bIsSignlePlayerMode)
	{	
		m_DebugName = "Health";			
	}
		
	virtual void UpdatePerformance(int * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource);
};

class CMOD_Player_Performance_Calculator_Accuracy : public CMOD_Player_Performance_Calculator
{
public:
	CMOD_Player_Performance_Calculator_Accuracy(bool bIsSignlePlayerMode)
		:CMOD_Player_Performance_Calculator(bIsSignlePlayerMode)
	{
		m_DebugName = "Accuracy";		
	}

	//The accuracy of the primay player.
	float m_PlayerZeroAccuracy;

	virtual void UpdatePerformance(int * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource);
	virtual bool HasExtraDebugInfo(){ return true;}
	virtual void PrintExtraDebugInfo(int offset);
};

class CMOD_Player_Performance_Calculator_FriendlyFire : public CMOD_Player_Performance_Calculator
{
public:
	CMOD_Player_Performance_Calculator_FriendlyFire(bool bIsSignlePlayerMode)
		:CMOD_Player_Performance_Calculator(bIsSignlePlayerMode)
	{
		m_DebugName = "Friendly Fire";		
	}
	
	virtual void UpdatePerformance(int * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource);
};

class CMOD_Player_Performance_Calculator_DirectorStress : public CMOD_Player_Performance_Calculator
{
public:
	CMOD_Player_Performance_Calculator_DirectorStress(bool bIsSignlePlayerMode)
		:CMOD_Player_Performance_Calculator(bIsSignlePlayerMode)
	{
		m_DebugName = "Dicrector Stress";		
	}

	vector<double>* g_directorStressHistory;

	double m_averageStressOfPlayers, m_averageStressHistory;

	virtual void OnMissionStarted();		
	virtual bool HasExtraDebugInfo(){ return true;}
	virtual void PrintExtraDebugInfo(int offset);
	virtual void UpdatePerformance(int * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource);
};

class CMOD_Player_Performance_Calculator_PlayTime : public CMOD_Player_Performance_Calculator
{
public:
	CMOD_Player_Performance_Calculator_PlayTime(bool bIsSignlePlayerMode)
		:CMOD_Player_Performance_Calculator(bIsSignlePlayerMode)
	{
		m_DebugName = "Play Time";		
	}
	
	virtual void UpdatePerformance(int * performance, bool isEndOfLevel, CASW_Game_Resource *pGameResource);
};


#endif