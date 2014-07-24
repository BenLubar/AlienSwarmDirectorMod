#include "cbase.h"
#include "asw_marine.h"
#include "asw_marine_profile.h"
#include "ai_hint.h"
#include "asw_marine_hint.h"
#include "asw_weapon_healgrenade_shared.h"
#include "asw_shieldbug.h"
#include "asw_boomer_blob.h"
#include "ai_network.h"
#include "triggers.h"
#include "asw_player.h"

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
	AssertMsg( !!m_hLeader.Get(), "A CASW_SquadFormation has no leader!\n" );
	if ( pMarine == Leader() )
	{
		AssertMsg1( false, "Tried to set %s to follow itself!\n", pMarine->GetMarineProfile()->GetShortName() );
		return INVALID_SQUADDIE;
	}

	unsigned slot = Find(pMarine);
	if ( IsValid( slot ) )
	{
		AssertMsg2( false, "Tried to double-add %s to squad (already in slot %d)\n",
			pMarine->GetMarineProfile()->GetShortName(),
			slot );
		return slot;
	}
	else
	{
		for ( slot = 0 ; slot < MAX_SQUAD_SIZE; ++slot )
		{
			if ( !Squaddie(slot) )
			{
				m_hSquad[slot] = pMarine;
				return slot;
			}
		}

		// if we're down here, the squad is full!
		AssertMsg2( false, "Tried to add %s to %s's squad, but that's full! (How?)\n",
			pMarine->GetMarineProfile()->GetShortName(), m_hLeader->GetMarineProfile()->GetShortName()	);
		return INVALID_SQUADDIE;
	}
}

bool CASW_SquadFormation::Remove( unsigned int slotnum )
{
	Assert(IsValid(slotnum));
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
	unsigned slot = Find(pMarine);
	if ( IsValid(slot) )
	{
		m_hSquad[slot] = NULL;
		return true;
	}
	else
	{
		AssertMsg1( bIgnoreAssert, "Tried to remove marine %s from squad, but wasn't a member.\n",
			pMarine->GetMarineProfile()->GetShortName() );
		return false;
	}
}

const  Vector CASW_SquadFormation::s_MarineFollowOffset[MAX_SQUAD_SIZE]=
{
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
#ifdef HL2_HINTS
		if ( m_hFollowHint[ slotnum ].Get() )
		{
// 			if ( m_bRearGuard[ slotnum ] )
// 			{
// 				return m_hFollowHint[ slotnum ]->Yaw() + 180.0f;
// 			}
			return m_hFollowHint[ slotnum ]->Yaw();
		}
#else
		if ( m_nMarineHintIndex[ slotnum ] != INVALID_HINT_INDEX )
		{
			// 			if ( m_bRearGuard[ slotnum ] )
			// 			{
			// 				return m_hFollowHint[ slotnum ]->Yaw() + 180.0f;
			// 			}
			return MarineHintManager()->GetHintYaw( m_nMarineHintIndex[ slotnum ] );
		}
#endif
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
	float flClosestBoomerBlobDistSqr = Square(OUT_OF_BOOMER_BOMB_RANGE);

	for( int iBoomerBlob = 0; iBoomerBlob < g_aExplosiveProjectiles.Count(); iBoomerBlob++ )
	{
 		CBaseEntity *pExplosive = g_aExplosiveProjectiles[ iBoomerBlob ];

		float flDistSqr = GetClosestPointToSegmentDistSqr(vecFrom, vecNode, pExplosive->GetAbsOrigin());
		flClosestBoomerBlobDistSqr = MIN( flDistSqr, flClosestBoomerBlobDistSqr );
	}

	return flClosestBoomerBlobDistSqr;
}

