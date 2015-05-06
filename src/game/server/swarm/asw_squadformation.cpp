#include "cbase.h"
#include "asw_marine.h"
#include "asw_marine_profile.h"
#include "asw_marine_hint.h"
#include "asw_weapon_healgrenade_shared.h"
#include "asw_shieldbug.h"
#include "asw_boomer_blob.h"
#include "triggers.h"
#include "asw_player.h"
#include "asw_marker.h"
#include "asw_spawn_manager.h"
#include "asw_objective.h"
#include "asw_objective_kill_aliens.h"
#include "asw_objective_kill_eggs.h"
#include "asw_objective_kill_goo.h"
#include "asw_objective_kill_queen.h"
#include "asw_objective_escape.h"
#include "asw_objective_triggered.h"
#include "asw_gamerules.h"
#include "ai_pathfinder.h"
#include "ai_waypoint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar asw_marine_ai_followspot( "asw_marine_ai_followspot", "0", FCVAR_CHEAT );
ConVar asw_follow_hint_max_range("asw_follow_hint_max_range", "900", FCVAR_CHEAT);
ConVar asw_follow_hint_min_range("asw_follow_hint_min_range", "100", FCVAR_CHEAT);
ConVar asw_follow_hint_max_z_dist("asw_follow_hint_max_z_dist", "190", FCVAR_CHEAT);
ConVar asw_follow_use_hints( "asw_follow_use_hints", "2", FCVAR_CHEAT, "0 = follow formation, 1 = use hints when in combat, 2 = always use hints" );
ConVar asw_follow_hint_delay( "asw_follow_hint_delay", "5", FCVAR_CHEAT, "The number of seconds marines will ignore follow hints after being told to follow" );
ConVar asw_follow_hint_debug( "asw_follow_hint_debug", "0", FCVAR_CHEAT );
ConVar asw_follow_velocity_predict( "asw_follow_velocity_predict", "0.3", FCVAR_CHEAT, "Marines travelling in diamond follow formation will predict their leader's movement ahead by this many seconds" );
ConVar asw_squad_debug( "asw_squad_debug", "1", FCVAR_CHEAT, "Draw debug overlays for squad movement" );

// TODO: is boomer bomb radius ever larger than 500 units?
#define OUT_OF_BOOMER_BOMB_RANGE 500

LINK_ENTITY_TO_CLASS(asw_squadformation, CASW_SquadFormation);

unsigned int CASW_SquadFormation::Add( CASW_Marine *pMarine )
{
	Assert( pMarine );
	AssertMsg( Leader(), "A CASW_SquadFormation has no leader!\n" );
	AssertMsg1( pMarine != Leader(), "Tried to set %s to follow itself!\n", pMarine->GetDebugName() );

	unsigned slot = Find( pMarine );
	if ( IsValid( slot ) )
	{
		AssertMsg2( false, "Tried to double-add %s to squad (already in slot %d)\n", pMarine->GetDebugName(), slot );
		return slot;
	}
	else
	{
		for ( slot = FIRST_SQUAD_FOLLOWER; slot < MAX_SQUAD_SIZE; slot++ )
		{
			if ( !Squaddie( slot ) )
			{
				m_hSquad[slot] = pMarine;
				return slot;
			}
		}

		// if we're down here, the squad is full!
		AssertMsg2( false, "Tried to add %s to %s's squad, but that's full! (How?)\n", pMarine->GetDebugName(), Leader()->GetDebugName() );
		return INVALID_SQUADDIE;
	}
}

bool CASW_SquadFormation::Remove( unsigned int slotnum )
{
	Assert( IsValid( slotnum ) );
	if ( Squaddie(slotnum) )
	{
		m_hSquad[slotnum] = NULL;
		return true;
	}
	else
	{
		return false;
	}
}

bool CASW_SquadFormation::Remove( CASW_Marine *pMarine, bool bIgnoreAssert )
{
	Assert( pMarine );
	unsigned slot = Find( pMarine );
	if ( IsValid( slot ) )
	{
		return Remove( slot );
	}
	else
	{
		AssertMsg1( bIgnoreAssert, "Tried to remove marine %s from squad, but wasn't a member.\n", pMarine->GetDebugName() );
		return false;
	}
}

const  Vector CASW_SquadFormation::s_MarineFollowOffset[MAX_SQUAD_SIZE]=
{
	Vector(0, 0, 0),
	Vector(-60, -70, 0),
	Vector(-120, 0, 0),
	Vector(-60, 70, 0),
	Vector(-100, 50, 0),
	Vector(-100, -50, 0),
	Vector(-20, 50, 0),
	Vector(-20, -50, 0)
};

const  float CASW_SquadFormation::s_MarineFollowDirection[MAX_SQUAD_SIZE]=
{
	0,
	-70,
	180,
	70,
	125,
	-125,
	35,
	-35
};

// position offsets when standing around a heal beacon
const  Vector CASW_SquadFormation::s_MarineBeaconOffset[MAX_SQUAD_SIZE]=
{
	Vector(0, 0, 0),
	Vector(30, -52, 0),
	Vector(-52, 0, 0),
	Vector(30, 52, 0),
	Vector(-30, 40, 0),
	Vector(-30, -40, 0),
	Vector(-20, 40, 0),
	Vector(-20, -40, 0)
};

