//#include "cbase.h"
#include "mod_level_builder.h"

#include "layout_system\tilegen_layout_system.h"
#include "layout_system\tilegen_listeners.h"
#include "layout_system/tilegen_mission_preprocessor.h"
#include "vgui\tilegen_pages.h"
#include <KeyValues.h>
#include "filesystem.h"
#include "TileSource/LevelTheme.h"
#include "TileSource/MapLayout.h"
#include "asw_map_builder.h"
#include "asw_key_values_database.h"
#include "cdll_int.h"  //needed for access to engine
#include "strtools.h"
#include "asw_mission_chooser.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DEFAULT_THEME "mod"

//I'm not sure what this is, but it is hardcoded
//in the example files I looked at (TileGenDialog.cpp)
#define GAME_DIRECTORY "GAME"

CLayoutSystem *g_pLayoutSystem;
CTilegenLayoutSystemPage *g_pLayoutSystemPage;
CMapLayout *g_pMapLayout;

extern IVEngineClient *engine;

//MOD_Level_Builder* MOD_Level_Builder::g_LevelBuilderSingleton;

MOD_Level_Builder::MOD_Level_Builder(){
	g_pLayoutSystem = NULL;
	g_pLayoutSystemPage = NULL;
	g_pMapLayout = NULL;

	SetIsBuildingLevel(false);
}

MOD_Level_Builder::~MOD_Level_Builder()
{
	delete g_pLayoutSystem;
	delete g_pLayoutSystemPage;
	delete g_pMapLayout;
}

/*Singleton moved to asw_mission_chooser.cpp (exposed through iasw_mission_chooser)
MOD_Level_Builder* MOD_Level_Builder::LevelBuilder()
{	
	if (!g_LevelBuilderSingleton)
	{
		g_LevelBuilderSingleton = new MOD_Level_Builder();
	}

	return g_LevelBuilderSingleton;
}
*/

bool MOD_Level_Builder::IsBuildingLevel() 
{
	return g_IsBuildingLevel;
}

void MOD_Level_Builder::SetIsBuildingLevel(bool value)
{
	g_IsBuildingLevel = value;
}

void MOD_Level_Builder::BuildLevel( const char *szMissionFile, const int iDifficultLevel, const char * szOutputFile)
{
	BuildLevel(szMissionFile, iDifficultLevel, DEFAULT_THEME, szOutputFile);
}

//Logic from CASW_Random_Missions::BuildAndLaunchRandomLevel
void MOD_Level_Builder::BuildLevel( const char *szMissionFile, const int iDifficultLevel, const char *szThemeName, const char * szOutputFile)
{	
	if (IsBuildingLevel())
	{
		Msg("mod_level_builder is already building a level!");
		return;
	}

	/*Code taken from TileGenDialog.cpp*/
	KeyValues *pGenerationOptions = new KeyValues("EmptyOptions");
	pGenerationOptions->Clear();
	pGenerationOptions->LoadFromFile(g_pFullFileSystem, szMissionFile, GAME_DIRECTORY);	

	CLayoutSystem *pLayoutSystem = new CLayoutSystem();

	CTilegenMissionPreprocessor *pMissionPreprocessor = new CTilegenMissionPreprocessor();

	CASW_KeyValuesDatabase *pRulesDatabase = new CASW_KeyValuesDatabase();
	pRulesDatabase->LoadFiles( "tilegen/rules/" );
	for ( int i = 0; i < pRulesDatabase->GetFileCount(); ++ i )
	{
		pMissionPreprocessor->ParseAndStripRules( pRulesDatabase->GetFile( i ) );
	}
	
	if (pMissionPreprocessor->SubstituteRules(pGenerationOptions) )
	{
		KeyValues *pMissionSettings = pGenerationOptions->FindKey( "mission_settings" ); 

		if ( pMissionSettings == NULL )
		{
			Warning("Mission is missing a Global Options Block!\n");
			return;
		}

		// Copy the filename key over
		pMissionSettings->SetString( "Filename", pGenerationOptions->GetString( "Filename", "invalid_filename" ) );
					
		AddListeners( pLayoutSystem );
		if ( !pLayoutSystem->LoadFromKeyValues( pGenerationOptions ) )
		{
			Warning("Failed to load mission from pre-processed key-values.");			
			return;
		}

		pLayoutSystem->BeginGeneration(new CMapLayout( pMissionSettings->MakeCopy() ));

		int safetyCounter = 0;
		while (pLayoutSystem->IsGenerating())
		{
			pLayoutSystem->ExecuteIteration();			

			if (safetyCounter++ > 1000)
			{
				Warning("Safety Counter has prevented executing pLayoutSystem->ExecuteIteration() over 1000 times.\n");
				break;
			}
		}
		
		CMapLayout *pMapLayout = pLayoutSystem->GetMapLayout();

		char fullPath[1024];
		strcat(fullPath, g_gamedir);
		strcat(fullPath, szOutputFile);

		Q_strcpy(pMapLayout->m_szFilename, fullPath);	
		pMapLayout->SaveMapLayout( fullPath );			
	}
	else
	{
		Warning("Failed Initializting Mission Preprocessor\n");
		return;
	}		
}

