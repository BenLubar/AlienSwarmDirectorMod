#include "cbase.h"
#include "c_prop_asw_fade.h"
#include "c_asw_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_Prop_ASW_Fade, DT_Prop_ASW_Fade, CProp_ASW_Fade)
	RecvPropInt(RECVINFO(m_nFadeOpacity)),
	RecvPropVector(RECVINFO(m_vecFadeOrigin)),
END_RECV_TABLE()

extern ConVar asw_fade_duration;
extern ConVar asw_controls;

C_Prop_ASW_Fade::C_Prop_ASW_Fade()
{
	m_flInterpStart = 0;
	m_iLastControls = 1;
	m_hLastMarine = NULL;
}

void C_Prop_ASW_Fade::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	if (updateType == DATA_UPDATE_CREATED)
	{
		SetNextClientThink(CLIENT_THINK_ALWAYS);
		if (GetRenderMode() == kRenderNormal)
		{
			SetRenderMode(kRenderTransTexture);
		}
		m_bFaded = false;
		m_nNormalOpacity = GetRenderAlpha();
	}
}

void C_Prop_ASW_Fade::ClientThink()
{
	BaseClass::ClientThink();

	C_ASW_Player *pPlayer = C_ASW_Player::GetLocalASWPlayer();
	if (!pPlayer)
	{
		return;
	}

	C_ASW_Marine *pMarine = pPlayer->GetViewMarine();

	Assert(asw_controls.GetInt() >= 0 && asw_controls.GetInt() <= 2);
	bool bFade = asw_controls.GetInt() == 1 && pMarine && pMarine->GetAbsOrigin().z <= m_vecFadeOrigin.z;
	byte target = bFade ? m_nFadeOpacity : m_nNormalOpacity;
	byte prev = bFade ? m_nNormalOpacity : m_nFadeOpacity;
	if (bFade != m_bFaded)
	{
		m_bFaded = bFade;
		m_flInterpStart = gpGlobals->curtime - fabs((m_nFadeOpacity != m_nNormalOpacity) ? asw_fade_duration.GetFloat() * (GetRenderAlpha() - prev) / (m_nFadeOpacity - m_nNormalOpacity) : asw_fade_duration.GetFloat());
		m_flInterpStart = MAX(0, m_flInterpStart);
	}

	if (asw_controls.GetInt() != m_iLastControls || pMarine != m_hLastMarine.Get())
	{
		m_iLastControls = asw_controls.GetInt();
		m_hLastMarine = pMarine;
		m_flInterpStart = 0;
		SetRenderAlpha(target);
		return;
	}

	if (m_flInterpStart + asw_fade_duration.GetFloat() <= gpGlobals->curtime)
	{
		SetRenderAlpha(target);
	}
	else if (m_flInterpStart > 0)
	{
		SetRenderAlpha(Lerp((gpGlobals->curtime - m_flInterpStart) / asw_fade_duration.GetFloat(), prev, target));
	}
}