const  float CASW_SquadFormation::s_MarineBeaconDirection[MAX_SQUAD_SIZE]=
{
	0,
	-70,
	180,
	70,
	125,
	-125,
	35,
	-35
};

float CASW_SquadFormation::GetYaw( unsigned slotnum )
{
	Assert( IsValid( slotnum ) );
	if ( m_bStandingInBeacon[slotnum] )
	{
		return s_MarineBeaconDirection[slotnum];
	}
	else if ( m_bFleeingBoomerBombs[ slotnum ] )
	{
		CASW_Marine *pMarine = Squaddie( slotnum );
		if ( pMarine )
		{
			return pMarine->ASWEyeAngles()[ YAW ];
		}
		return 0.0f;
	}
	else if ( MarineHintManager()->GetHintCount() && m_flUseHintsAfter[slotnum] < gpGlobals->curtime && asw_follow_use_hints.GetBool() && Leader() && ( Leader()->IsInCombat() || asw_follow_use_hints.GetInt() == 2 ) )
	{
		if ( m_nMarineHintIndex[ slotnum ] != INVALID_HINT_INDEX )
		{
			return MarineHintManager()->GetHintYaw( m_nMarineHintIndex[ slotnum ] );
		}
	}
	// face our formation direction
	return anglemod( m_flCurrentForwardAbsoluteEulerYaw + s_MarineFollowDirection[ slotnum ] );
}

Vector CASW_SquadFormation::GetLdrAnglMatrix( const Vector &origin, const QAngle &ang, matrix3x4_t * RESTRICT pout ) RESTRICT
{
	Vector vecLeaderAim;
	{
		// "forward" is actual movement velocity if available, facing otherwise
		float leaderVelSq = m_vLastLeaderVelocity.LengthSqr();
		if ( leaderVelSq > 1.0f )
		{
			vecLeaderAim = m_vLastLeaderVelocity * FastRSqrtFast(leaderVelSq);
			VectorMatrix( vecLeaderAim, *pout );
			pout->SetOrigin( origin );
		}
		else
		{
			AngleMatrix( ang, origin, *pout );
			MatrixGetColumn( *pout, 0, vecLeaderAim );
		}
	}

	m_vCachedForward = vecLeaderAim;
	if (m_vCachedForward[1] == 0 && m_vCachedForward[0] == 0)
	{
		m_flCurrentForwardAbsoluteEulerYaw = 0;
	}
	else
	{
		m_flCurrentForwardAbsoluteEulerYaw = (atan2(vecLeaderAim[1], vecLeaderAim[0]) * (180.0 / M_PI));
		m_flCurrentForwardAbsoluteEulerYaw = 
			fsel( m_flCurrentForwardAbsoluteEulerYaw, m_flCurrentForwardAbsoluteEulerYaw, m_flCurrentForwardAbsoluteEulerYaw + 360 );
	}
	

	return vecLeaderAim;
}

float GetClosestPointToSegmentDistSqr(const Vector &v, const Vector &w, const Vector &p)
{
	// Adapted from http://stackoverflow.com/a/1501725

	// Return minimum distance between line segment vw and point p
	const float l2 = v.DistToSqr(w); // i.e. |w-v|^2 -  avoid a sqrt
	// v == w case
	if (l2 == 0.0f)
		return v.DistToSqr(p);

	// Consider the line extending the segment, parameterized as v + t (w - v).
	// We find projection of point p onto the line. 
	// It falls where t = [(p-v) . (w-v)] / |w-v|^2
	const float t = DotProduct(p - v, w - v) / l2;

	// Beyond the 'v' end of the segment
	if (t < 0.0f)
		return v.DistToSqr(p);

	// Beyond the 'w' end of the segment
	if (t > 1.0f)
		return w.DistToSqr(p);

	// Projection falls on the segment
	const Vector projection = v + t * (w - v);
	return projection.DistToSqr(p);
}

// returns Square(OUT_OF_BOOMER_BOMB_RANGE) if not within blast radius of any boomer bomb
float GetClosestBoomerBlobDistSqr( const Vector &vecNode, const Vector &vecFrom )
{
	float flClosestBoomerBlobDistSqr = Square( OUT_OF_BOOMER_BOMB_RANGE );

	FOR_EACH_VEC( g_aExplosiveProjectiles, iBoomerBlob )
	{
 		CBaseEntity *pExplosive = g_aExplosiveProjectiles[iBoomerBlob];

		float flDistSqr;
		if ( vecFrom.DistToSqr( pExplosive->GetAbsOrigin() ) < Square( OUT_OF_BOOMER_BOMB_RANGE ) )
			flDistSqr = vecNode.DistToSqr( pExplosive->GetAbsOrigin() );
		else
			flDistSqr = GetClosestPointToSegmentDistSqr( vecFrom, vecNode, pExplosive->GetAbsOrigin() );
		flClosestBoomerBlobDistSqr = MIN( flDistSqr, flClosestBoomerBlobDistSqr );
	}

	return flClosestBoomerBlobDistSqr;
}

