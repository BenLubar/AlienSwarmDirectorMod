#include "cbase.h"
#include "c_func_asw_fade.h"
#include "c_asw_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_Func_ASW_Fade, DT_Func_ASW_Fade, CFunc_ASW_Fade)
	RecvPropInt(RECVINFO(m_nFadeOpacity)),
END_RECV_TABLE()

extern ConVar asw_controls;

C_Func_ASW_Fade::C_Func_ASW_Fade()
{
	m_bLastControls = true;
	m_hLastMarine = NULL;
}

void C_Func_ASW_Fade::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	if (updateType == DATA_UPDATE_CREATED)
	{
		SetNextClientThink(CLIENT_THINK_ALWAYS);
		if (GetRenderMode() == kRenderNormal)
		{
			SetRenderMode(kRenderTransTexture);
		}
	}
}


void C_Func_ASW_Fade::ClientThink()
{
	BaseClass::ClientThink();

	C_ASW_Player *pPlayer = C_ASW_Player::GetLocalASWPlayer();
	if (!pPlayer)
	{
		return;
	}

	C_ASW_Marine *pMarine = pPlayer->GetSpectatingMarine();
	if (!pMarine)
	{
		pMarine = pPlayer->GetMarine();
	}
	if (!pMarine)
	{
		m_hLastMarine = NULL;
		return;
	}

	byte target = m_nFadeOpacity;
	if (!asw_controls.GetBool() || pMarine->GetAbsOrigin().z > GetAbsOrigin().z)
	{
		target = 255;
	}

	if (asw_controls.GetBool() != m_bLastControls || pMarine != m_hLastMarine.Get())
	{
		m_bLastControls = asw_controls.GetBool();
		m_hLastMarine = pMarine;
		SetRenderAlpha(target);
		return;
	}

	byte current = GetRenderAlpha();
	if (current > target)
	{
		SetRenderAlpha(current - 1);
	}
	else if (current < target)
	{
		SetRenderAlpha(current + 1);
	}
}