/*
//Logic is based off of CTileGenDialog::GenerateMission (TileGenDialog.cpp - 726)
void MOD_Level_Builder::BuildLevel( const char *szMissionFile, const int iDifficultLevel, const char *szThemeName, const char * szOutputFile)
{	
	if (IsBuildingLevel())
	{
		Msg("mod_level_builder is already building a level!");
		return;
	}

	//Load Theme
	CLevelTheme *pLevelTheme = CLevelTheme::FindTheme(szThemeName);
	if (!pLevelTheme)
	{
		Msg("Failed loading theme!");
		return;
	}

	//Load Mission
	KeyValues *pGenerationOptions = new KeyValues( "EmptyOptions" );
	pGenerationOptions->LoadFromFile(g_pFullFileSystem, szMissionFile, GAME_DIRECTORY);

	
	// Preprocess the mission file (i.e. apply rules)
	CTilegenMissionPreprocessor *p = g_pLayoutSystemPage->GetPreprocessor();
	p->SubstituteRules(pGenerationOptions);
	if (! g_pLayoutSystemPage->GetPreprocessor()->SubstituteRules( pGenerationOptions ) )
	{
		Msg("Failed to pre-process layout system definition.");		
		return;
	}

	KeyValues *pMissionSettings = pGenerationOptions->FindKey( "mission_settings" ); 

	if ( pMissionSettings == NULL )
	{
		Msg("Mission is missing a Global Options block.");			
		return;
	}
	// Copy the filename key over
	pMissionSettings->SetString( "Filename", pGenerationOptions->GetString( "Filename", "invalid_filename" ) );

	delete g_pLayoutSystem;
	g_pLayoutSystem = new CLayoutSystem();
		
	AddListeners( g_pLayoutSystem );
	if ( !g_pLayoutSystem->LoadFromKeyValues( pGenerationOptions ) )
	{
		Msg("Failed to load mission from pre-processed key-values.");			
		return;
	}

	// Clear the current layout		
	delete g_pMapLayout;
	g_pMapLayout = new CMapLayout( pMissionSettings->MakeCopy() );

	// Generate Map Layout
	g_pLayoutSystem->BeginGeneration( g_pMapLayout );

	
	
	
	// Compile Map		
}
*/



void MOD_Level_Builder::CompileAndExecuteLevel(const char * szLayoutFile)
{
	if (engine)
	{
		char buffer[512];
		Q_snprintf(buffer, sizeof(buffer), "asw_spawning_enabled 0;  asw_build_map %s", szLayoutFile );
		Msg("Executing: [%s]", buffer);
		engine->ClientCmd_Unrestricted( buffer );
	}
	else
	{
		Warning("No Engine!!");
		return;
	}		
}