#include "cbase.h"
#include "asw_objective.h"
#include "asw_game_resource.h"
#include "asw_mission_manager.h"
#include "asw_gamerules.h"
#include "asw_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( asw_mission_objective, CASW_Objective );

IMPLEMENT_SERVERCLASS_ST(CASW_Objective, DT_ASW_Objective)	
	//SendPropInt		(SENDINFO(m_MarineProfileIndex), 10 ),
	//SendPropEHandle (SENDINFO(m_MarineEntity) ),
	//SendPropEHandle (SENDINFO(m_Commander) ),
	SendPropString( SENDINFO( m_ObjectiveTitle ) ),
	SendPropString( SENDINFO( m_ObjectiveDescription1 ) ),
	SendPropString( SENDINFO( m_ObjectiveDescription2 ) ),
	SendPropString( SENDINFO( m_ObjectiveDescription3 ) ),
	SendPropString( SENDINFO( m_ObjectiveDescription4 ) ),
	SendPropString( SENDINFO( m_ObjectiveImage ) ),
	SendPropString( SENDINFO( m_ObjectiveMarkerName ) ),
	SendPropString( SENDINFO( m_ObjectiveInfoIcon1 ) ),
	SendPropString( SENDINFO( m_ObjectiveInfoIcon2 ) ),
	SendPropString( SENDINFO( m_ObjectiveInfoIcon3 ) ),
	SendPropString( SENDINFO( m_ObjectiveInfoIcon4 ) ),
	SendPropString( SENDINFO( m_ObjectiveInfoIcon5 ) ),
	SendPropString( SENDINFO( m_ObjectiveIcon ) ),
	SendPropString( SENDINFO( m_MapMarkings ) ),	
	SendPropBool( SENDINFO( m_bComplete ) ),
	SendPropBool( SENDINFO( m_bFailed ) ),
	SendPropBool( SENDINFO( m_bOptional ) ),	
	SendPropBool( SENDINFO( m_bDummy ) ),
	SendPropBool( SENDINFO( m_bVisible ) ),
	SendPropInt( SENDINFO( m_Priority ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CASW_Objective )
	// Variables.
	DEFINE_AUTO_ARRAY( m_ObjectiveTitle, FIELD_CHARACTER ),
	DEFINE_AUTO_ARRAY( m_ObjectiveDescription1, FIELD_CHARACTER ),
	DEFINE_AUTO_ARRAY( m_ObjectiveDescription2, FIELD_CHARACTER ),
	DEFINE_AUTO_ARRAY( m_ObjectiveDescription3, FIELD_CHARACTER ),
	DEFINE_AUTO_ARRAY( m_ObjectiveDescription4, FIELD_CHARACTER ),
	DEFINE_AUTO_ARRAY( m_ObjectiveImage, FIELD_CHARACTER ),
	DEFINE_AUTO_ARRAY( m_ObjectiveMarkerName, FIELD_CHARACTER ),
	DEFINE_KEYFIELD( m_Priority, FIELD_INTEGER,	"Priority" ),
	DEFINE_KEYFIELD( m_bOptional, FIELD_BOOLEAN,	"Optional" ),
	DEFINE_KEYFIELD( m_bVisible, FIELD_BOOLEAN,		"Visible" ),
	DEFINE_FIELD( m_bFailed, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bComplete, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDummy, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bVisible, FIELD_BOOLEAN ),
	DEFINE_AUTO_ARRAY( m_ObjectiveInfoIcon1, FIELD_CHARACTER ),
	DEFINE_AUTO_ARRAY( m_ObjectiveInfoIcon2, FIELD_CHARACTER ),
	DEFINE_AUTO_ARRAY( m_ObjectiveInfoIcon3, FIELD_CHARACTER ),
	DEFINE_AUTO_ARRAY( m_ObjectiveInfoIcon4, FIELD_CHARACTER ),
	DEFINE_AUTO_ARRAY( m_ObjectiveInfoIcon5, FIELD_CHARACTER ),
	DEFINE_AUTO_ARRAY( m_ObjectiveIcon, FIELD_CHARACTER ),	
	DEFINE_AUTO_ARRAY( m_MapMarkings, FIELD_CHARACTER ),
	DEFINE_OUTPUT( m_OnObjectiveComplete,		"OnObjectiveComplete" ),

	DEFINE_INPUTFUNC( FIELD_BOOLEAN,	"SetVisible",	InputSetVisible ),
END_DATADESC()


CASW_Objective::CASW_Objective()
{
	m_bFailed = false;
	m_bComplete = false;
	m_bDummy = false;
	m_bVisible = true;
}


CASW_Objective::~CASW_Objective()
{
}

// always send this info to players
int CASW_Objective::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	return FL_EDICT_ALWAYS;
}

