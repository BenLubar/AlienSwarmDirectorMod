#include "cbase.h"
#include "nb_lobby_row_xsmall.h"
#include "asw_briefing.h"
#include <vgui/IVgui.h>
#include "vgui_controls/ImagePanel.h"

#include "vgui_controls/Panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CNB_Lobby_Row_XSmall::CNB_Lobby_Row_XSmall(vgui::Panel *parent, const char *name) : BaseClass(parent, name)
{
}

CNB_Lobby_Row_XSmall::~CNB_Lobby_Row_XSmall()
{
}

void CNB_Lobby_Row_XSmall::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::BaseClass::BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("resource/ui/nb_lobby_row_xsmall.res");

	for (int i = 0; i < ASW_NUM_INVENTORY_SLOTS; i++)
	{
		m_szLastWeaponImage[i][0] = 0;
	}
	m_szLastPortraitImage[0] = 0;
	m_lastSteamID.Set(0, k_EUniverseInvalid, k_EAccountTypeInvalid);
}