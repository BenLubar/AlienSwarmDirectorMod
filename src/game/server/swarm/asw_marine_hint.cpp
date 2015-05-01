//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "asw_marine_hint.h"
#include "asw_marine.h"
#include "ai_pathfinder.h"
#include "ai_waypoint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// ========= Temporary hint entity ============

LINK_ENTITY_TO_CLASS( info_marine_hint, CASW_Marine_Hint_Ent );

BEGIN_DATADESC( CASW_Marine_Hint_Ent )

END_DATADESC();

void CASW_Marine_Hint_Ent::Spawn()
{
	Assert( MarineHintManager() );
	MarineHintManager()->AddHint( this );

	BaseClass::Spawn();

	UTIL_RemoveImmediate( this );
}

// ========= Temporary hint entity that's also an info_node ============

LINK_ENTITY_TO_CLASS( info_node_marine_hint, CASW_Marine_Hint_Node_Ent );

BEGIN_DATADESC( CASW_Marine_Hint_Node_Ent )

END_DATADESC();

// info_nodes are automatically removed when the map loads, store position/direction in our hint list
void CASW_Marine_Hint_Node_Ent::UpdateOnRemove()
{
	Assert( MarineHintManager() );
	MarineHintManager()->AddHint( this );

	BaseClass::UpdateOnRemove();
}

// =============== Hint Manager ===============

CASW_Marine_Hint_Manager g_Marine_Hint_Manager;
CASW_Marine_Hint_Manager* MarineHintManager() { return &g_Marine_Hint_Manager; }

CASW_Marine_Hint_Manager::CASW_Marine_Hint_Manager()
{
	
}

CASW_Marine_Hint_Manager::~CASW_Marine_Hint_Manager()
{

}

void CASW_Marine_Hint_Manager::LevelInitPreEntity()
{
	BaseClass::LevelInitPreEntity();

	Reset();
}

void CASW_Marine_Hint_Manager::LevelShutdownPostEntity()
{
	Reset();
}

void CASW_Marine_Hint_Manager::Reset()
{
	m_Hints.PurgeAndDeleteElements();
	m_LastResortHints.PurgeAndDeleteElements();
}

int CASW_Marine_Hint_Manager::FindHints(const Vector &position, CASW_Marine *pLeader, const float flMinDistance, const float flMaxDistance, CUtlVector<HintData_t *> &result)
{
	VPROF_BUDGET("CASW_Marine_Hint_Manager::FindHints", "SquadFormation");
	Assert(pLeader);
	const float flMinDistSqr = 0; // we want the leader to be able to use their current hint.
	const float flMaxDistSqr = flMaxDistance * flMaxDistance;
	int nCount = m_Hints.Count();
	for ( int i = 0; i < nCount; i++ )
	{
		HintData_t *pHint = m_Hints[i];
		float flDistSqr = position.DistToSqr(pHint->m_vecPosition);
		if (flDistSqr < flMinDistSqr || flDistSqr > flMaxDistSqr)
		{
			continue;
		}

		AI_Waypoint_t *pPath = pLeader->GetPathfinder()->BuildRoute(position, pHint->m_vecPosition, NULL, 0, NAV_GROUND);
		if (!pPath)
		{
			continue;
		}
		pHint->m_flDistance = position.DistTo(pPath->GetPos());
		AI_Waypoint_t *pCur = pPath;
		while (pCur->GetNext())
		{
			AI_Waypoint_t *pNext = pCur->GetNext();
			pHint->m_flDistance += pCur->GetPos().DistTo(pNext->GetPos());
			pCur = pNext;
		}
		DeleteAll(pPath);

		if (pHint->m_flDistance > flMaxDistance)
		{
			continue;
		}

		result.AddToTail( pHint );
	}
	return result.Count();
}

void CASW_Marine_Hint_Manager::AddHint( CBaseEntity *pEnt, bool bLastResort )
{
	HintData_t *pHintData = new HintData_t;
	pHintData->m_vecPosition = pEnt->GetAbsOrigin();
	pHintData->m_flYaw = pEnt->GetAbsAngles()[ YAW ];
	pHintData->m_nHintIndex = (bLastResort ? m_LastResortHints : m_Hints).AddToTail(pHintData);
}

CON_COMMAND( asw_show_marine_hints, "Show hint manager spots" )
{
	int nCount = MarineHintManager()->GetHintCount();
	for ( int i = 0; i < nCount; i++ )
	{
		Vector vecPos = MarineHintManager()->GetHintPosition( i );
		float flYaw = MarineHintManager()->GetHintYaw( i );

		NDebugOverlay::YawArrow( vecPos, flYaw, 64, 16, 255, 255, 255, 0, true, 3.0f );
	}
}