void CASW_SquadFormation::UpdateFollowPositions()
{
	VPROF_BUDGET("CASW_SquadFormation::UpdateFollowPositions", "SquadFormation");

	CASW_Marine * RESTRICT pLeader = Leader();
	if ( !pLeader )
	{
		AssertMsg1( false, "Tried to update positions for a squad with no leader and %d followers.\n", Count() );
		return;
	}
	m_flLastSquadUpdateTime = gpGlobals->curtime;

	if (!pLeader->IsInhabited())
	{
		UpdateGoalPosition();
	}

	if ( MarineHintManager()->GetHintCount() )
	{
		FindFollowHintNodes();
	}

	QAngle angLeaderFacing = pLeader->EyeAngles();
	angLeaderFacing[PITCH] = 0;
	matrix3x4_t matLeaderFacing;
	Vector vProjectedLeaderPos = GetLeaderPosition() + pLeader->GetAbsVelocity() * asw_follow_velocity_predict.GetFloat();
	GetLdrAnglMatrix( vProjectedLeaderPos, angLeaderFacing, &matLeaderFacing );

	for ( int i = 0 ; i < MAX_SQUAD_SIZE ; ++i )
	{
		CASW_Marine *pMarine = Squaddie( i );
		if ( !pMarine )
			continue;

		m_bStandingInBeacon[i] = false;
		m_bFleeingBoomerBombs[ i ] = false;
		CBaseEntity *pBeaconToStandIn = NULL;
		// check for nearby heal beacons			
		if ( IHealGrenadeAutoList::AutoList().Count() > 0 && pMarine->GetHealth() < pMarine->GetMaxHealth() )
		{
			const float flHealGrenadeDetectionRadius = 600.0f;
			for ( int g = 0; g < IHealGrenadeAutoList::AutoList().Count(); ++g )
			{
				const CUtlVector< IHealGrenadeAutoList* > &grenades = IHealGrenadeAutoList::AutoList();
				CBaseEntity *pBeacon = grenades[g]->GetEntity();
				if ( pBeacon && pBeacon->GetAbsOrigin().DistTo( pMarine->GetAbsOrigin() ) < flHealGrenadeDetectionRadius )
				{
					pBeaconToStandIn = pBeacon;
					break;
				}
			}
		}

		if ( pBeaconToStandIn )
		{
			m_vFollowPositions[i] = pBeaconToStandIn->GetAbsOrigin() + s_MarineBeaconOffset[i];
			m_bStandingInBeacon[i] = true;
		}
		else if ( g_aExplosiveProjectiles.Count() && MarineHintManager()->GetHintCount() && m_flUseHintsAfter[i] < gpGlobals->curtime && asw_follow_use_hints.GetBool() ) // assume there's combat if there are explosives
		{
			bool bSafeNodeFound = false;
			bool bUnsafeNodeFound = false;
			float flClosestSafeNodeDistSqr = FLT_MAX;
			float flBestUnsafeNodeDistSqr = 0.0f;
			Vector vecClosestSafeNode;
			Vector vecBestUnsafeNode;
			const float k_flMaxSearchDistance = 900.0f;

			for( int iHint = 0; iHint < MarineHintManager()->GetHintCount(); iHint++ )
			{
				Vector vecNodeLocation = MarineHintManager()->GetHintPosition( iHint );
				if ( vecNodeLocation.DistToSqr( m_vLastLeaderPos ) > Square( k_flMaxSearchDistance ) )
					continue;

				bool bNodeTaken = false;
				for ( int iSlot = 0; iSlot < MAX_SQUAD_SIZE; iSlot++ )
				{
					if ( iSlot != i && vecNodeLocation.DistToSqr( m_vFollowPositions[ iSlot ] ) < Square( 30.0f ) )	// don't let marines get too close, even if nodes are close
					{
						bNodeTaken = true;
						break;
					}
				}

				if( !bNodeTaken )
				{
					float flClosestBoomerBlobDistSqr = GetClosestBoomerBlobDistSqr( vecNodeLocation, pMarine->GetAbsOrigin() );
					if ( flClosestBoomerBlobDistSqr >= Square(OUT_OF_BOOMER_BOMB_RANGE) )
					{
						// if closer than the previous closest node, and the current node isn't taken, reserve it
						float flSafeNodeDistSqr = vecNodeLocation.DistToSqr( pLeader->GetAbsOrigin() );
						if ( flSafeNodeDistSqr < flClosestSafeNodeDistSqr )
						{
							flClosestSafeNodeDistSqr = flSafeNodeDistSqr;
							bSafeNodeFound = true;
							vecClosestSafeNode = vecNodeLocation;
						}
					}
					else
					{
						if ( flClosestBoomerBlobDistSqr > flBestUnsafeNodeDistSqr )
						{
							flBestUnsafeNodeDistSqr = flClosestBoomerBlobDistSqr;
							bUnsafeNodeFound = true;
							vecBestUnsafeNode = vecNodeLocation;
						}
					}
				}
			}

			if( bSafeNodeFound )
			{
				m_vFollowPositions[ i ] = vecClosestSafeNode;
				m_bFleeingBoomerBombs[ i ] = true;
			}
			else if( bUnsafeNodeFound )
			{
				m_vFollowPositions[ i ] = vecBestUnsafeNode;
				m_bFleeingBoomerBombs[ i ] = true;
			}
		}
		else if ( MarineHintManager()->GetHintCount() && m_flUseHintsAfter[i] < gpGlobals->curtime && asw_follow_use_hints.GetBool() && ( pLeader->IsInCombat() || asw_follow_use_hints.GetInt() == 2 ) )
		{
			if (i == CASW_SquadFormation::SQUAD_LEADER)
			{
				m_vFollowPositions[i] = vProjectedLeaderPos;
			}
			else if ( m_nMarineHintIndex[i] != INVALID_HINT_INDEX )
			{
				m_vFollowPositions[i] = MarineHintManager()->GetHintPosition( m_nMarineHintIndex[i] );
			}
			else if ( pMarine )
			{
				VectorTransform(s_MarineFollowOffset[i], matLeaderFacing, m_vFollowPositions[i]);
			}
		}
		else
		{
			VectorTransform( s_MarineFollowOffset[i], matLeaderFacing, m_vFollowPositions[i] );
		}
		if ( asw_marine_ai_followspot.GetBool() )
		{
			static float colors[MAX_SQUAD_SIZE][3] = { { 255, 255, 255 }, { 255, 64, 64 }, { 64, 255, 64 }, { 64, 64, 255 }, { 255, 255, 64 }, { 255, 64, 255 }, { 64, 255, 255 }, { 64, 64, 64 } };
			NDebugOverlay::HorzArrow( pLeader->GetAbsOrigin(), m_vFollowPositions[i], 3, 
				colors[i][0], colors[i][1], colors[i][2], 255, false, 0.35f );
		}
	}

	m_flLastLeaderYaw = pLeader->EyeAngles()[ YAW ];
	m_vLastLeaderPos = pLeader->GetAbsOrigin();
	m_vLastLeaderVelocity = pLeader->GetAbsVelocity();
}

