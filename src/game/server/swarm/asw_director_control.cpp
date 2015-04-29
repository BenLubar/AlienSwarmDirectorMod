//====== Copyright (c) 1996-2009, Valve Corporation, All rights reserved. =====
//
// Purpose: Alien Swarm Director Controller.
//
//=============================================================================
#include "cbase.h"
#include "asw_director_control.h"
#include "asw_director.h"
#include "asw_spawn_manager.h"
#include "asw_marine.h"
#include "asw_marine_resource.h"
#include "asw_game_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( asw_director_control, CASW_Director_Control );

BEGIN_DATADESC( CASW_Director_Control )
	DEFINE_KEYFIELD( m_bWanderersStartEnabled, FIELD_BOOLEAN,	"wanderers" ),
	DEFINE_KEYFIELD( m_bHordesStartEnabled, FIELD_BOOLEAN,	"hordes" ),
	DEFINE_KEYFIELD( m_bDirectorControlsSpawners, FIELD_BOOLEAN,	"controlspawners" ),
	DEFINE_KEYFIELD( m_flPreSpawnAliens, FIELD_FLOAT,	"prespawn" ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"EnableHordes",	InputEnableHordes ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"DisableHordes",	InputDisableHordes ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"EnableWanderers",	InputEnableWanderers ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"DisableWanderers",	InputDisableWanderers ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ForceHorde",	InputForceHorde),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"ForceHordeSize",	InputForceHordeSize),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ForceWanderer",	InputForceWanderer),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ClearIntensity",	InputClearIntensity ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"StartFinale",	InputStartFinale ),
	DEFINE_OUTPUT( m_OnDirectorSpawnedAlien, "OnDirectorSpawnedAlien" ),
	DEFINE_OUTPUT( m_OnEscapeRoomStart, "OnEscapeRoomStart"),
END_DATADESC()

void CASW_Director_Control::OnEscapeRoomStart( CASW_Marine *pMarine )
{
	m_OnEscapeRoomStart.FireOutput( pMarine, this );
}

void CASW_Director_Control::InputEnableHordes( inputdata_t &inputdata )
{
	if ( !ASWDirector() )	
		return;

	ASWDirector()->SetHordesEnabled( true );
}

void CASW_Director_Control::InputDisableHordes( inputdata_t &inputdata )
{
	if ( !ASWDirector() )	
		return;

	ASWDirector()->SetHordesEnabled( false );
}

void CASW_Director_Control::InputEnableWanderers( inputdata_t &inputdata )
{
	if ( !ASWDirector() )	
		return;

	ASWDirector()->SetWanderersEnabled( true );
}

void CASW_Director_Control::InputDisableWanderers( inputdata_t &inputdata )
{
	if ( !ASWDirector() )	
		return;

	ASWDirector()->SetWanderersEnabled( false );
}

void CASW_Director_Control::InputStartFinale( inputdata_t &inputdata )
{
	if ( !ASWDirector() )	
		return;

	ASWDirector()->StartFinale();
}

void CASW_Director_Control::InputForceHorde( inputdata_t &inputdata )
{
	if ( !ASWSpawnManager() )
		return;

	ASWSpawnManager()->AddRandomHorde();
}

void CASW_Director_Control::InputForceHordeSize( inputdata_t &inputdata )
{
	if ( !ASWSpawnManager() )
		return;

	ASWSpawnManager()->AddHorde( inputdata.value.Int() );
}

void CASW_Director_Control::InputForceWanderer( inputdata_t &inputdata )
{
	if ( !ASWSpawnManager() )
		return;

	ASWSpawnManager()->AddAlien( true );
}

void CASW_Director_Control::InputClearIntensity( inputdata_t &inputdata )
{
	if ( !ASWGameResource() )
		return;

	for ( int i = 0; i < ASWGameResource()->GetMaxMarineResources(); i++ )
	{
		CASW_Marine_Resource *pMR = ASWGameResource()->GetMarineResource( i );
		if ( pMR && pMR->GetIntensity() )
		{
			pMR->GetIntensity()->Reset();
		}
	}
}