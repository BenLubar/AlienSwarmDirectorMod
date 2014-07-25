#include "Room.h"
#include "ChunkFile.h"
#include "MapLayout.h"
#include "RoomTemplate.h"
#include "LevelTheme.h"
#include "KeyValues.h"

#include <vgui/IVGui.h>
#include "vgui_controls/Controls.h"
#include <vgui_controls/Panel.h>
#include "TileGenDialog.h"
#include "layout_system\tilegen_ranges.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

CRoom::CRoom() : m_iszLevelTheme(), m_iszRoomTemplate()
{
	m_iPosX = 0;
	m_iPosY = 0;
	m_iNumChildren = 0;
	m_nPlacementIndex = -1;
	m_bHasAlienEncounter = false;
	m_nInstanceSeed = -1;

	m_pPlacedRoomPanel = NULL;
	m_pMapLayout = NULL;
}

CRoom::CRoom(CMapLayout *pMapLayout, std::string iszLevelTheme, std::string iszRoomTemplate, int TileX, int TileY, int nInstanceSeed) : m_iszLevelTheme(iszLevelTheme), m_iszRoomTemplate(iszRoomTemplate)
{
	m_iPosX = TileX;
	m_iPosY = TileY;
	m_iNumChildren = 0;
	m_bHasAlienEncounter = false;
	m_nInstanceSeed = nInstanceSeed;
	
	// Add ourself to the list of placed rooms.
	// This also sets m_nPlacementIndex.
	pMapLayout->PlaceRoom( this );

	m_pPlacedRoomPanel = NULL;
}

CRoom::~CRoom()
{
	if ( g_pTileGenDialog )
	{
		g_pTileGenDialog->m_SelectedRooms.FindAndRemove( this );
	}
	if ( m_pMapLayout )
	{
		// remove it from the list of placed rooms
		m_pMapLayout->RemoveRoom( this );
	}

	if (m_pPlacedRoomPanel)
	{
		m_pPlacedRoomPanel->MarkForDeletion();
		m_pPlacedRoomPanel = NULL;
	}
}

KeyValues *CRoom::GetKeyValuesCopy()
{
	KeyValues *pKeys = new KeyValues( "room" );
	pKeys->SetInt( "posx", m_iPosX );
	pKeys->SetInt( "posy", m_iPosY );
	pKeys->SetInt( "instance_seed", m_nInstanceSeed );
	pKeys->SetString( "theme", m_iszLevelTheme.c_str() );
	pKeys->SetString( "template", m_iszRoomTemplate.c_str() );

	return pKeys;
}

bool CRoom::SaveRoomToFile(CChunkFile *pFile)
{
	if (!pFile)
		return false;

	ChunkFileResult_t eResult = pFile->BeginChunk("room");
	if (eResult != ChunkFile_Ok)
		return false;

	// write out each property of the room
	if (pFile->WriteKeyValueInt("posx", m_iPosX) != ChunkFile_Ok)
		return false;
	if (pFile->WriteKeyValueInt("posy", m_iPosY) != ChunkFile_Ok)
		return false;
	if (pFile->WriteKeyValueInt("instance_seed", m_nInstanceSeed) != ChunkFile_Ok)
		return false;

	// theme and template name
	if (pFile->WriteKeyValue("theme", m_iszLevelTheme.c_str()) != ChunkFile_Ok)
		return false;
	if (pFile->WriteKeyValue("template", m_iszRoomTemplate.c_str()) != ChunkFile_Ok)
		return false;

	if (pFile->EndChunk() != ChunkFile_Ok)
		return false;

	return true;
}

// creates a new CRoom and reads in its properties from the file
bool CRoom::LoadRoomFromKeyValues( KeyValues *pRoomKeys, CMapLayout *pMapLayout )
{	
	CRoom* pNewRoom = new CRoom();
	
	pNewRoom->m_iPosX = pRoomKeys->GetInt( "posx" );
	pNewRoom->m_iPosY = pRoomKeys->GetInt( "posy" );
	pNewRoom->m_nInstanceSeed = pRoomKeys->GetInt( "instance_seed" );

	pNewRoom->m_iszLevelTheme = pRoomKeys->GetString( "theme" );
	pNewRoom->m_iszRoomTemplate = pRoomKeys->GetString( "template" );

	if (!pNewRoom->GetRoomTemplate())
	{
		Msg( "Failed to load room template %s in theme %s\n", pRoomKeys->GetString( "theme" ), pRoomKeys->GetString( "template" ) );
		delete pNewRoom;
		return false;
	}

	// add it to the list of placed rooms
	pMapLayout->PlaceRoom( pNewRoom );

	return true;
}

CLevelTheme *CRoom::GetLevelTheme() const
{
	return CLevelTheme::FindTheme(m_iszLevelTheme.c_str());
}

CRoomTemplate *CRoom::GetRoomTemplate() const
{
	CLevelTheme *pLevelTheme = GetLevelTheme();
	return pLevelTheme ? pLevelTheme->FindRoom(m_iszRoomTemplate.c_str()) : NULL;
}

