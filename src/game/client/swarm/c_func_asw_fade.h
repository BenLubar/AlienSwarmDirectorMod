#ifndef _INCLUDED_C_FUNC_ASW_FADE_H
#define _INCLUDED_C_FUNC_ASW_FADE_H
#ifdef _WIN32
#pragma once
#endif

#include "c_func_brush.h"

class C_Func_ASW_Fade : public C_FuncBrush
{
public:
	DECLARE_CLASS(C_Func_ASW_Fade, C_FuncBrush);
	DECLARE_CLIENTCLASS();

	virtual void OnDataChanged(DataUpdateType_t updateType);
	virtual void ClientThink();

protected:
	byte m_nFadeOpacity;
};

#endif	// _INCLUDED_C_FUNC_ASW_FADE_H
