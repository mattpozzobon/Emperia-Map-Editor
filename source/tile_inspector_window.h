//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_TILE_INSPECTOR_WINDOW_H_
#define RME_TILE_INSPECTOR_WINDOW_H_

#include "position.h"

class Editor;

class TileInspectorWindow final : public wxScrolledWindow
{
public:
	explicit TileInspectorWindow(wxWindow* parent);

	void SetTile(const Editor* editor, const Position& position);
	void Clear();

private:
	wxStaticText* AddValueRow(wxFlexGridSizer* grid, const wxString& label);
	void SetValue(wxStaticText* control, const wxString& value);
	void OnEditProperties(wxCommandEvent& event);
	void OnCopyPosition(wxCommandEvent& event);
	void OnCopyIds(wxCommandEvent& event);

	Position inspected_position;
	bool has_position;
	bool has_tile;
	wxString ids_text;
	wxStaticText* position_value;
	wxStaticText* ground_value;
	wxStaticText* creature_value;
	wxStaticText* spawn_value;
	wxStaticText* house_value;
	wxStaticText* zone_value;
	wxStaticText* state_value;
	wxStaticText* item_count_value;
	wxListBox* contents;
	wxButton* edit_button;
	wxButton* copy_position_button;
	wxButton* copy_ids_button;
};

#endif