void CASW_SquadFormation::UpdateFollowPositions()
{
	VPROF("CASW_SquadFormation::UpdateFollowPositions");
	CASW_Marine * RESTRICT pLeader = Leader();
	if ( !pLeader )
	{
		AssertMsg1(false, "Tried to update positions for a squad with no leader and %d followers.\n",
			Count() 	);
		return;
	}
	m_flLastSquadUpdateTime = gpGlobals->curtime;

	if ( MarineHintManager()->GetHintCount() )
	{
		FindFollowHintNodes();
	}

	QAngle angLeaderFacing = pLeader->EyeAngles();
	angLeaderFacing[PITCH] = 0;
	matrix3x4_t matLeaderFacing;
	Vector vProjectedLeaderPos = pLeader->GetAbsOrigin() + pLeader->GetAbsVelocity() * asw_follow_velocity_predict.GetFloat();
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

				bool bNodeTaken = vecNodeLocation.DistToSqr( m_vLastLeaderPos ) < Square( 30.0f );
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
#ifdef HL2_HINTS
			if ( m_hFollowHint[i].Get() )
			{
				m_vFollowPositions[i] = m_hFollowHint[i]->GetAbsOrigin();
			}
#else
			if ( m_nMarineHintIndex[i] != INVALID_HINT_INDEX )
			{
				m_vFollowPositions[i] = MarineHintManager()->GetHintPosition( m_nMarineHintIndex[i] );
			}
#endif
			else
			{
				if ( pMarine )
				{
					VectorTransform(s_MarineFollowOffset[i], matLeaderFacing, m_vFollowPositions[i]);
				}
			}
		}
		else
		{
			VectorTransform( s_MarineFollowOffset[i], matLeaderFacing, m_vFollowPositions[i] );
		}
		if ( asw_marine_ai_followspot.GetBool() )
		{
			static float colors[MAX_SQUAD_SIZE][3] = { { 255, 64, 64 }, { 64, 255, 64 }, { 64, 64, 255 }, { 255, 255, 64 }, { 255, 64, 255 }, { 64, 255, 255 }, { 64, 64, 64 } };
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
		Assert( pLeader != pTest );  // am not leader
		if ( pLeader == pTest ) 
			return false;

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
	Assert( pMarine != Leader() );
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
		Leader( pNewLeader );
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
		AssertMsg2( Count() == 0, "Tried to unset squad leader %s, but squad has %d followers\n",
			pNewLeader->GetMarineProfile()->GetShortName(), Count() );
		Leader(NULL);
		return;
	}

	if ( pOldLeader == pNewLeader )
	{
		AssertMsg1( false, "Tried to reset squad leader to its current value (%s)\n", pNewLeader->GetMarineProfile()->GetShortName() );
		return;
	}

	// if the new leader was previously a follower, swap with the old leader
	int slot = Find( pNewLeader );
	if ( IsValid(slot) )
	{
		m_hSquad[slot] = pOldLeader;
		Leader( pNewLeader );
	}
	else
	{
		// make the old leader a follower 
		Leader( pNewLeader );
		Add( pOldLeader );
	}
	if ( bUpdateLeaderPos )
	{
		m_flLastLeaderYaw = pNewLeader->EyeAngles()[ YAW ];
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
	m_hLeader = NULL;
	for ( int i = 0 ; i < MAX_SQUAD_SIZE ; ++i )
	{
		m_flUseHintsAfter[i] = -1;
		m_hSquad[i] = NULL;
		m_bRearGuard[i] = false;
		m_bStandingInBeacon[i] = false;
		m_bFleeingBoomerBombs[ i ] = false;
#ifdef HL2_HINTS
		m_hFollowHint[i] = NULL;
#else
		m_nMarineHintIndex[i] = INVALID_HINT_INDEX;
#endif
	}
	m_flLastSquadUpdateTime = 0;
	m_vLastLeaderVelocity.Zero();
}

//-----------------------------------------------------------------------------
// Purpose: Sorts AI nodes by proximity to leader
//-----------------------------------------------------------------------------
CASW_Marine *g_pSortLeader = NULL;
#ifdef HL2_HINTS
int CASW_SquadFormation::FollowHintSortFunc( CAI_Hint* const *pHint1, CAI_Hint* const *pHint2 )
#else
int CASW_SquadFormation::FollowHintSortFunc( HintData_t* const *pHint1, HintData_t* const *pHint2 )
#endif
{
	int nDist1 = (int) (*pHint1)->GetAbsOrigin().DistToSqr( g_pSortLeader->GetAbsOrigin() );
	int nDist2 = (int) (*pHint2)->GetAbsOrigin().DistToSqr( g_pSortLeader->GetAbsOrigin() );

	return ( nDist1 - nDist2 );
}

