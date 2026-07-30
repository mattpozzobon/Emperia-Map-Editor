//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "command_palette.h"
#include "brush.h"
#include "gui.h"

#include <set>

namespace
{
	int MatchScore(const wxString& haystack, const wxArrayString& terms)
	{
		int score = 0;
		for(const wxString& term : terms) {
			const int position = haystack.Find(term);
			if(position == wxNOT_FOUND) {
				return -1;
			}
			score += position == 0 ? 100 : std::max(1, 40 - position);
		}
		return score;
	}
}

CommandPaletteDialog::CommandPaletteDialog(wxWindow* parent, const std::vector<wxString>& recent_maps) :
	wxDialog(parent, wxID_ANY, "Command Palette", wxDefaultPosition, FROM_DIP(parent, wxSize(620, 440)),
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	search(nullptr),
	results(nullptr),
	result_count(nullptr),
	selected_action(MenuBar::COMMAND_PALETTE),
	selected_brush(nullptr),
	selected_position(std::nullopt)
{
	using namespace MenuBar;

	commands = {
		{"New map", "File", "create blank", NEW, false, false},
		{"Open map...", "File", "load file", OPEN, false, false},
		{"Save map", "File", "write", SAVE, true, true},
		{"Save map as...", "File", "write copy", SAVE_AS, true, true},
		{"Preferences", "Application", "settings options", PREFERENCES, false, false},
		{"Undo", "Edit", "history revert", UNDO, true, true},
		{"Redo", "Edit", "history repeat", REDO, true, true},
		{"Find item...", "Search", "locate search", FIND_ITEM, true, true},
		{"Replace items...", "Search", "find change", REPLACE_ITEMS, true, true},
		{"Jump to brush...", "Navigate", "find palette asset", JUMP_TO_BRUSH, false, true},
		{"Jump to item brush...", "Navigate", "find raw id", JUMP_TO_ITEM_BRUSH, false, true},
		{"Go to position...", "Navigate", "coordinates xyz", GOTO_POSITION, true, true},
		{"Go to previous position", "Navigate", "back location", GOTO_PREVIOUS_POSITION, true, true},
		{"Terrain palette", "Palette", "ground", SELECT_TERRAIN, true, true},
		{"Doodad palette", "Palette", "decoration", SELECT_DOODAD, true, true},
		{"Item palette", "Palette", "objects", SELECT_ITEM, true, true},
		{"Creature palette", "Palette", "monster npc", SELECT_CREATURE, true, true},
		{"House palette", "Palette", "homes", SELECT_HOUSE, true, true},
		{"Waypoint palette", "Palette", "marker", SELECT_WAYPOINT, true, true},
		{"RAW palette", "Palette", "item id", SELECT_RAW, true, true},
		{"New palette", "Window", "panel assets", NEW_PALETTE, true, true},
		{"New map view", "Window", "split duplicate", NEW_VIEW, true, true},
		{"Minimap", "Window", "overview", WIN_MINIMAP, true, true},
		{"Actions history", "Window", "undo log", WIN_ACTIONS_HISTORY, true, true},
		{"Tile inspector", "Window", "properties contents context", WIN_INSPECTOR, true, true},
		{"Toggle fullscreen", "Window", "display", TOGGLE_FULLSCREEN, false, false},
		{"Zoom in", "View", "closer", ZOOM_IN, true, true},
		{"Zoom out", "View", "farther", ZOOM_OUT, true, true},
		{"Reset zoom", "View", "normal 100 percent", ZOOM_NORMAL, true, true},
		{"Toggle grid", "View", "tiles lines", SHOW_GRID, true, true},
		{"Toggle shade", "View", "floors shadow", SHOW_SHADE, true, true},
		{"Toggle all floors", "View", "layers", SHOW_ALL_FLOORS, true, true},
		{"Toggle creatures", "View", "monster npc", SHOW_CREATURES, true, true},
		{"Toggle spawns", "View", "monster area", SHOW_SPAWNS, true, true},
		{"Toggle houses", "View", "homes overlay", SHOW_HOUSES, true, true},
		{"Toggle zones", "View", "world overlay", SHOW_ZONES, true, true},
		{"Toggle pathing", "View", "blocking walkability", SHOW_PATHING, true, true},
		{"Toggle tooltips", "View", "hover details", SHOW_TOOLTIPS, true, true},
		{"Toggle animation preview", "View", "lights animate", SHOW_PREVIEW, true, true},
		{"Map properties...", "Map", "metadata version", MAP_PROPERTIES, true, true},
		{"Zone configuration...", "Map", "world regions", MAP_ZONE_CONFIG, true, true},
		{"Map statistics", "Map", "counts information", MAP_STATISTICS, true, true},
	};

	for(const wxString& path : recent_maps) {
		if(path.IsEmpty()) {
			continue;
		}
		const wxFileName file(path);
		commands.push_back({
			"Open " + file.GetFullName(),
			"Recent",
			"map file " + path,
			MenuBar::OPEN,
			false,
			false,
			nullptr,
			std::nullopt,
			path
		});
	}

	wxBoxSizer* root_sizer = newd wxBoxSizer(wxVERTICAL);

	search = newd wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	search->SetHint("Type a command, action, palette, or setting");
	search->SetName("Command search");
	root_sizer->Add(search, 0, wxEXPAND | wxALL, FROM_DIP(this, 10));

	results = newd wxListBox(this, wxID_ANY);
	results->SetName("Matching commands");
	root_sizer->Add(results, 1, wxEXPAND | wxLEFT | wxRIGHT, FROM_DIP(this, 10));

	result_count = newd wxStaticText(this, wxID_ANY, wxEmptyString);
	root_sizer->Add(result_count, 0, wxEXPAND | wxALL, FROM_DIP(this, 10));

	SetSizer(root_sizer);
	SetMinSize(FROM_DIP(this, wxSize(420, 300)));
	CentreOnParent();

	search->Bind(wxEVT_TEXT, &CommandPaletteDialog::OnSearch, this);
	search->Bind(wxEVT_KEY_DOWN, &CommandPaletteDialog::OnSearchKeyDown, this);
	results->Bind(wxEVT_KEY_DOWN, &CommandPaletteDialog::OnListKeyDown, this);
	results->Bind(wxEVT_LISTBOX_DCLICK, &CommandPaletteDialog::OnListDoubleClick, this);

	RefreshResults();
	search->SetFocus();
}

MenuBar::ActionID CommandPaletteDialog::GetSelectedAction() const noexcept
{
	return selected_action;
}

const Brush* CommandPaletteDialog::GetSelectedBrush() const noexcept
{
	return selected_brush;
}

const std::optional<Position>& CommandPaletteDialog::GetSelectedPosition() const noexcept
{
	return selected_position;
}

const wxString& CommandPaletteDialog::GetSelectedMapPath() const noexcept
{
	return selected_map_path;
}

void CommandPaletteDialog::RefreshResults()
{
	struct Match
	{
		const Command* command;
		int score;
	};

	const wxArrayString terms = wxSplit(search->GetValue().Lower(), ' ', '\0');
	const wxString query = search->GetValue().Lower().Strip(wxString::both);
	const bool has_editor = g_gui.IsEditorOpen();
	const bool has_version = g_gui.IsVersionLoaded();
	std::vector<Match> matches;
	dynamic_commands.clear();
	dynamic_commands.reserve(24);

	for(const Command& command : commands) {
		if((command.requires_editor && !has_editor) || (command.requires_version && !has_version)) {
			continue;
		}

		const wxString searchable =
			(command.label + " " + command.category + " " + command.keywords).Lower();
		const int score = MatchScore(searchable, terms);
		if(score >= 0) {
			matches.push_back({&command, score});
		}
	}

	if(has_editor && !query.IsEmpty()) {
		const std::regex position_pattern(
			R"(^\s*(?:(?:go|goto)\s+)?([0-9]+)[,\s:]+([0-9]+)[,\s:]+([0-9]+)\s*$)",
			std::regex::icase
		);
		std::smatch position_match;
		const std::string position_query = nstr(query);
		if(std::regex_match(position_query, position_match, position_pattern)) {
			try {
				const Position position(
					std::stoi(position_match[1].str()),
					std::stoi(position_match[2].str()),
					std::stoi(position_match[3].str())
				);
				if(position.isValid()) {
					dynamic_commands.push_back({
						wxString::Format("Go to %d, %d, %d", position.x, position.y, position.z),
						"Navigate",
						"coordinates xyz direct",
						MenuBar::COMMAND_PALETTE,
						true,
						true,
						nullptr,
						position
					});
					matches.push_back({&dynamic_commands.back(), 1000});
				}
			} catch(const std::out_of_range&) {
				// Ignore coordinates outside the integer range.
			}
		}
	}

	if(has_version && query.size() >= 2) {
		std::set<const Brush*> added_brushes;
		const BrushMap& brush_map = g_brushes.getMap();
		for(const auto& entry : brush_map) {
			const Brush* brush = entry.second;
			if(!brush || !brush->visibleInPalette() || !added_brushes.insert(brush).second) {
				continue;
			}

			const wxString brush_name = wxstr(brush->getName());
			const wxString searchable = (
				brush_name + " " +
				wxString::Format("%d %u", brush->getLookID(), brush->getID())
			).Lower();
			const int score = MatchScore(searchable, terms);
			if(score < 0) {
				continue;
			}

			dynamic_commands.push_back({
				brush_name,
				brush->isRaw() ? "Item" : "Brush",
				wxString::Format("id %d internal %u", brush->getLookID(), brush->getID()),
				MenuBar::COMMAND_PALETTE,
				false,
				true,
				brush,
				std::nullopt
			});
			matches.push_back({&dynamic_commands.back(), score + 60});
			if(dynamic_commands.size() >= 24) {
				break;
			}
		}
	}

	std::stable_sort(matches.begin(), matches.end(), [](const Match& left, const Match& right) {
		return left.score > right.score;
	});

	results->Freeze();
	results->Clear();
	visible_commands.clear();
	for(const Match& match : matches) {
		results->Append(match.command->category + ": " + match.command->label);
		visible_commands.push_back(match.command);
	}
	if(!visible_commands.empty()) {
		results->SetSelection(0);
	}
	results->Thaw();

	result_count->SetLabel(wxString::Format(
		visible_commands.size() == 1 ? "%zu command" : "%zu commands",
		visible_commands.size()
	));
}

void CommandPaletteDialog::AcceptSelection()
{
	const int selection = results->GetSelection();
	if(selection == wxNOT_FOUND || static_cast<size_t>(selection) >= visible_commands.size()) {
		return;
	}

	selected_action = visible_commands[selection]->action;
	selected_brush = visible_commands[selection]->brush;
	selected_position = visible_commands[selection]->position;
	selected_map_path = visible_commands[selection]->map_path;
	EndModal(wxID_OK);
}

void CommandPaletteDialog::OnSearch(wxCommandEvent& event)
{
	RefreshResults();
}

void CommandPaletteDialog::OnSearchKeyDown(wxKeyEvent& event)
{
	const int selection = results->GetSelection();
	switch(event.GetKeyCode()) {
		case WXK_DOWN:
			if(!visible_commands.empty()) {
				results->SetSelection(std::min<int>(selection + 1, visible_commands.size() - 1));
			}
			break;
		case WXK_UP:
			if(!visible_commands.empty()) {
				results->SetSelection(std::max(0, selection - 1));
			}
			break;
		case WXK_RETURN:
		case WXK_NUMPAD_ENTER:
			AcceptSelection();
			break;
		case WXK_ESCAPE:
			EndModal(wxID_CANCEL);
			break;
		default:
			event.Skip();
			break;
	}
}

void CommandPaletteDialog::OnListKeyDown(wxKeyEvent& event)
{
	switch(event.GetKeyCode()) {
		case WXK_RETURN:
		case WXK_NUMPAD_ENTER:
			AcceptSelection();
			break;
		case WXK_ESCAPE:
			EndModal(wxID_CANCEL);
			break;
		default:
			event.Skip();
			break;
	}
}

void CommandPaletteDialog::OnListDoubleClick(wxCommandEvent& event)
{
	AcceptSelection();
}
