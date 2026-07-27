//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef RME_OLD_PROPERTIES_WINDOW_H_
#define RME_OLD_PROPERTIES_WINDOW_H_

#include "main.h"

#include "common_windows.h"
#include <wx/webrequest.h>

class ContainerItemButton;
class ContainerItemPopupMenu;

class OldPropertiesWindow : public ObjectPropertiesWindowBase
{
public:
	OldPropertiesWindow(wxWindow* parent, const Map* map, const Tile* tile, Item* item, wxPoint = wxDefaultPosition);
	OldPropertiesWindow(wxWindow* parent, const Map* map, const Tile* tile, Creature* creature, wxPoint = wxDefaultPosition);
	OldPropertiesWindow(wxWindow* parent, const Map* map, const Tile* tile, Spawn* spawn, wxPoint = wxDefaultPosition);
	virtual ~OldPropertiesWindow();

	void OnFocusChange(wxFocusEvent&);

	void OnClickOK(wxCommandEvent&);
	void OnClickCancel(wxCommandEvent&);
	void OnTranslateMapText(wxCommandEvent&);
	void OnTranslationRequestState(wxWebRequestEvent&);

	void Update();

protected:
	wxSpinCtrl* count_field;
	wxChoice* direction_field;
	wxSpinCtrl* action_id_field;
	wxSpinCtrl* unique_id_field;
	wxSpinCtrl* door_id_field;
	wxChoice* depot_id_field;
	PositionCtrl* destination_field;
	wxChoice* splash_type_field;
	wxTextCtrl* text_field;
	wxTextCtrl* localized_text_fields[3];
	wxTextCtrl* translation_token_field;
	wxButton* translate_text_button;
	std::string map_text_key;
	std::vector<wxWebRequest> translation_requests;
	int pending_translation_requests;
	wxTextCtrl* description_field;
	std::vector<ContainerItemButton*> container_items;

	friend class ContainerItemButton;
	friend class ContainerItemPopupMenu;

	DECLARE_EVENT_TABLE();
};

#endif

