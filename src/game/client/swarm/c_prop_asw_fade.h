#ifndef _INCLUDED_C_PROP_ASW_FADE_H
#define _INCLUDED_C_PROP_ASW_FADE_H
#ifdef _WIN32
#pragma once
#endif

#include "c_props.h"

class C_Prop_ASW_Fade : public C_DynamicProp
{
public:
	DECLARE_CLASS(C_Prop_ASW_Fade, C_DynamicProp);
	DECLARE_CLIENTCLASS();

	virtual void OnDataChanged(DataUpdateType_t updateType);
	virtual void ClientThink();

protected:
	byte m_nFadeOpacity;
	Vector m_vecFadeOrigin;
};

#endif	// _INCLUDED_C_PROP_ASW_FADE_H