bool CASW_SquadFormation::SanityCheck() const
{
	CASW_Marine *pLeader = Leader();
	Assert( pLeader );
	if ( !pLeader ) 
		return false;

	// for each slot, make sure no dups, and that leader is not following self
	for ( int testee = 0 ; testee < MAX_SQUAD_SIZE ; ++testee )
	{
		CASW_Marine *pTest = m_hSquad[testee];
		if ( pTest )
		{
			for ( int i = (testee+1)%MAX_SQUAD_SIZE ; i != testee ; i = (i + 1)%MAX_SQUAD_SIZE )
			{
				Assert( m_hSquad[i] != pTest ); // am not in array twice
				if ( m_hSquad[i] == pTest ) 
					return false;
			}
		}
	}

	return true;
}

// For such a tiny array, a linear search is actually the fastest
// way to go
unsigned int CASW_SquadFormation::Find( CASW_Marine *pMarine ) const
{
	//Assert( pMarine != Leader() );
	for ( int i = 0 ; i < MAX_SQUAD_SIZE ; ++i )
	{
		if ( m_hSquad[i] == pMarine )
			return i;
	}
	return INVALID_SQUADDIE;
}

void CASW_SquadFormation::ChangeLeader( CASW_Marine *pNewLeader, bool bUpdateLeaderPos )
{
	// if the squad has no leader, become the leader
	CASW_Marine *pOldLeader = Leader();
	if ( !pOldLeader )
	{
		m_hSquad[SQUAD_LEADER] = pNewLeader;
		return;
	}

	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		CASW_Player *pPlayer = dynamic_cast<CASW_Player *>(UTIL_PlayerByIndex(i));
		if (pPlayer && pPlayer->GetSpectatingMarine() == pOldLeader)
		{
			pPlayer->SetSpectatingMarine(pNewLeader);
		}
	}

	// if we're trying to wipe out the leader, do so if there are no followers
	if ( !pNewLeader )
	{
		AssertMsg2( Count() == 1, "Tried to unset squad leader %s, but squad has %d followers\n", pNewLeader->GetDebugName(), Count() - 1 );
		m_hSquad[SQUAD_LEADER] = NULL;
		return;
	}

	if ( pOldLeader == pNewLeader )
	{
		return;
	}

	// if the new leader was previously a follower, swap with the old leader
	unsigned slot = Find( pNewLeader );
	if ( IsValid(slot) )
	{
		m_hSquad[slot] = pOldLeader;
		m_hSquad[SQUAD_LEADER] = pNewLeader;
	}
	else
	{
		// make the old leader a follower 
		m_hSquad[SQUAD_LEADER] = pNewLeader;
		Add( pOldLeader );
	}
	if ( bUpdateLeaderPos )
	{
		m_flLastLeaderYaw = pNewLeader->EyeAngles()[YAW];
		m_vLastLeaderPos = pNewLeader->GetAbsOrigin();
		m_vLastLeaderVelocity = pNewLeader->GetAbsVelocity();
		m_flLastSquadUpdateTime = gpGlobals->curtime;
	}
	else
	{
		m_flLastSquadUpdateTime = 0;
	}
}

void CASW_SquadFormation::Reset()
{
	for ( int i = 0 ; i < MAX_SQUAD_SIZE ; ++i )
	{
		m_flUseHintsAfter[i] = -1;
		m_hSquad[i] = NULL;
		m_bStandingInBeacon[i] = false;
		m_bFleeingBoomerBombs[i] = false;
		m_nMarineHintIndex[i] = INVALID_HINT_INDEX;
	}
	m_flLastSquadUpdateTime = 0;
	m_flLastLeaderYaw = 0;
	m_vLastLeaderPos.Zero();
	m_vLastLeaderVelocity.Zero();
	m_vecObjective = vec3_invalid;
	m_bInMarker = false;
}

