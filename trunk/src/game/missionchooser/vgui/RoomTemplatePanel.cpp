#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include "vgui_controls/Controls.h"
#include <vgui/IScheme.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/MessageBox.h>
#include <KeyValues.h>
#include <vgui_controls/Tooltip.h>
#include "vgui/missionchooser_tgaimagepanel.h"

#include "TileGenDialog.h"
#include "RoomTemplatePanel.h"
#include "RoomTemplateEditDialog.h"
#include "TileSource/RoomTemplate.h"
#include "TileSource/LevelTheme.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

CUtlVector<CRoomTemplatePanel*> g_pRoomTemplatePanels;

CRoomTemplatePanel::CRoomTemplatePanel(Panel *parent, const char *name) : BaseClass(parent, name), m_iszLevelTheme(), m_iszRoomTemplate()
{
	m_hMenu = NULL;

	m_pRoomTGAPanel = new CMissionChooserTGAImagePanel(this, "RoomTGAPanel" );
	m_pRoomTGAPanel->SetMouseInputEnabled(false);

	m_pSelectedOutline = new vgui::Panel(this, "SelectedBorder");
	m_pSelectedOutline->SetVisible(false);
	m_pSelectedOutline->SetMouseInputEnabled(false);
	m_pSelectedOutline->SetZPos(10);	// make sure the border stays above exits/tile squares

	m_iLastTilesX = -1;
	m_iLastTilesY = -1;

	m_bRoomTemplateEditMode = false;
	m_bRoomTemplateBrowserMode = false;

	m_bForceShowExits = false;
	m_bForceShowTileSquares = false;

	m_pTagsLabel = new vgui::Label( this, "TagsLabel", "" );
	m_pTagsLabel->SetMouseInputEnabled( false );
	m_pTagsLabel->SetContentAlignment( vgui::Label::a_northwest );
	m_pTagsLabel->SetZPos( 2 );

	SetMouseInputEnabled(true);

	g_pRoomTemplatePanels.AddToTail(this);
}

CRoomTemplatePanel::~CRoomTemplatePanel( void )
{
	g_pRoomTemplatePanels.FindAndRemove(this);
}

CRoomTemplate *CRoomTemplatePanel::GetRoomTemplate()
{
	CLevelTheme *pLevelTheme = CLevelTheme::FindTheme(m_iszLevelTheme.c_str());
	return pLevelTheme ? pLevelTheme->FindRoom(m_iszRoomTemplate.c_str()) : NULL;
}

void CRoomTemplatePanel::SetRoomTemplate( const CRoomTemplate* pTemplate )
{
	m_iszLevelTheme = pTemplate->m_iszLevelTheme;
	m_iszRoomTemplate = pTemplate->GetFullName();
	UpdateImages();

	if (pTemplate)
	{
		GetTooltip()->SetText( pTemplate->GetFullName() );
		GetTooltip()->SetTooltipFormatToSingleLine();

		char buffer[2048];
		buffer[0] = 0;
		for ( int i = 0; i < pTemplate->GetNumTags(); i++ )
		{
			Q_snprintf( buffer, sizeof( buffer ), "%s\n%s", buffer, pTemplate->GetTag( i ) );
		}		
		m_pTagsLabel->SetText( buffer );
	}
	else
	{
		m_pTagsLabel->SetText( "" );
	}
	//else
		//pLabel->GetTooltip()->SetText( m_pShader->GetParamHelp( i ) );
}