void CASW_Objective::SetComplete(bool bComplete)
{
	bool bOld = m_bComplete;
	m_bComplete = bComplete;

	if (!bOld && m_bComplete)
	{
		m_OnObjectiveComplete.FireOutput(this, this);
	}
	
	if (ASWGameRules() && ASWGameRules()->GetMissionManager() && ASWGameRules()->GetGameState() == ASW_GS_INGAME)	// only emit objective complete sounds in the middle of a mission
	{
		if (!ASWGameRules()->GetMissionManager()->CheckMissionComplete() && !ASWGameRules()->IsTutorialMap())
		{
			// play objective complete sound
			if (!bOld && bComplete)
			{
				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CASW_Player* pPlayer = dynamic_cast<CASW_Player*>(UTIL_PlayerByIndex(i));

					if ( pPlayer )
					{
						pPlayer->EmitPrivateSound("Game.ObjectiveComplete");
					}
				}
			}
		}
	}
}

void CASW_Objective::SetFailed(bool bFailed)
{
	m_bFailed = bFailed;

	if (ASWGameRules() && ASWGameRules()->GetMissionManager())
		ASWGameRules()->GetMissionManager()->CheckMissionComplete();
}

void CASW_Objective::AlienKilled(CBaseEntity* pAlien) { }
void CASW_Objective::MarineKilled(CASW_Marine* pMarine) { }
void CASW_Objective::EggKilled(CASW_Egg* pEgg) { }
void CASW_Objective::GooKilled(CASW_Alien_Goo* pGoo) { }
void CASW_Objective::MissionStarted(void) { }
void CASW_Objective::MissionFail(void) { }
void CASW_Objective::MissionSuccess(void) { }

