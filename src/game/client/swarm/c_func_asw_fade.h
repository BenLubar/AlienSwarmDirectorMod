#ifndef _INCLUDED_C_FUNC_ASW_FADE_H
#define _INCLUDED_C_FUNC_ASW_FADE_H
#ifdef _WIN32
#pragma once
#endif

#include "c_func_brush.h"
#include "c_asw_marine.h"

class C_Func_ASW_Fade : public C_FuncBrush
{
public:
	DECLARE_CLASS(C_Func_ASW_Fade, C_FuncBrush);
	DECLARE_CLIENTCLASS();

	C_Func_ASW_Fade();

	virtual void OnDataChanged(DataUpdateType_t updateType);
	virtual void ClientThink();

protected:
	bool m_bLastControls;
	CHandle<C_ASW_Marine> m_hLastMarine;
	bool m_bFaded;
	float m_flInterpStart;
	byte m_nNormalOpacity;
	byte m_nFadeOpacity;
};

#endif	// _INCLUDED_C_FUNC_ASW_FADE_H