void CRoomTemplatePanel::UpdateImages()
{
	for (int i=0;i<m_pExitImagePanels.Count();i++)
	{
		m_pExitImagePanels[i]->MarkForDeletion();
	}	
	m_pExitImagePanels.RemoveAll();

	CRoomTemplate *pRoomTemplate = GetRoomTemplate();
	if (!pRoomTemplate)
	{
		m_pRoomTGAPanel->SetTGA("");
		return;
	}

	char buffer[MAX_PATH];
	Q_snprintf( buffer, sizeof(buffer), "tilegen/roomtemplates/%s/%s.tga", m_iszLevelTheme.c_str(), m_iszRoomTemplate.c_str() );
	m_pRoomTGAPanel->SetTGA( buffer );

	// check for updating our grid images
	//if (m_pRoomTemplate->GetTilesX() != m_iLastTilesX || m_pRoomTemplate->GetTilesY() != m_iLastTilesY)
	{
		m_iLastTilesX = pRoomTemplate->GetTilesX();
		m_iLastTilesY = pRoomTemplate->GetTilesY();

		// clear our grid images
		for (int i=0;i<m_pGridImagePanels.Count();i++)
		{
			m_pGridImagePanels[i]->SetVisible(false);
			m_pGridImagePanels[i]->MarkForDeletion();
		}
		m_pGridImagePanels.RemoveAll();
		
		// create enough grid image panels for each square
		if (g_pTileGenDialog->m_bShowTileSquares || m_bForceShowTileSquares)
		{
			int iGrids = pRoomTemplate->GetArea();
			for (int i=0;i<iGrids;i++)
			{
				CMissionChooserTGAImagePanel *pGridImagePanel = new CMissionChooserTGAImagePanel(this, "GridImagePanel");
				pGridImagePanel->SetMouseInputEnabled(false);
				pGridImagePanel->SetTGA("tilegen/roomtemplates/gridsquare.tga");		
				m_pGridImagePanels.AddToTail(pGridImagePanel);
			}
		}		
	}

	if (g_pTileGenDialog->m_bShowExits || m_bForceShowExits)
	{
		// create enough exit image panels for each exit in the room
		int iExits = pRoomTemplate->m_Exits.Count();
		for (int i=0;i<iExits;i++)
		{
			CRoomTemplateExit *pExit = pRoomTemplate->m_Exits[i];
			CMissionChooserTGAImagePanel *pExitImagePanel = new CMissionChooserTGAImagePanel(this, "ExitImagePanel");
			pExitImagePanel->SetMouseInputEnabled(false);
			switch (pExit->m_ExitDirection)
			{
				case EXITDIR_NORTH:
					pExitImagePanel->SetTGA("tilegen/roomtemplates/exitnorth.tga");
					break;
				case EXITDIR_EAST:
					pExitImagePanel->SetTGA("tilegen/roomtemplates/exiteast.tga");
					break;
				case EXITDIR_SOUTH:
					pExitImagePanel->SetTGA("tilegen/roomtemplates/exitsouth.tga");
					break;
				case EXITDIR_WEST:
				default:			
					pExitImagePanel->SetTGA("tilegen/roomtemplates/exitwest.tga");
					break;

			}
			m_pExitImagePanels.AddToTail(pExitImagePanel);
		}
	}

	InvalidateLayout(true, true);
	Repaint();
}

void CRoomTemplatePanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pSelectedOutline->SetBorder(pScheme->GetBorder("SelectedRoomBorder"));
	m_pSelectedOutline->SetPaintBackgroundEnabled(false);

	m_pTagsLabel->SetPaintBackgroundEnabled(false);
	m_pTagsLabel->SetFont( pScheme->GetFont( "DefaultVerySmall" ) );

	if (m_bRoomTemplateBrowserMode)
	{
		if (g_pTileGenDialog && g_pTileGenDialog->m_pCursorTemplate == GetRoomTemplate())
		{
			//SetPaintBackgroundEnabled(true);
			//SetPaintBackgroundType(0);
			//SetBgColor(Color(255,255,255,255));
			//InvalidateLayout(true);
			m_pSelectedOutline->SetVisible(true);
		}
		else
		{
			//SetPaintBackgroundEnabled(false);
			m_pSelectedOutline->SetVisible(false);
		}
	}	
}

void CRoomTemplatePanel::PerformLayout()
{
	BaseClass::PerformLayout();

	CRoomTemplate *pRoomTemplate = GetRoomTemplate();
	if (!pRoomTemplate || !g_pTileGenDialog)
		return;

	int tile_size = g_pTileGenDialog->RoomTemplatePanelTileSize();

	SetSize(pRoomTemplate->GetTilesX() * tile_size , pRoomTemplate->GetTilesY() * tile_size);

	m_pRoomTGAPanel->SetBounds(0, 0, GetWide(), GetTall());
	m_pSelectedOutline->SetBounds(0, 0, GetWide(), GetTall());

	// arrange the grid squares across our size
	for (int y=0; y<pRoomTemplate->GetTilesY(); y++)
	{
		for (int x=0; x<pRoomTemplate->GetTilesX(); x++)
		{
			int iGridImage = x + pRoomTemplate->GetTilesX() * y;
			if (m_pGridImagePanels.Count() > iGridImage)
			{
				m_pGridImagePanels[iGridImage]->SetBounds(x * tile_size, y * tile_size, tile_size, tile_size);
			}
		}
	}

	// arrange exits
	for (int i=0;i<pRoomTemplate->m_Exits.Count();i++)
	{
		CRoomTemplateExit *pExit = pRoomTemplate->m_Exits[i];
		if (!pExit || m_pExitImagePanels.Count() <= i)
			continue;

		int x = pExit->m_iXPos;
		//int y = (m_pRoomTemplate->GetTilesY()-1) - pExit->m_iYPos;		// reverse the y-axis to match hammer's coord style
		int y = pExit->m_iYPos;
		m_pExitImagePanels[i]->SetBounds(x * tile_size, y * tile_size, tile_size, tile_size);
	}

	m_pTagsLabel->SizeToContents();
	m_pTagsLabel->SetPos( 4, 0 );
	m_pTagsLabel->InvalidateLayout( true );
}

