
#include "cbase.h"

#include "mod_buildAndLaunchRandomLevel.h"
#include "convar.h"
#include <vgui_controls/Frame.h>
#include "ienginevgui.h"

#include "asw_build_map_frame.h" //- not having causes a problem in imaterial.h

#include "missionchooser/iasw_random_missions.h" // - required

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

 
void MOD_BuildAndLaunchRandomLevel(const CCommand &args)
{
	Msg("PJ: Hello World\n");
	missionchooser->RandomMissions()->BuildAndLaunchRandomLevel();
}

static ConCommand MOD_BuildAndLaunchRandomLevel2("mod_BuildAndLaunchRandomLevel", MOD_BuildAndLaunchRandomLevel, "PJ - First Server Command.", FCVAR_CHEAT );

void MOD_LaunchTileGen(const CCommand &args)
{
	Msg("PJ: Hello World\n");
	vgui::VPANEL GameUIRoot = enginevgui->GetPanel( PANEL_GAMEUIDLL );	
	vgui::Frame *pFrame = dynamic_cast<vgui::Frame*>( missionchooser->RandomMissions()->CreateTileGenFrame( NULL ) );

	pFrame->SetParent( GameUIRoot );
	pFrame->MoveToCenterOfScreen();
	pFrame->Activate();	
}

static ConCommand MOD_BuildAndLaunchRandomLevel3("mod_LaunchTileGen", MOD_LaunchTileGen, "Start Tile Gen.", FCVAR_CHEAT );

