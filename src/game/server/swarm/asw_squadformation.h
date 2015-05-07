#ifndef ASW_SQUADFORMATION_H
#define ASW_SQUADFORMATION_H
#pragma once

#include "asw_shareddefs.h"

class CASW_Marine;
class CAI_Hint;
class HintData_t;

// Aggregates state about all marines assigned to a squad,
// eg, AI marines following a leader in formation.
class CASW_SquadFormation : public CServerOnlyPointEntity
{
	DECLARE_CLASS(CASW_SquadFormation, CServerOnlyPointEntity);

public:
	// local compiler constants
	enum 
	{
		SQUAD_LEADER = 0,
		FIRST_SQUAD_FOLLOWER = 1,
		MAX_SQUAD_SIZE = ASW_MAX_MARINE_RESOURCES,
		INVALID_SQUADDIE,
	};

	inline CASW_Marine *Leader() const { return Squaddie( SQUAD_LEADER ); }
	inline int Count() const; // number of squad members

	inline CASW_Marine *Squaddie( unsigned int slotnum ) const;
	inline CASW_Marine *operator[]( unsigned int slotnum ) const { return Squaddie( slotnum ); }

	// return the slot number for a particular marine. 
	// Return INVALID_SQUADDIE if the marine is not in this squad.
	unsigned int Find( CASW_Marine *pMarine ) const;
	inline static bool IsValid( unsigned slotnum );

	float GetYaw( unsigned slotnum );
	inline const Vector &GetIdealPosition( unsigned slotnum ) const;

	unsigned int Add( CASW_Marine *pMarine );
	bool Remove( CASW_Marine *pMarine, bool bIgnoreAssert = false ); // ret true if this marine was actually found and removed
	bool Remove( unsigned int slotnum ); // ret true if this marine was actually found and removed

	void ChangeLeader( CASW_Marine *pNewLeader, bool bUpdateLeaderPos = false );
	
	// recompute the array of positions that squaddies should head towards.
	void UpdateFollowPositions();

	// find the nearest uncompleted objective
	void UpdateGoalPosition();

	// should the squaddie positions be recomputed -- assumed this function is called from a marine's Think
	bool ShouldUpdateFollowPositions() const;

	// follow in tight formation instead of using hints for asw_follow_hint_delay seconds
	void FollowCommandUsed( unsigned slotnum = INVALID_SQUADDIE );
	float m_flUseHintsAfter[MAX_SQUAD_SIZE];

	Vector GetLeaderPosition();

	CASW_SquadFormation() : m_flLastSquadUpdateTime(0) { Reset(); }
	bool SanityCheck() const;

	void FindFollowHintNodes();

	void Reset();

	static int FollowHintSortFunc( HintData_t* const *pHint1, HintData_t* const *pHint2 );

	void DrawDebugGeometryOverlays();

	Vector m_vecObjective;
	bool m_bInMarker;

protected:
	CHandle<CASW_Marine> m_hSquad[MAX_SQUAD_SIZE];

	// the location in world where each squaddie is supposed
	// to be standing this frame.
	Vector m_vFollowPositions[MAX_SQUAD_SIZE];	

	// hint nodes for use in combat
	int m_nMarineHintIndex[MAX_SQUAD_SIZE];
	bool m_bStandingInBeacon[MAX_SQUAD_SIZE];
	bool m_bFleeingBoomerBombs[MAX_SQUAD_SIZE];

	// clumsy holdovers from old system, should be replaced
	// with proper movement histories with prediction
	float m_flLastLeaderYaw;
	Vector m_vLastLeaderPos;

	Vector m_vLastLeaderVelocity;

	float m_flCurrentForwardAbsoluteEulerYaw;
	float m_flLastSquadUpdateTime;

	int m_iLastHealBeaconCount;

#pragma region   STATICS
	const static Vector s_MarineFollowOffset[MAX_SQUAD_SIZE];
	const static float s_MarineFollowDirection[MAX_SQUAD_SIZE];
	const static Vector s_MarineBeaconOffset[MAX_SQUAD_SIZE];
	const static float s_MarineBeaconDirection[MAX_SQUAD_SIZE];
#pragma endregion

private:
	Vector GetLdrAnglMatrix( const Vector &origin, const QAngle &ang, matrix3x4_t *pout );
};

inline CASW_Marine *CASW_SquadFormation::Squaddie( unsigned int slotnum ) const 
{
	Assert( IsValid(slotnum) );
	if (!IsValid(slotnum))
	{
		return NULL;
	}
	return m_hSquad[slotnum];
}

inline bool CASW_SquadFormation::IsValid( unsigned slotnum )
{
	return slotnum < MAX_SQUAD_SIZE;
}

inline const Vector &CASW_SquadFormation::GetIdealPosition( unsigned slotnum ) const
{
	return m_vFollowPositions[slotnum];
}

inline int CASW_SquadFormation::Count() const
{
	int ret = 0;
	for ( int i = 0 ; i < MAX_SQUAD_SIZE; ++i )
	{
		if ( m_hSquad[i].Get() )
			++ret;
	}
	return ret;
}
#endif /* ASW_SQUADFORMATION_H */