void CRoomTemplatePanel::OnMouseReleased(vgui::MouseCode code)
{
	if (m_bRoomTemplateEditMode && code == MOUSE_RIGHT)
	{
		// find which tile we clicked in
		int rx, ry;
		rx = ry = 0;
		LocalToScreen( rx, ry );
		int mx, my;
		input()->GetCursorPos(mx, my);
		mx = mx - rx;
		my = my - ry;
		if (mx < 0 || my < 0)
			return;

		if ( m_hMenu.Get() != NULL )
		{
			m_hMenu->MarkForDeletion();
			m_hMenu = NULL;
		}

		int tile_size = g_pTileGenDialog->RoomTemplatePanelTileSize();
		int iTileX = mx / tile_size;
		int iTileY = my / tile_size;

		vgui::Panel *parent = GetParent();
		CRoomTemplateEditDialog *pEdit = dynamic_cast<CRoomTemplateEditDialog*>(parent);
		if (pEdit)
		{
			pEdit->m_iSelectedTileX = iTileX;
			pEdit->m_iSelectedTileY = iTileY; //(m_pRoomTemplate->GetTilesY()-1) - iTileY;	// reverse y axis

			// find exits in this tile at this spot
			const CRoomTemplate *pTemplate = pEdit->m_pRoomTemplate;
			int iExits = pEdit->m_pRoomTemplate->m_Exits.Count();
			bool bHasNorthExit = false, bHasEastExit = false, bHasSouthExit = false, bHasWestExit = false;
			for (int i=iExits-1;i>=0;i--)
			{
				CRoomTemplateExit *pExit = pTemplate->m_Exits[i];
				if (pExit->m_iXPos == iTileX && pExit->m_iYPos == iTileY)
				{
					switch( pExit->m_ExitDirection )
					{
						case EXITDIR_NORTH: bHasNorthExit = true; break;
						case EXITDIR_EAST: bHasEastExit = true; break;
						case EXITDIR_SOUTH: bHasSouthExit = true; break;
						case EXITDIR_WEST: bHasWestExit = true; break;
					}
				}
			}

			vgui::Menu *pEditExitMenu = new Menu(this, "EditExitMenu");

			if ( bHasNorthExit )
			{
				pEditExitMenu->AddMenuItem("North exit", "EditExitNorth", parent);
			}			
			if ( bHasEastExit )
			{
				pEditExitMenu->AddMenuItem("East exit", "EditExitEast", parent);
			}
			if ( bHasSouthExit )
			{
				pEditExitMenu->AddMenuItem("South exit", "EditExitSouth", parent);
			}
			if ( bHasWestExit )
			{
				pEditExitMenu->AddMenuItem("West exit", "EditExitWest", parent);
			}

			vgui::Menu *pAddExitMenu = new Menu(this, "AddExitMenu");
			if ( !bHasNorthExit )
			{
				pAddExitMenu->AddMenuItem("To the north", "AddExitNorth", parent);
			}
			if ( !bHasEastExit )
			{
				pAddExitMenu->AddMenuItem("To the east", "AddExitEast", parent);
			}
			if ( !bHasSouthExit )
			{
				pAddExitMenu->AddMenuItem("To the south", "AddExitSouth", parent);
			}
			if ( !bHasWestExit )
			{
				pAddExitMenu->AddMenuItem("To the west", "AddExitWest", parent);
			}

			vgui::Menu *pRightClickMenu = new Menu(this, "RightClickMenu");				
			pRightClickMenu->AddMenuItem("Clear all exits from this tile", "ClearExitsFromTile" , parent);
			pRightClickMenu->AddMenuItem("Clear all exits from this room template", "ClearAllExits", parent);
			pRightClickMenu->AddCascadingMenuItem( "AddExit", "Add exit to this tile...", "", this, pAddExitMenu );
			pRightClickMenu->AddCascadingMenuItem( "EditExit", "Exit properties...", "", this, pEditExitMenu );
			pRightClickMenu->SetPos( mx+rx, my+ry );
			pRightClickMenu->SetVisible( true );
			m_hMenu = pRightClickMenu;
		}
		else
		{
			MessageBox *pMessage = new MessageBox ("Error", "Couldn't cast RoomTemplatePanel's parent to RoomTemplateEditDialog", this);
			pMessage->DoModal();
		}
		
		//char buffer[256];
		//Q_snprintf(buffer, sizeof(buffer), "tilex = %d tiley = %d\n", iTileX, iTileY);
		//MessageBox *pMessage = new MessageBox ("Right clicked roomtemplatepanel", buffer, this);
		//pMessage->DoModal();
		return;
	}
	if (m_bRoomTemplateBrowserMode)
	{
		if (code == MOUSE_LEFT)
		{
			g_pTileGenDialog->SetCursorRoomTemplate( GetRoomTemplate() );
		}
		else if (code == MOUSE_RIGHT)
		{
			if (GetRoomTemplate())
			{
				CRoomTemplateEditDialog *pDialog = new CRoomTemplateEditDialog(g_pTileGenDialog, "RoomTemplateEditDialog", GetRoomTemplate(), false);

				pDialog->AddActionSignalTarget( g_pTileGenDialog );
				pDialog->DoModal();
			}
		}
	}
	BaseClass::OnMouseReleased(code);
}
void CRoomTemplatePanel::UpdateAllImages()
{
	int iCount = g_pRoomTemplatePanels.Count();
	for (int i=0;i<iCount;i++)
	{
		CRoomTemplatePanel *pPanel = g_pRoomTemplatePanels[i];
		if (pPanel)
			pPanel->UpdateImages();
	}
}