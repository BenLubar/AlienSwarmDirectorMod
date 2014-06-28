//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "asw_gamestats.h"

// Must run with -gamestats to be able to turn on/off stats with ConVar below.
static ConVar asw_stats_track( "asw_stats_track", "0", FCVAR_ARCHIVE|FCVAR_HIDDEN, "Turn on//off Infested stats tracking." );
static ConVar asw_stats_verbose( "asw_stats_verbose", "0", FCVAR_NONE|FCVAR_HIDDEN, "Turn on//off verbose logging of stats." );
static ConVar asw_stats_nogameplaycheck( "asw_stats_nogameplaycheck", "0", FCVAR_NONE|FCVAR_HIDDEN, "Disable normal check for valid gameplay, send stats regardless." );

CASWGameStats CASW_GameStats;