//-----------------------------------------------------------------------------
// Purpose: Finds the set of hint nodes to use when following during combat
//-----------------------------------------------------------------------------
void CASW_SquadFormation::FindFollowHintNodes()
{
	CASW_Marine *pLeader = Leader();
	if ( !pLeader )
		return;

	// evaluate each squaddie individually to see if his node should be updated
	for ( int slotnum = 0; slotnum < MAX_SQUAD_SIZE; slotnum++ )
	{
		CASW_Marine *pMarine = Squaddie( slotnum );
		if ( !pMarine )
		{
#ifdef HL2_HINTS
			m_hFollowHint[slotnum] = NULL;
#else
			m_nMarineHintIndex[slotnum] = INVALID_HINT_INDEX;
#endif
			continue;
		}

		bool bNeedNewNode = (pMarine->GetAbsOrigin().DistToSqr(pLeader->GetAbsOrigin()) < Square(asw_follow_hint_min_range.GetFloat())) || (pMarine->GetAbsOrigin().DistToSqr(pLeader->GetAbsOrigin()) > Square(asw_follow_hint_max_range.GetFloat())) || !pMarine->FVisible(pLeader) || m_nMarineHintIndex[slotnum] == INVALID_HINT_INDEX;

		// find shield bug (if any) nearest each marine
		const float k_flShieldbugScanRangeSqr = Square(400.0f);
		CASW_Shieldbug *pClosestShieldbug = NULL;
		float flClosestShieldBugDistSqr = k_flShieldbugScanRangeSqr;

		if ( pMarine->IsAlive() )
		{
			for( int iShieldbug = 0; iShieldbug < IShieldbugAutoList::AutoList().Count(); iShieldbug++ )
			{
				CASW_Shieldbug *pShieldbug = static_cast< CASW_Shieldbug* >( IShieldbugAutoList::AutoList()[ iShieldbug ] );
				if( pShieldbug && pShieldbug->IsAlive() )
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
#ifdef HL2_HINTS
		m_hFollowHint[slotnum] = NULL;
#else
		m_nMarineHintIndex[slotnum] = INVALID_HINT_INDEX;
#endif

		// find a new node
#ifdef HL2_HINTS
		CHintCriteria hintCriteria;
		hintCriteria.SetHintType( HINT_FOLLOW_WAIT_POINT );
		hintCriteria.AddIncludePosition( pLeader->GetAbsOrigin(), asw_follow_hint_max_range.GetFloat() );
		hintCriteria.AddExcludePosition( pLeader->GetAbsOrigin(), asw_follow_hint_min_range.GetFloat() );

		CUtlVector< CAI_Hint * > hints;
		CAI_HintManager::FindAllHints( pLeader, pLeader->GetAbsOrigin(), hintCriteria, &hints );
#else
		CUtlVector< HintData_t* > hints;
		MarineHintManager()->FindHints(pLeader->GetAbsOrigin(), asw_follow_hint_min_range.GetFloat(), asw_follow_hint_max_range.GetFloat(), &hints);
#endif
		int nCount = hints.Count();

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
				Vector vecDir = ( hints[ i ]->GetAbsOrigin() - pLeader->GetAbsOrigin() ).Normalized();
				float flYaw = UTIL_VecToYaw( vecDir );
				flYaw = AngleDiff( flYaw, flMovementYaw );
				bool bRemoveNode = false;

				// remove hints that are in front of the leader's overall direction of movement,
				if (flYaw < 85.0f && flYaw > -85.0f)
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
								for( int iSlot = 0; iSlot < MAX_SQUAD_SIZE; iSlot++ )
								{
#ifdef HL2_HINTS
									if ( iSlot != slotnum && hints[ i ] == m_hFollowHint[ iSlot ].Get() )
#else
									if ( iSlot != slotnum && hints[ i ]->m_nHintIndex == m_nMarineHintIndex[ iSlot ] )
#endif
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

		g_pSortLeader = pLeader;
		hints.Sort( CASW_SquadFormation::FollowHintSortFunc );

		// if this marine is close to a shield bug, grab a flanking node
		if( !pEscapeVolume && pClosestShieldbug && iClosestFlankingNode != INVALID_HINT_INDEX )
		{
			m_nMarineHintIndex[ slotnum ] = iClosestFlankingNode;
			continue;
		}

		// find the first node not used by another other squaddie
		int nNode = 0;

		while ( nNode < hints.Count() )
		{
			bool bValidNode = true;
#ifdef HL2_HINTS
			const Vector &vecNodePos = hints[nNode]->GetAbsOrigin();
#else
			const Vector &vecNodePos = hints[nNode]->m_vecPosition;
#endif
			for ( int k = 0; k < MAX_SQUAD_SIZE; k++ )
			{
#ifdef HL2_HINTS
				if ( k != slotnum && m_hFollowHint[k].Get() && vecNodePos.DistToSqr( m_hFollowHint[k].Get()->GetAbsOrigin() ) < Square( asw_follow_hint_min_range.GetFloat() ) )
#else
				if ( k != slotnum && m_nMarineHintIndex[k] != INVALID_HINT_INDEX && vecNodePos.DistToSqr( MarineHintManager()->GetHintPosition( m_nMarineHintIndex[k] ) ) < Square( asw_follow_hint_min_range.GetFloat() ) )
#endif
				{
					bValidNode = false;
					break;
				}
			}
			if ( bValidNode )
			{
#ifdef HL2_HINTS
				m_hFollowHint[ slotnum ] = hints[ nNode ];
#else
				m_nMarineHintIndex[ slotnum ] = hints[ nNode ]->m_nHintIndex;
#endif
				nNode++;
				break;
			}
			nNode++;
		}
	}
}



bool CASW_SquadFormation::ShouldUpdateFollowPositions() const
{
	// we haven't updated in a quarter second.
	return Leader() && gpGlobals->curtime > m_flLastSquadUpdateTime + 0.25f;
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
#ifdef HL2_HINTS
			if ( m_hFollowHint[ i ].Get() )
			{
				NDebugOverlay::Line( pMarine->WorldSpaceCenter(), m_hFollowHint[ i ]->GetAbsOrigin(), 255, 255, 63, false, 0.05f );
				NDebugOverlay::EntityText( pMarine->entindex(), i * 2,
					CFmtStr( "Node: %d wc: %d node: %d pos: %f %f %f",
						m_hFollowHint[ i ]->GetNodeId(),
						m_hFollowHint[i]->GetWCId(),
						m_hFollowHint[i]->GetNode() ? m_hFollowHint[i]->GetNode()->GetId() : -1,
						m_hFollowHint[i]->GetAbsOrigin().x,
						m_hFollowHint[i]->GetAbsOrigin().y,
						m_hFollowHint[i]->GetAbsOrigin().z ),
					0.05f, 255, 255, 255, 255 );
			}
#else
			if ( m_nMarineHintIndex[ i ] != INVALID_HINT_INDEX )
			{
				NDebugOverlay::Line( pMarine->WorldSpaceCenter(), MarineHintManager()->GetHintPosition( m_nMarineHintIndex[ i ] ), 255, 255, 63, false, 0.05f );
			}
#endif
		}
	}

	/*
	int max_marines = ASWGameResource()->GetMaxMarineResources();
	for ( int i=0;i<max_marines;i++ )
	{		
		CASW_Marine_Resource* pMR = ASWGameResource()->GetMarineResource( i );
		if ( !pMR )
			continue;
		
		CASW_Marine *pMarine = pMR->GetMarineEntity();
		if ( !pMarine )
			continue;

		NDebugOverlay::EntityText( pMarine->entindex(), 0, CFmtStr( "Squad slot: %d", p ), 0.05f, 255, 255, 255, 255 );
	}
	*/
}

void CASW_SquadFormation::FollowCommandUsed( int slotnum )
{
	if ( slotnum == INVALID_SQUADDIE )
	{
		for (int i = 0; i < MAX_SQUAD_SIZE; i++)
		{
			FollowCommandUsed(i);
		}
		return;
	}

	Assert(0 <= slotnum && slotnum < MAX_SQUAD_SIZE);

	// skip the follow hint delay when the first (automated) follow command comes through at mission start.
	if (m_flUseHintsAfter[slotnum] == -1)
		m_flUseHintsAfter[slotnum] = 0;
	else
		m_flUseHintsAfter[slotnum] = gpGlobals->curtime + asw_follow_hint_delay.GetFloat();
}