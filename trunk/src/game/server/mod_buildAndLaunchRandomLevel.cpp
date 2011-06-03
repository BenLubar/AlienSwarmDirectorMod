#include "cbase.h"
#include "mod_buildAndLaunchRandomLevel.h"
#include "convar.h"
#include "ienginevgui.h"

 void MyFunction_f( void )
 {
     Msg("PJ: This is my function\n");
 }
 
 ConCommand my_function( "my_function", MyFunction_f, "Shows a message.", FCVAR_CHEAT );

 
void MOD_BuildAndLaunchRandomLevel(const CCommand &args)
{
	Msg("PJ: Hello World\n");
}

static ConCommand MOD_BuildAndLaunchRandomLevel2("mod_BuildAndLaunchRandomLevel", MOD_BuildAndLaunchRandomLevel, "PJ - First Server Command.", FCVAR_CHEAT );