//-----------------------------------------------------------------------------
// Purpose: Sorts AI nodes by proximity to leader
//-----------------------------------------------------------------------------
int CASW_SquadFormation::FollowHintSortFunc( HintData_t* const *pHint1, HintData_t* const *pHint2 )
{
	float flDist1 = (*pHint1)->m_flDistance;
	float flDist2 = (*pHint2)->m_flDistance;

	if (flDist1 == flDist2)
	{
		return 0;
	}

	return flDist1 > flDist2 ? 1 : -1;
}

//-----------------------------------------------------------------------------
// Purpose: Finds the set of hint nodes to use when following during combat
//-----------------------------------------------------------------------------
void CASW_SquadFormation::FindFollowHintNodes()
{
	VPROF_BUDGET("CASW_SquadFormation::FindFollowHintNodes", "SquadFormation");

	CASW_Marine *pLeader = Leader();
	if ( !pLeader )
		return;

	Vector vecLeader = GetLeaderPosition();

	CUtlVector<HintData_t *> hints;
	int nCount = -1;

	// evaluate each squaddie individually to see if his node should be updated
	for ( unsigned slotnum = 0; slotnum < MAX_SQUAD_SIZE; slotnum++ )
	{
		CASW_Marine *pMarine = Squaddie( slotnum );
		if ( !pMarine || pMarine->IsInhabited() )
		{
			// Missing or inhabited marines don't take nodes.
			m_nMarineHintIndex[slotnum] = INVALID_HINT_INDEX;
			continue;
		}

		bool bNeedNewNode = ( m_vFollowPositions[slotnum].DistToSqr( pMarine->GetAbsOrigin() ) > Square( asw_follow_hint_min_range.GetFloat() ) ) || ( m_vFollowPositions[slotnum].DistToSqr( vecLeader ) > Square( asw_follow_hint_max_range.GetFloat() ) ) || !pMarine->FVisible( pLeader ) || m_nMarineHintIndex[slotnum] == INVALID_HINT_INDEX;
		if ( slotnum != SQUAD_LEADER && !bNeedNewNode )
			bNeedNewNode = ( m_vFollowPositions[slotnum].DistToSqr( vecLeader ) < Square( asw_follow_hint_min_range.GetFloat() ) );

		// find shield bug (if any) nearest each marine
		const float k_flShieldbugScanRangeSqr = Square(400.0f);
		CASW_Shieldbug *pClosestShieldbug = NULL;
		float flClosestShieldBugDistSqr = k_flShieldbugScanRangeSqr;

		if ( pMarine->IsAlive() )
		{
			for( int iShieldbug = 0; iShieldbug < IShieldbugAutoList::AutoList().Count(); iShieldbug++ )
			{
				CASW_Shieldbug *pShieldbug = static_cast< CASW_Shieldbug* >( IShieldbugAutoList::AutoList()[ iShieldbug ] );
				if( pShieldbug && pShieldbug->IsAlive() && pShieldbug->GetSleepState() == AISS_AWAKE )
				{
					float flDistSqr = pMarine->GetAbsOrigin().DistToSqr( pShieldbug->GetAbsOrigin() );
					if( flDistSqr < flClosestShieldBugDistSqr )
					{
						flClosestShieldBugDistSqr = flDistSqr;
						pClosestShieldbug = pShieldbug;
					}
				}
			}
		}

		if ( !bNeedNewNode && !pClosestShieldbug )
			continue;

		// clear out the old hint so we don't get stuck if there are no good hints
		m_nMarineHintIndex[slotnum] = INVALID_HINT_INDEX;

		// find a new node
		if (nCount == -1)
		{
			MarineHintManager()->FindHints(vecLeader, pLeader, asw_follow_hint_min_range.GetFloat(), asw_follow_hint_max_range.GetFloat(), hints);
			nCount = hints.Count();

			hints.Sort(CASW_SquadFormation::FollowHintSortFunc);
		}

		float flMovementYaw = pLeader->GetOverallMovementDirection();

		int iClosestFlankingNode = INVALID_HINT_INDEX;
		float flClosestFlankingNodeDistSqr = FLT_MAX;

		CBaseTrigger *pEscapeVolume = pLeader->IsInEscapeVolume();
		if ( pEscapeVolume && pEscapeVolume->CollisionProp() )
		{
			// remove hints that aren't in the escape volume bounds
			for ( int i = nCount - 1; i >= 0; i-- )
			{
				if ( !pEscapeVolume->CollisionProp()->IsPointInBounds( hints[ i ]->GetAbsOrigin() ) )
				{
					hints.Remove( i );
					nCount--;
				}
				else if ( asw_follow_hint_debug.GetBool() )
				{
					NDebugOverlay::Box( hints[ i ]->GetAbsOrigin(), -Vector( 5, 5, 0 ), Vector( 5, 5, 5 ), 255, 0, 0, 255, 3.0f );
				}
			}
		}
		else
		{
			// remove hints that are in front of the leader's overall direction of movement
			// TODO: turn this into a hint filter
			for ( int i = nCount - 1; i >= 0; i-- )
			{
				Vector vecDir = ( hints[ i ]->GetAbsOrigin() - pLeader->GetAbsOrigin() );
				float flYaw = UTIL_VecToYaw( vecDir );
				flYaw = AngleDiff( flYaw, flMovementYaw );
				bool bRemoveNode = false;

				// remove hints that are in front of the leader's overall direction of movement,
				if ( flYaw < 85.0f && flYaw > -85.0f && vecDir.LengthSqr() > Square( asw_follow_hint_min_range.GetFloat() ) )
				{
					bRemoveNode = true;
				}

				// unless we need to use them to get the AI to flank a shieldbug
				if( pClosestShieldbug )
				{
					bRemoveNode = true;
					// if any of the marines are close, don't delete nodes behind the shieldbug
					float flShieldbugDistSqr = hints[ i ]->GetAbsOrigin().DistToSqr( pClosestShieldbug->GetAbsOrigin() );
					if( flShieldbugDistSqr < k_flShieldbugScanRangeSqr )
					{
						// preserve the node if it's behind the shieldbug
						Vector vecShieldBugToNode, vecShieldbugFacing;

						vecShieldBugToNode = hints[ i ]->GetAbsOrigin() - pClosestShieldbug->GetAbsOrigin();
						QAngle angFacing = pClosestShieldbug->GetAbsAngles();
						AngleVectors( angFacing, &vecShieldbugFacing );
						vecShieldbugFacing.z = 0;
						vecShieldBugToNode.z = 0;

						VectorNormalize( vecShieldbugFacing );
						VectorNormalize( vecShieldBugToNode );

						float flForwardDot = vecShieldbugFacing.Dot( vecShieldBugToNode );
						if( flForwardDot < 0.5f )	// if node is 60 degrees or more away from shieldbug's facing...
						{
							float flDistSqr = hints[ i ]->GetAbsOrigin().DistToSqr( pMarine->GetAbsOrigin() );
							bool bHasLOS = pMarine->TestShootPosition( pMarine->GetAbsOrigin(), hints[ i ]->GetAbsOrigin() );

							// if closer than the previous closest node, and the current node isn't taken, reserve it
							if( flDistSqr < flClosestFlankingNodeDistSqr && bHasLOS )
							{
								bool flNodeTaken = false;
								for( unsigned iSlot = 0; iSlot < MAX_SQUAD_SIZE; iSlot++ )
								{
									if ( iSlot != slotnum && hints[ i ]->m_nHintIndex == m_nMarineHintIndex[ iSlot ] )
									{
										flNodeTaken = true;
										break;
									}
								}

								if( !flNodeTaken )
								{
									iClosestFlankingNode = hints[ i ]->m_nHintIndex;
									flClosestFlankingNodeDistSqr = flDistSqr;
									bRemoveNode = false;
								}
							}
						}
					}
				}

				// if zdiff is too great, remove
				float flZDiff = fabs( hints[ i ]->GetAbsOrigin().z - pLeader->GetAbsOrigin().z );
				if ( flZDiff > asw_follow_hint_max_z_dist.GetFloat() )
				{
					bRemoveNode = true;
				}

				if( bRemoveNode )
				{
					hints.Remove( i );
					nCount--;
				}
			}
		}

		// if this marine is close to a shield bug, grab a flanking node
		if( !pEscapeVolume && pClosestShieldbug && iClosestFlankingNode != INVALID_HINT_INDEX )
		{
			m_nMarineHintIndex[ slotnum ] = iClosestFlankingNode;
			continue;
		}

		if (pEscapeVolume)
		{
			// If we're escaping, pick the closest node to us - we don't care if it's already taken.
			FOR_EACH_VEC( hints, nNode )
			{
				bool bValidNode = true;
				const Vector &vecNodePos = hints[nNode]->m_vecPosition;
				for ( unsigned k = 0; k < MAX_SQUAD_SIZE; k++ )
				{
					if ( k != slotnum && m_nMarineHintIndex[k] != INVALID_HINT_INDEX && vecNodePos.DistToSqr( MarineHintManager()->GetHintPosition( m_nMarineHintIndex[k] ) ) < Square( asw_follow_hint_min_range.GetFloat() ) )
					{
						bValidNode = false;
						break;
					}
				}
				if ( bValidNode )
				{
					m_nMarineHintIndex[slotnum] = hints[nNode]->m_nHintIndex;
					break;
				}
			}
		}
		else
		{
			// find the first node not used by another other squaddie
			FOR_EACH_VEC( hints, nNode )
			{
				bool bValidNode = true;
				const Vector &vecNodePos = hints[nNode]->m_vecPosition;
				for ( unsigned k = 0; k < MAX_SQUAD_SIZE; k++ )
				{
					if ( k != slotnum && m_nMarineHintIndex[k] != INVALID_HINT_INDEX && vecNodePos.DistToSqr( MarineHintManager()->GetHintPosition( m_nMarineHintIndex[k] ) ) < Square( asw_follow_hint_min_range.GetFloat() ) )
					{
						bValidNode = false;
						break;
					}
				}
				if ( bValidNode )
				{
					m_nMarineHintIndex[slotnum] = hints[nNode]->m_nHintIndex;
					break;
				}
			}
		}
	}
}

