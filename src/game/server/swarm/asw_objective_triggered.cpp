#include "cbase.h"
#include "asw_objective_triggered.h"
#include "asw_computer_area.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// An objective controlled by mapper inputs

LINK_ENTITY_TO_CLASS( asw_objective_triggered, CASW_Objective_Triggered );

BEGIN_DATADESC( CASW_Objective_Triggered )
	DEFINE_INPUTFUNC( FIELD_VOID, "SetComplete", InputSetComplete ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetIncomplete", InputSetIncomplete ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetFailed", InputSetFailed ),
END_DATADESC()


CASW_Objective_Triggered::CASW_Objective_Triggered() : CASW_Objective()
{
	m_hTrigger = NULL;
	m_flTriggerValid = -1;
}


CASW_Objective_Triggered::~CASW_Objective_Triggered()
{
}

void CASW_Objective_Triggered::InputSetComplete( inputdata_t &inputdata )
{
	SetComplete(true);
}

void CASW_Objective_Triggered::InputSetIncomplete( inputdata_t &inputdata )
{
	SetComplete(false);
	SetFailed(false);
}

void CASW_Objective_Triggered::InputSetFailed( inputdata_t &inputdata )
{
	SetFailed(true);
}

CBaseEntity *CASW_Objective_Triggered::FindTriggerEnt()
{
	if (m_flTriggerValid >= gpGlobals->curtime)
		return m_hTrigger;

	m_flTriggerValid = gpGlobals->curtime + 15;

	CBaseEntity *pEnt = NULL;
	while ((pEnt = gEntList.FindEntityByClassname(pEnt, "trigger_asw_computer_area")) != NULL)
	{
		CASW_Computer_Area *pArea = assert_cast<CASW_Computer_Area *>(pEnt);
		const char *pszObjective = pArea->m_DownloadObjectiveName.Get();
		if (pszObjective && !V_stricmp(pszObjective, GetEntityNameAsCStr()))
		{
			m_hTrigger = pEnt;
			return pEnt;
		}
	}

	pEnt = NULL;
	while ((pEnt = gEntList.FindEntityByOutputTarget(pEnt, GetEntityName())) != NULL)
	{
		datamap_t *dmap = pEnt->GetDataDescMap();
		while (dmap)
		{
			int fields = dmap->dataNumFields;
			for (int i = 0; i < fields; i++)
			{
				typedescription_t *dataDesc = &dmap->dataDesc[i];
				if ((dataDesc->fieldType == FIELD_CUSTOM) && (dataDesc->flags & FTYPEDESC_OUTPUT))
				{
					CBaseEntityOutput *pOutput = (CBaseEntityOutput *) ((int) pEnt + (int) dataDesc->fieldOffset);

					for (CEventAction *ev = pOutput->GetFirstAction(); ev; ev = ev->m_pNext)
					{
						if (ev->m_iTarget == GetEntityName() && !V_stricmp(STRING(ev->m_iTargetInput), "SetComplete"))
						{
							m_hTrigger = pEnt;
							return pEnt;
						}
					}
				}
			}

			dmap = dmap->baseMap;
		}
	}

	m_hTrigger = NULL;
	return NULL;
}