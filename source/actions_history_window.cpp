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

#include "main.h"
#include "actions_history_window.h"
#include "artprovider.h"
#include "editor.h"
#include "gui.h"
#include "settings.h"

#include <wx/settings.h>

HistoryListBox::HistoryListBox(wxWindow* parent) :
	wxVListBox(parent, wxID_ANY),
	icon_dip_size(18),
	row_dip_height(28),
	item_dip_padding(5)
{
	const int density = std::clamp(g_settings.getInteger(Config::UI_DENSITY), 0, 2);
	const int icon_sizes[] = {14, 18, 22};
	const int row_heights[] = {22, 28, 34};
	const int item_paddings[] = {4, 5, 6};
	icon_dip_size = icon_sizes[density];
	row_dip_height = row_heights[density];
	item_dip_padding = item_paddings[density];

	wxSize icon_size = FROM_DIP(parent, wxSize(icon_dip_size, icon_dip_size));
	open_bitmap = wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR, icon_size);
	move_bitmap = wxArtProvider::GetBitmap(ART_MOVE, wxART_LIST, icon_size);
	remote_bitmap = wxArtProvider::GetBitmap(ART_REMOTE, wxART_LIST, icon_size);
	select_bitmap = wxArtProvider::GetBitmap(ART_SELECT, wxART_LIST, icon_size);
	unselect_bitmap = wxArtProvider::GetBitmap(ART_UNSELECT, wxART_LIST, icon_size);
	delete_bitmap = wxArtProvider::GetBitmap(ART_DELETE, wxART_LIST, icon_size);
	cut_bitmap = wxArtProvider::GetBitmap(ART_CUT, wxART_LIST, icon_size);
	paste_bitmap = wxArtProvider::GetBitmap(ART_PASTE, wxART_LIST, icon_size);
	randomize_bitmap = wxArtProvider::GetBitmap(ART_RANDOMIZE, wxART_LIST, icon_size);
	borderize_bitmap = wxArtProvider::GetBitmap(ART_BORDERIZE, wxART_LIST, icon_size);
	draw_bitmap = wxArtProvider::GetBitmap(ART_DRAW, wxART_LIST, icon_size);
	erase_bitmap = wxArtProvider::GetBitmap(ART_ERASE, wxART_LIST, icon_size);
	switch_bitmap = wxArtProvider::GetBitmap(ART_SWITCH, wxART_LIST, icon_size);
	rotate_bitmap = wxArtProvider::GetBitmap(ART_ROTATE, wxART_LIST, icon_size);
	replace_bitmap = wxArtProvider::GetBitmap(ART_REPLACE, wxART_LIST, icon_size);
	change_bitmap = wxArtProvider::GetBitmap(ART_CHANGE, wxART_LIST, icon_size);
}

void HistoryListBox::OnDrawItem(wxDC& dc, const wxRect& rect, size_t index) const
{
	const Editor* editor = g_gui.GetCurrentEditor();
	if(!editor) {
		return;
	}

	const ActionQueue* actions = editor->getHistoryActions();
	if(!actions) {
		return;
	}

	const wxSystemColour text_colour = IsSelected(index) ?
		wxSYS_COLOUR_HIGHLIGHTTEXT : wxSYS_COLOUR_WINDOWTEXT;
	dc.SetTextForeground(wxSystemSettings::GetColour(text_colour));

	const BatchAction* action = actions->getAction(index - 1);
	const int padding = FROM_DIP(this, item_dip_padding);
	const int text_x = rect.GetX() + FROM_DIP(this, icon_dip_size + item_dip_padding * 2);
	const wxString label = action ? action->getLabel() : wxString("Open Map");
	wxCoord text_height;
	dc.GetTextExtent(label, nullptr, &text_height);
	const int icon_y = rect.GetY() + std::max(0, (rect.GetHeight() - FROM_DIP(this, icon_dip_size)) / 2);
	const int text_y = rect.GetY() + std::max(0, (rect.GetHeight() - text_height) / 2);

	if(action) {
		const wxBitmap& bitmap = getIconBitmap(action->getType());
		dc.DrawBitmap(bitmap, rect.GetX() + padding, icon_y, true);
	} else {
		dc.DrawBitmap(open_bitmap, rect.GetX() + padding, icon_y, true);
	}
	dc.DrawText(label, text_x, text_y);
}

wxCoord HistoryListBox::OnMeasureItem(size_t index) const
{
	return FROM_DIP(this, row_dip_height);
}

const wxBitmap& HistoryListBox::getIconBitmap(ActionIdentifier identifier) const
{
	switch (identifier)
	{
		case ACTION_MOVE:
			return move_bitmap;
		case ACTION_REMOTE:
			return remote_bitmap;
		case ACTION_SELECT:
			return select_bitmap;
		case ACTION_UNSELECT:
			return unselect_bitmap;
		case ACTION_DELETE_TILES:
			return delete_bitmap;
		case ACTION_CUT_TILES:
			return cut_bitmap;
		case ACTION_PASTE_TILES:
			return paste_bitmap;
		case ACTION_RANDOMIZE:
			return randomize_bitmap;
		case ACTION_BORDERIZE:
			return borderize_bitmap;
		case ACTION_DRAW:
			return draw_bitmap;
		case ACTION_ERASE:
			return erase_bitmap;
		case ACTION_SWITCHDOOR:
			return switch_bitmap;
		case ACTION_ROTATE_ITEM:
			return rotate_bitmap;
		case ACTION_REPLACE_ITEMS:
			return replace_bitmap;
		case ACTION_CHANGE_PROPERTIES:
			return change_bitmap;
		default:
			return wxNullBitmap;
	}
}

ActionsHistoryWindow::ActionsHistoryWindow(wxWindow* parent) :
	wxPanel(parent, wxID_ANY)
{
	SetMinSize(FROM_DIP(this, wxSize(210, 220)));

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	list = new HistoryListBox(this);
	list->SetCanFocus(false);
	sizer->Add(list, 1, wxEXPAND, 5);

	SetSizer(sizer);
	Layout();

	// Connect Events
	list->Connect(wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(ActionsHistoryWindow::OnListSelected), NULL, this);
}

ActionsHistoryWindow::~ActionsHistoryWindow()
{
	// Disconnect Events
	list->Disconnect(wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(ActionsHistoryWindow::OnListSelected), NULL, this);
}

void ActionsHistoryWindow::RefreshActions()
{
	if(!IsShownOnScreen())
		return;

	const Editor* editor = g_gui.GetCurrentEditor();
	if(!editor) {
		list->SetItemCount(0);
		list->Refresh();
		return;
	}

	size_t count = 1;
	int selection = 0;

	const ActionQueue* actions = editor->getHistoryActions();
	if(actions) {
		count += actions->size();
		selection += actions->getCurrentIndex();
	}

	list->SetItemCount(count);
	list->SetSelection(selection);
	list->Refresh();
}

void ActionsHistoryWindow::OnListSelected(wxCommandEvent& event)
{
	int index = list->GetSelection();
	if(index == wxNOT_FOUND)
		return;

	Editor* editor = g_gui.GetCurrentEditor();
	if(editor && editor->getHistoryActions()) {
		int current = editor->getHistoryActions()->getCurrentIndex();
		if(index > current) {
			editor->redo(index - current);
		} else if (index < current) {
			editor->undo(current - index);
		}
	}
}
