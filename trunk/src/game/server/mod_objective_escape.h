#ifndef _INCLUDED_MOD_OBJECTIVE_ESCAPE_H
#define _INCLUDED_MOD_OBJECTIVE_ESCAPE_H

#include "asw_objective_escape.h"

class CMOD_Objective_Escape : public CASW_Objective_Escape
{
public:
	DECLARE_CLASS( CMOD_Objective_Escape, CASW_Objective_Escape );
	DECLARE_DATADESC();

	CMOD_Objective_Escape();
	~CMOD_Objective_Escape();
	 
	void CheckEscapeStatus();
	bool OtherObjectivesComplete();
	bool AllLiveMarinesInExit();
	void InputMarineInEscapeArea( inputdata_t &inputdata );

	CBaseTrigger* GetTrigger();
	EHANDLE m_hTrigger;	
};


#endif /* _INCLUDED_MOD_OBJECTIVE_ESCAPE_H */