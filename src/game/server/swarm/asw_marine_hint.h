//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef _INCLUDED_ASW_MARINE_HINT_H
#define _INCLUDED_ASW_MARINE_HINT_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "ai_initutils.h"

class CASW_Marine;

class CASW_Marine_Hint_Ent : public CServerOnlyEntity
{
	DECLARE_CLASS( CASW_Marine_Hint_Ent, CServerOnlyEntity );
public:

	DECLARE_DATADESC();

	virtual void Spawn();	
};

class CASW_Marine_Hint_Node_Ent : public CNodeEnt
{
	DECLARE_CLASS( CASW_Marine_Hint_Node_Ent, CNodeEnt );
public:

	DECLARE_DATADESC();

	virtual void UpdateOnRemove();	
};

class HintData_t
{
public:
	const Vector &GetAbsOrigin() { return m_vecPosition; }

	Vector m_vecPosition;
	float m_flYaw;
	int m_nHintIndex;
	float m_flDistance;
};

//-----------------------------------------------------------------------------
// Purpose: Stores positions for the marines to use while following
//-----------------------------------------------------------------------------
class CASW_Marine_Hint_Manager : protected CAutoGameSystem
{
	typedef CAutoGameSystem BaseClass;
public:
	CASW_Marine_Hint_Manager();
	~CASW_Marine_Hint_Manager();

	void Reset();
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPostEntity();	

	void AddHint( CBaseEntity *pEnt, bool bLastResort = false );
	int FindHints( CASW_Marine *pLeader, const float flMinDistance, const float flMaxDistance, CUtlVector<HintData_t *> &Result );
	
	int GetHintCount() { return m_Hints.Count(); }
	const Vector& GetHintPosition( int nHint ) { return m_Hints[ nHint ]->m_vecPosition; }
	float GetHintYaw( int nHint ) { return m_Hints[ nHint ]->m_flYaw; }

private:
	CUtlVector<HintData_t *> m_Hints;
	CUtlVector<HintData_t *> m_LastResortHints;
	friend class CASW_Campaign_Fixes;
};
CASW_Marine_Hint_Manager* MarineHintManager();

#define INVALID_HINT_INDEX -1

#endif // _INCLUDED_ASW_MARINE_HINT_H