bool CASW_Objective::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "objectivetitle" ) )
	{
		Q_strncpy( m_ObjectiveTitle.GetForModify(), szValue, 255 );
		return true;
	}
	if ( FStrEq( szKeyName, "objectivedescription1" ) )
	{
		Q_strncpy( m_ObjectiveDescription1.GetForModify(), szValue, 255 );
		return true;
	}
	if ( FStrEq( szKeyName, "objectivedescription2" ) )
	{
		Q_strncpy( m_ObjectiveDescription2.GetForModify(), szValue, 255 );
		return true;
	}
	if ( FStrEq( szKeyName, "objectivedescription3" ) )
	{
		Q_strncpy( m_ObjectiveDescription3.GetForModify(), szValue, 255 );
		return true;
	}
	if ( FStrEq( szKeyName, "objectivedescription4" ) )
	{
		Q_strncpy( m_ObjectiveDescription4.GetForModify(), szValue, 255 );
		return true;
	}
	if ( FStrEq( szKeyName, "objectiveimage" ) )
	{
		Q_strncpy( m_ObjectiveImage.GetForModify(), szValue, 255 );
		return true;
	}
	if ( FStrEq( szKeyName, "objectivemarkername" ) )
	{
		Q_strncpy( m_ObjectiveMarkerName.GetForModify(), szValue, 255 );
		return true;
	}
	if ( FStrEq( szKeyName, "objectiveinfoicon1" ) )
	{
		Q_strncpy( m_ObjectiveInfoIcon1.GetForModify(), szValue, 255 );
		return true;
	}
	if ( FStrEq( szKeyName, "objectiveinfoicon2" ) )
	{
		Q_strncpy( m_ObjectiveInfoIcon2.GetForModify(), szValue, 255 );
		return true;
	}
	if ( FStrEq( szKeyName, "objectiveinfoicon3" ) )
	{
		Q_strncpy( m_ObjectiveInfoIcon3.GetForModify(), szValue, 255 );
		return true;
	}
	if ( FStrEq( szKeyName, "objectiveinfoicon4" ) )
	{
		Q_strncpy( m_ObjectiveInfoIcon4.GetForModify(), szValue, 255 );
		return true;
	}
	if ( FStrEq( szKeyName, "objectiveinfoicon5" ) )
	{
		Q_strncpy( m_ObjectiveInfoIcon5.GetForModify(), szValue, 255 );
		return true;
	}
	if ( FStrEq( szKeyName, "objectiveicon" ) )
	{
		Q_strncpy( m_ObjectiveIcon.GetForModify(), szValue, 255 );
		return true;
	}
	if ( FStrEq( szKeyName, "mapmarkings" ) )
	{
		Q_strncpy( m_MapMarkings.GetForModify(), szValue, 255 );
		return true;
	}
	
	
	
	if ( FStrEq( szKeyName, "priority" ) )
	{
		m_Priority = atoi( szValue );
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

void CASW_Objective::InputSetVisible( inputdata_t &inputdata )
{
	m_bVisible = inputdata.value.Bool();
	//Msg("Objective %s SetVisible %d\n", STRING(GetEntityName()), m_bVisible.Get());

	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin( filter, "ShowObjectives" );
	WRITE_FLOAT( 5.0f );
	MessageEnd();
}

Vector2D CASW_Objective::GetOldStyleMarkerLocation()
{
	// don't load the file if we aren't going to look at it.
	if (m_MapMarkings.Get()[0] == '\0')
		return vec2_invalid;

	KeyValues *pMission = new KeyValues(STRING(gpGlobals->mapname));

	char tempfile[MAX_PATH];
	Q_snprintf(tempfile, sizeof(tempfile), "resource/overviews/%s.txt", STRING(gpGlobals->mapname));
	if (!pMission->LoadFromFile(filesystem, tempfile, "GAME"))
		return vec2_invalid;

	int offset_x = pMission->GetInt("pos_x");
	int offset_y = pMission->GetInt("pos_y");
	float scale = pMission->GetFloat("scale", 1.0f);

	// the Alien Swarm devs put this ugly hack in, so I guess now I have to as well.
	offset_x += 128 * scale;

	int pos = FindNextToken(m_MapMarkings.Get());
	if (!Q_stricmp(szTokenBuffer, "BRACKETS"))
	{
		Vector2D min, max;
		pos += FindNextToken(m_MapMarkings.Get() + pos);
		min.x = atoi(szTokenBuffer);
		pos += FindNextToken(m_MapMarkings.Get() + pos);
		min.y = atoi(szTokenBuffer);
		pos += FindNextToken(m_MapMarkings.Get() + pos);
		max.x = min.x + atoi(szTokenBuffer);
		pos += FindNextToken(m_MapMarkings.Get() + pos);
		max.y = min.y + atoi(szTokenBuffer);


		// now we un-offset the coordinates.
		min.x = (min.x * scale) + offset_x;
		min.y = (min.y * -scale) + offset_y;
		max.x = (max.x * scale) + offset_x;
		max.y = (max.y * -scale) + offset_y;

		return Vector2D(RandomFloat(min.x, max.x), RandomFloat(min.y, max.y));
	}
	return vec2_invalid;
}

// returns the next word from a string (copied from C_ASW_Objective)
int CASW_Objective::FindNextToken(const char* pText)
{
	int iTextLength = V_strlen(pText);
	for (int i = 0; i < iTextLength; i++)
	{
		if (pText[i] == ' ' || pText[i] == '\0')
		{
			szTokenBuffer[i] = '\0';
			//if (pText[i] != '\0')
			//Msg("FindNextToken: Tokenbuffer = %s (ptext: %s)\n", szTokenBuffer, pText + i+1);
			return i + 1;
		}
		szTokenBuffer[i] = pText[i];
	}
	szTokenBuffer[iTextLength] = '\0';
	//Msg("FindNextToken end of string. Tokenbuffer = %s\n", szTokenBuffer);
	return iTextLength + 1;
}