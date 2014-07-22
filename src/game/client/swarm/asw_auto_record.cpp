#include "cbase.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar asw_auto_record("asw_auto_record", "0", FCVAR_ARCHIVE, "Maximum number of game recordings to keep. -1 for unlimited.", true, -1, false, 0);

class CASW_Auto_Record : public CAutoGameSystem
{
	typedef CAutoGameSystem BaseClass;
public:
	CASW_Auto_Record() : BaseClass("CASW_Auto_Record")
	{
		m_bIsRecording = false;
	}

	virtual void LevelInitPreEntity()
	{
		BaseClass::LevelInitPreEntity();

		if (engine->IsPlayingDemo())
			return;

		if (engine->IsRecordingDemo())
			return;

		if (asw_auto_record.GetBool())
		{
			if (asw_auto_record.GetInt() > 0)
			{
				DeleteOldRecordings(asw_auto_record.GetInt() - 1);
			}

			struct tm now;
			Plat_GetLocalTime(&now);
			// we assume a map will only be started once per second, and that if a map is started multiple times per second, the old recordings aren't useful.
			// our date format is RFC2550 compliant until January 1st 10000.
			engine->ClientCmd_Unrestricted(VarArgs("record \"aswauto-%04d%02d%02d%02d%02d%02d_%s\"", now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec, MapName()));
			m_bIsRecording = true;
		}
	}

	virtual void LevelShutdownPostEntity()
	{
		BaseClass::LevelShutdownPostEntity();

		if (!m_bIsRecording)
			return;

		engine->ClientCmd_Unrestricted("stop");
		m_bIsRecording = false;
	}

	virtual void DeleteOldRecordings(int nMax)
	{
		// this is O(n^2) with the number of demos, but I'm assuming people won't accumulate a million demos and then change the maximum to 1 so it'll be closer to O(n).

		while (true)
		{
			int nCount = 0;
			char szOldest[MAX_PATH];
			szOldest[0] = '\0';

			FileFindHandle_t ffh;
			for (const char *pszFile = filesystem->FindFirstEx("aswauto-*.dem", "MOD", &ffh); pszFile; pszFile = filesystem->FindNext(ffh))
			{
				nCount++;
				if (szOldest[0] == '\0' || V_strcmp(szOldest, pszFile) > 0)
				{
					V_strncpy(szOldest, pszFile, MAX_PATH);
				}
			}
			filesystem->FindClose(ffh);

			if (nCount == 0)
				return;
			if (nCount <= nMax)
				return;

			Msg("Deleting oldest recording: %s\n", szOldest);
			filesystem->RemoveFile(szOldest, "MOD");
			if (filesystem->FileExists(szOldest, "MOD"))
			{
				Warning("Deletion failed!\n");
				return;
			}
			Msg("%d recordings remaining.\n", nCount - 1);
		}
	}

	bool m_bIsRecording;
};

static CASW_Auto_Record g_ASWAutoRecord;
