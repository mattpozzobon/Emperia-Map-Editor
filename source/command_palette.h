//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_COMMAND_PALETTE_H_
#define RME_COMMAND_PALETTE_H_

#include "main_menubar.h"
#include "position.h"

#include <optional>

class Brush;

class CommandPaletteDialog final : public wxDialog
{
public:
	CommandPaletteDialog(wxWindow* parent, const std::vector<wxString>& recent_maps);

	MenuBar::ActionID GetSelectedAction() const noexcept;
	const Brush* GetSelectedBrush() const noexcept;
	const std::optional<Position>& GetSelectedPosition() const noexcept;
	const wxString& GetSelectedMapPath() const noexcept;

private:
	struct Command
	{
		wxString label;
		wxString category;
		wxString keywords;
		MenuBar::ActionID action;
		bool requires_editor;
		bool requires_version;
		const Brush* brush = nullptr;
		std::optional<Position> position;
		wxString map_path;
	};

	void RefreshResults();
	void AcceptSelection();
	void OnSearch(wxCommandEvent& event);
	void OnSearchKeyDown(wxKeyEvent& event);
	void OnListKeyDown(wxKeyEvent& event);
	void OnListDoubleClick(wxCommandEvent& event);

	wxTextCtrl* search;
	wxListBox* results;
	wxStaticText* result_count;

	std::vector<Command> commands;
	std::vector<Command> dynamic_commands;
	std::vector<const Command*> visible_commands;
	MenuBar::ActionID selected_action;
	const Brush* selected_brush;
	std::optional<Position> selected_position;
	wxString selected_map_path;
};

#endif