void CASW_SquadFormation::UpdateGoalPosition()
{
	Assert(Leader());
	if (!Leader())
	{
		return;
	}

	Vector vecPrevObjective = m_vecObjective;

	m_vecObjective = vec3_invalid;
	for (int i = 0; i < ASW_MAX_OBJECTIVES; i++)
	{
		CASW_Objective *pObj = ASWGameResource()->GetObjective(i);
		if (pObj && !pObj->IsObjectiveComplete() && !pObj->IsObjectiveFailed() && !pObj->IsObjectiveHidden() && !pObj->IsObjectiveDummy())
		{
			float flMinDist = -1;
			Vector vecBest = vec3_origin;

			int iPriority = 0;
			Vector2D oldStyleMarker = pObj->GetOldStyleMarkerLocation();
			if (oldStyleMarker != vec2_invalid)
			{
				iPriority = 1;
				vecBest.x = oldStyleMarker.x;
				vecBest.y = oldStyleMarker.y;
				vecBest.z = Leader()->GetAbsOrigin().z; // assume the objective is around the same z-height as where we are now.
				flMinDist = vecBest.DistToSqr(Leader()->GetAbsOrigin());
			}

			CASW_Marker *pMarker = NULL;
			bool bInMarker = false;

			while ((pMarker = dynamic_cast<CASW_Marker *>(gEntList.FindEntityByClassname(pMarker, "asw_marker"))) != NULL)
			{
				if (pMarker->GetObjective() == pObj && !pMarker->IsObjectiveComplete() && pMarker->IsMarkerEnabled())
				{
					Vector vecMarker = pMarker->GetAbsOrigin();
					vecMarker.x += RandomInt(-pMarker->GetMapWidth() / 2, pMarker->GetMapWidth() / 2);
					vecMarker.y += RandomInt(-pMarker->GetMapHeight() / 2, pMarker->GetMapHeight() / 2);

					float flDist = vecMarker.DistToSqr(Leader()->GetAbsOrigin());
					if (iPriority < 3 || flMinDist == -1 || flDist < flMinDist)
					{
						flMinDist = flDist;
						vecBest = vecMarker;
						iPriority = 3;

						bInMarker = fabs(pMarker->GetAbsOrigin().x - Leader()->GetAbsOrigin().x) <= (pMarker->GetMapWidth() / 2.0f) &&
							fabs(pMarker->GetAbsOrigin().y - Leader()->GetAbsOrigin().y) <= (pMarker->GetMapHeight() / 2.0f);
					}
				}
				else if (iPriority <= 2)
				{
					Vector vecMarker = pMarker->GetAbsOrigin();
					vecMarker.x += RandomInt(-pMarker->GetMapWidth() / 2, pMarker->GetMapWidth() / 2);
					vecMarker.y += RandomInt(-pMarker->GetMapHeight() / 2, pMarker->GetMapHeight() / 2);

					float flDist = vecMarker.DistToSqr(Leader()->GetAbsOrigin());

					if (iPriority < 2 || flMinDist > flDist)
					{
						flMinDist = flDist;
						vecBest = vecMarker;
						iPriority = 2;

						bInMarker = fabs(pMarker->GetAbsOrigin().x - Leader()->GetAbsOrigin().x) <= (pMarker->GetMapWidth() / 2.0f) &&
							fabs(pMarker->GetAbsOrigin().y - Leader()->GetAbsOrigin().y) <= (pMarker->GetMapHeight() / 2.0f);
					}
				}
			}
			if (bInMarker)
			{
				if (dynamic_cast<CASW_Objective_Kill_Aliens *>(pObj))
				{
					ASW_Alien_Class_Entry *pEntry = ASWSpawnManager()->GetAlienClass(dynamic_cast<CASW_Objective_Kill_Aliens *>(pObj)->m_AlienClassNum);
					Assert(pEntry);
					if (pEntry)
					{
						CBaseEntity *pEnt = gEntList.FindEntityByClassnameNearestFast(pEntry->m_iszAlienClass, Leader()->GetAbsOrigin(), 0);
						Assert(pEnt);
						if (pEnt)
						{
							vecBest = pEnt->GetAbsOrigin();
						}
					}
				}
				else if (dynamic_cast<CASW_Objective_Kill_Eggs *>(pObj))
				{
					CBaseEntity *pEnt = gEntList.FindEntityByClassnameNearest("asw_egg", Leader()->GetAbsOrigin(), 0);
					Assert(pEnt);
					if (pEnt)
					{
						vecBest = pEnt->GetAbsOrigin();
					}
				}
				else if (dynamic_cast<CASW_Objective_Kill_Goo *>(pObj))
				{
					CBaseEntity *pEnt = gEntList.FindEntityByClassnameNearest("asw_alien_goo", Leader()->GetAbsOrigin(), 0);
					Assert(pEnt);
					if (pEnt)
					{
						vecBest = pEnt->GetAbsOrigin();
					}
				}
				else if (dynamic_cast<CASW_Objective_Kill_Queen *>(pObj))
				{
					CBaseEntity *pEnt = gEntList.FindEntityByClassnameNearest("asw_queen", Leader()->GetAbsOrigin(), 0);
					Assert(pEnt);
					if (pEnt)
					{
						vecBest = pEnt->GetAbsOrigin();
					}
				}
				else if (dynamic_cast<CASW_Objective_Escape *>(pObj))
				{
					Assert(ASWSpawnManager());
					Assert(ASWSpawnManager()->m_EscapeTriggers.Count() == 1);
					CTriggerMultiple *pEnt = (ASWSpawnManager() && ASWSpawnManager()->m_EscapeTriggers.Count()) ? ASWSpawnManager()->m_EscapeTriggers.Head() : NULL;
					Assert(pEnt);
					if (pEnt)
					{
						Vector mins = pEnt->CollisionProp()->OBBMins();
						Vector maxs = pEnt->CollisionProp()->OBBMaxs();
						if (pEnt->CollisionProp()->IsBoundsDefinedInEntitySpace())
						{
							mins += pEnt->GetAbsOrigin();
							maxs += pEnt->GetAbsOrigin();
						}
						vecBest.x = RandomFloat(mins.x, maxs.x);
						vecBest.y = RandomFloat(mins.y, maxs.y);
						vecBest.z = RandomFloat(mins.z, maxs.z);
					}
					FollowCommandUsed();
				}
				else if (dynamic_cast<CASW_Objective_Triggered *>(pObj))
				{
					string_t iName = pObj->GetEntityName();
					Assert(iName);

					CBaseEntity *pEnt = assert_cast<CASW_Objective_Triggered *>(pObj)->FindTriggerEnt();
					Assert(pEnt);
					if (pEnt)
					{
						Vector mins = pEnt->CollisionProp()->OBBMins();
						Vector maxs = pEnt->CollisionProp()->OBBMaxs();
						if (pEnt->CollisionProp()->IsBoundsDefinedInEntitySpace())
						{
							mins += pEnt->GetAbsOrigin();
							maxs += pEnt->GetAbsOrigin();
						}
						vecBest.x = RandomFloat(mins.x, maxs.x);
						vecBest.y = RandomFloat(mins.y, maxs.y);
						vecBest.z = RandomFloat(mins.z, maxs.z);
					}

					if (RandomFloat() < 0.1f)
					{
						FollowCommandUsed();
					}
				}
				else
				{
					AssertMsg(false, "not prepared for this objective type");
				}
			}
			if (flMinDist != -1)
			{
				m_bInMarker = bInMarker;
				m_vecObjective = vecBest;
				if (asw_squad_debug.GetInt() >= 2)
				{
					Msg("%s: Objective position (%f %f %f) -> (%f %f %f), in marker: %s\n", Leader()->GetDebugName(), VectorExpand(vecPrevObjective), VectorExpand(vecBest), bInMarker ? "true" : "false");
				}
				return;
			}
		}
	}
	if (ASWGameRules() && ASWGameRules()->GetGameState() == ASW_GS_INGAME)
		DevWarning("%s: Could not find an incomplete objective with a marker.\n", Leader()->GetDebugName());
}

