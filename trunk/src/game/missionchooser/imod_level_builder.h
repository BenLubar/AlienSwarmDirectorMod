#ifndef IMOD_LEVEL_BUILDER_H
#define IMOD_LEVEL_BUILDER_H

#if defined( COMPILER_MSVC )
#pragma once
#endif

class IMOD_Level_Builder
{
public:
	virtual bool IsBuildingLevel() = 0;
	virtual void BuildLevel( const char *szMissionFile, const int iDifficultLevel, const char *szThemeName, const char * szOutputFile) = 0;
	virtual void BuildLevel( const char *szMissionFile, const int iDifficultLevel, const char * szOutputFile) = 0;
	virtual void CompileAndExecuteLevel(const char * szLayoutFile) = 0;
};


#endif //IMOD_LEVEL_BUILDER_H