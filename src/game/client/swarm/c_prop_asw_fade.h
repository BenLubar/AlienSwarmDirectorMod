#ifndef _INCLUDED_C_PROP_ASW_FADE_H
#define _INCLUDED_C_PROP_ASW_FADE_H
#ifdef _WIN32
#pragma once
#endif

#include "c_props.h"
#include "c_asw_marine.h"

class C_Prop_ASW_Fade : public C_DynamicProp
{
public:
	DECLARE_CLASS(C_Prop_ASW_Fade, C_DynamicProp);
	DECLARE_CLIENTCLASS();

	C_Prop_ASW_Fade();

	virtual void OnDataChanged(DataUpdateType_t updateType);
	virtual void ClientThink();

	Vector m_vecFadeOrigin;

protected:
	int m_iLastControls;
	CHandle<C_ASW_Marine> m_hLastMarine;
	bool m_bFaded;
	float m_flInterpStart;
	byte m_nNormalOpacity;
	byte m_nFadeOpacity;
};

#endif	// _INCLUDED_C_PROP_ASW_FADE_H