Vector CASW_SquadFormation::GetLeaderPosition()
{
	CASW_Marine *pLeader = Leader();
	Assert(pLeader);
	Vector position = pLeader->GetAbsOrigin();
	if (!pLeader->IsInhabited() && m_vecObjective != vec3_invalid)
	{
		AI_Waypoint_t *pPath = pLeader->GetPathfinder()->BuildRoute(position, m_vecObjective, NULL, 0, NAV_GROUND);
		if (pPath)
		{
			AI_Waypoint_t *pCur = pPath;
			while (pCur && pCur->GetPos().AsVector2D().DistToSqr(position.AsVector2D()) < Square(100))
			{
				pCur = pCur->GetNext();
			}
			if (pCur)
			{
				position = pCur->GetPos();
			}
			DeleteAll(pPath);
		}
	}
	return position;
}

bool CASW_SquadFormation::ShouldUpdateFollowPositions() const
{
	return Leader() && gpGlobals->curtime > m_flLastSquadUpdateTime + 1;
}

void CASW_SquadFormation::DrawDebugGeometryOverlays()
{
	if ( !asw_squad_debug.GetBool() )
		return;

	CASW_Marine *pLeader = Leader();
	if ( !pLeader )
		return;

	for ( int i = 0; i < MAX_SQUAD_SIZE; i++ )
	{
		CASW_Marine *pMarine = Squaddie( i );
		if ( pMarine )
		{
			NDebugOverlay::Line( pMarine->WorldSpaceCenter(), pLeader->WorldSpaceCenter(), 63, 63, 63, false, 0.05f );
			if ( m_nMarineHintIndex[ i ] != INVALID_HINT_INDEX )
			{
				NDebugOverlay::Line( pMarine->WorldSpaceCenter(), MarineHintManager()->GetHintPosition( m_nMarineHintIndex[ i ] ), 255, 255, 63, false, 0.05f );
			}
		}
	}
}

void CASW_SquadFormation::FollowCommandUsed( unsigned slotnum )
{
	if ( slotnum == INVALID_SQUADDIE )
	{
		for ( int i = 0; i < MAX_SQUAD_SIZE; i++ )
		{
			FollowCommandUsed(i);
		}
		return;
	}

	Assert( IsValid( slotnum ) );

	// skip the follow hint delay when the first (automated) follow command comes through at mission start.
	if ( m_flUseHintsAfter[slotnum] == -1 )
		m_flUseHintsAfter[slotnum] = 0;
	else
		m_flUseHintsAfter[slotnum] = gpGlobals->curtime + asw_follow_hint_delay.GetFloat();
}