// =================================
// IASW_Room_Details interface
// =================================

// tags
bool CRoom::HasTag( const char *szTag )
{
	const CRoomTemplate *pRoomTemplate = GetRoomTemplate();
	if ( !pRoomTemplate )
		return false;

	return pRoomTemplate->HasTag( szTag );
}

int CRoom::GetNumTags()
{
	const CRoomTemplate *pRoomTemplate = GetRoomTemplate();
	if (!pRoomTemplate)
		return 0;

	return pRoomTemplate->GetNumTags();
}

const char* CRoom::GetTag( int i )
{
	const CRoomTemplate *pRoomTemplate = GetRoomTemplate();
	if (!pRoomTemplate)
		return "";

	return pRoomTemplate->GetTag( i );
}

int CRoom::GetSpawnWeight()
{
	const CRoomTemplate *pRoomTemplate = GetRoomTemplate();
	if (!pRoomTemplate)
		return -1;

	return pRoomTemplate->GetSpawnWeight();
}

bool CRoom::GetThumbnailName( char* szOut, int iBufferSize )
{
	Q_snprintf( szOut, iBufferSize, "tilegen/roomtemplates/%s/%s.tga", m_iszLevelTheme.c_str(), m_iszRoomTemplate.c_str() );
	return true;
}

bool CRoom::GetFullRoomName( char* szOut, int iBufferSize )
{
	Q_snprintf( szOut, iBufferSize, "%s\\%s", m_iszLevelTheme.c_str(), m_iszRoomTemplate.c_str() );
	return true;
}

void CRoom::GetSoundscape( char* szOut, int iBufferSize )
{
	const CRoomTemplate *pRoomTemplate = GetRoomTemplate();
	if (!pRoomTemplate)
	{
		szOut[0] = '\0';
	}
	else
	{
		Q_snprintf(szOut, iBufferSize, "%s", pRoomTemplate->GetSoundscape());
	}
}

void CRoom::GetTheme( char* szOut, int iBufferSize )
{
	Q_strncpy(szOut, m_iszLevelTheme.c_str(), iBufferSize);
}

const Vector& CRoom::GetAmbientLight()
{
	const CLevelTheme *pLevelTheme = GetLevelTheme();
	return pLevelTheme ? pLevelTheme->m_vecAmbientLight : vec3_origin;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CRoom::GetTileType()
{
	const CRoomTemplate *pRoomTemplate = GetRoomTemplate();
	if ( !pRoomTemplate )
		return -1;

	return pRoomTemplate->GetTileType();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CRoom::GetTileTypeName( int nType )
{
	// This is the default, I didn't find anything value for GetTileType
	if ( nType == -1 )
		return NULL;

	Assert( nType >= ASW_TILETYPE_UNKNOWN && nType < ASW_TILETYPE_COUNT );
	return g_szASWTileTypeStrings[nType];
}


void CRoom::GetWorldBounds( Vector *vecWorldMins, Vector *vecWorldMaxs )
{
	const CRoomTemplate *pRoomTemplate = GetRoomTemplate();
	if ( !pRoomTemplate )
	{
		*vecWorldMins = vec3_origin;
		*vecWorldMaxs = vec3_origin;
		return;
	}

	int half_map_size = MAP_LAYOUT_TILES_WIDE * 0.5f;		// shift back so the middle of our grid is the origin
	int xOffset = ( m_iPosX - half_map_size ) * ASW_TILE_SIZE;
	int yOffset = ( m_iPosY - half_map_size ) * ASW_TILE_SIZE;

	vecWorldMins->x = xOffset;
	vecWorldMins->y = yOffset;
	vecWorldMins->z = 0;

	vecWorldMaxs->x = xOffset + pRoomTemplate->GetTilesX() * ASW_TILE_SIZE;
	vecWorldMaxs->y = yOffset + pRoomTemplate->GetTilesY() * ASW_TILE_SIZE;
	vecWorldMaxs->z = 0;
}

const Vector& CRoom::WorldSpaceCenter()
{
	Vector mins = vec3_origin;
	Vector maxs = vec3_origin;
	GetWorldBounds( &mins, &maxs );

	Vector &vecResult = AllocTempVector();
	vecResult = ( mins + maxs ) / 2.0f;
	return vecResult;
}

int CRoom::GetNumExits()
{
	const CRoomTemplate *pRoomTemplate = GetRoomTemplate();
	if ( !pRoomTemplate )
		return 0;

	return pRoomTemplate->m_Exits.Count();	
}

IASW_Room_Details *CRoom::GetAdjacentRoom( int nExit )
{
	const CRoomTemplate *pRoomTemplate = GetRoomTemplate();
	int nExitX, nExitY;
	if ( GetExitPosition( pRoomTemplate, m_iPosX, m_iPosY, nExit, &nExitX, &nExitY ) )
	{
		return m_pMapLayout->GetRoom( nExitX, nExitY );
	}

	return NULL;
}