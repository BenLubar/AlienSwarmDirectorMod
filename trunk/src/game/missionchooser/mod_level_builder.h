#ifndef _INCLUDED_MOD_LEVEL_BUILDER_H
#define _INCLUDED_MOD_LEVEL_BUILDER_H
#ifdef _WIN32
#pragma once
#endif

#include "imod_level_builder.h"

//#include "layout_system\tilegen_layout_system.h"
//#include "vgui\tilegen_pages.h"

class MOD_Level_Builder : public IMOD_Level_Builder
{
	//static MOD_Level_Builder* g_LevelBuilderSingleton;

	bool g_IsBuildingLevel;
	void SetIsBuildingLevel(bool value);
	
public:
	//static MOD_Level_Builder* LevelBuilder();
	MOD_Level_Builder();
	~MOD_Level_Builder();

	virtual bool IsBuildingLevel();
	virtual void BuildLevel( const char *szMissionFile, const int iDifficultLevel, const char *szThemeName, const char * szOutputFile);
	virtual void BuildLevel( const char *szMissionFile, const int iDifficultLevel, const char * szOutputFile);
	virtual void CompileAndExecuteLevel(const char * szLayoutFile);
	
};


#endif //_INCLUDED_MOD_LEVEL_BUILDER_H