#ifndef _INCLUDED_NB_LOBBY_ROW_XSMALL_H
#define _INCLUDED_NB_LOBBY_ROW_XSMALL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include "vgui_controls/ImagePanel.h"
#include "nb_lobby_row_small.h"

// == MANAGED_CLASS_DECLARATIONS_START: Do not edit by hand ==
class vgui::ImagePanel;
class vgui::Panel;
// == MANAGED_CLASS_DECLARATIONS_END ==

class CNB_Lobby_Row_XSmall : public CNB_Lobby_Row_Small
{
	DECLARE_CLASS_SIMPLE(CNB_Lobby_Row_XSmall, CNB_Lobby_Row_Small);
public:
	CNB_Lobby_Row_XSmall(vgui::Panel *parent, const char *name);
	virtual ~CNB_Lobby_Row_XSmall();

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
};

#endif // _INCLUDED_NB_LOBBY_ROW_XSMALL_H


