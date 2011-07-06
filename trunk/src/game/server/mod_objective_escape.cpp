#include "cbase.h"
#include "mod_objective_escape.h"
#include "asw_game_resource.h"
#include "asw_marine_resource.h"
#include "asw_marine.h"
#include "triggers.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( mod_objective_escape, CMOD_Objective_Escape );

BEGIN_DATADESC( CMOD_Objective_Escape )
	DEFINE_FIELD( m_hTrigger, FIELD_EHANDLE ),
	DEFINE_INPUTFUNC( FIELD_VOID, "MarineInEscapeArea", InputMarineInEscapeArea ),
END_DATADESC()


CMOD_Objective_Escape::CMOD_Objective_Escape() : CASW_Objective_Escape()
{	
	
}


CMOD_Objective_Escape::~CMOD_Objective_Escape()
{	
	g_aEscapeObjectives.FindAndRemove( this );
}

void CMOD_Objective_Escape::CheckEscapeStatus()
{
	BaseClass::CheckEscapeStatus();
	
}

bool CMOD_Objective_Escape::OtherObjectivesComplete(){
	return BaseClass::OtherObjectivesComplete();
}
bool CMOD_Objective_Escape::AllLiveMarinesInExit()
{
	return BaseClass::AllLiveMarinesInExit();
}
void CMOD_Objective_Escape::InputMarineInEscapeArea( inputdata_t &inputdata ){
	BaseClass::InputMarineInEscapeArea(inputdata);
}

CBaseTrigger* CMOD_Objective_Escape::GetTrigger()
{
	return BaseClass::GetTrigger();
}