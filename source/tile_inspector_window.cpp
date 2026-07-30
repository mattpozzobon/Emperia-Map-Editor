//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "tile_inspector_window.h"

#include "creature.h"
#include "editor.h"
#include "gui.h"
#include "item.h"
#include "map.h"
#include "map_display.h"
#include "map_tab.h"
#include "settings.h"
#include "spawn.h"
#include "tile.h"

#include <wx/clipbrd.h>
#include <wx/wrapsizer.h>

namespace
{
	void AppendState(wxString& states, const wxString& state)
	{
		if(!states.IsEmpty()) {
			states += ", ";
		}
		states += state;
	}

	wxString DescribeItem(const wxString& prefix, const Item* item)
	{
		if(!item) {
			return wxString("None");
		}

		wxString description = wxString::Format(
			"%s%s [ID %u, appearance %u]",
			prefix,
			wxstr(item->getName()),
			item->getID(),
			item->getAppearanceID()
		);

		wxString access;
		if(item->getMinimumLevel() > 0) {
			AppendState(access, wxString::Format("level %u+", item->getMinimumLevel()));
		}
		if(item->isNobleOnly()) AppendState(access, "Noble");
		if(!item->getRequiredQuests().empty()) {
			AppendState(access, "quests: " + wxstr(item->getRequiredQuests()));
		}
		if(!item->getRequiredStorage().empty()) {
			AppendState(access, "storage: " + wxstr(item->getRequiredStorage()));
		}
		if(!access.IsEmpty()) description += " {Access: " + access + "}";
		return description;
	}
}

TileInspectorWindow::TileInspectorWindow(wxWindow* parent) :
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL),
	inspected_position(),
	has_position(false),
	has_tile(false),
	position_value(nullptr),
	ground_value(nullptr),
	creature_value(nullptr),
	spawn_value(nullptr),
	house_value(nullptr),
	zone_value(nullptr),
	state_value(nullptr),
	item_count_value(nullptr),
	contents(nullptr),
	edit_button(nullptr),
	copy_position_button(nullptr),
	copy_ids_button(nullptr)
{
	const int density = std::clamp(g_settings.getInteger(Config::UI_DENSITY), 0, 2);
	const int panel_widths[] = {210, 230, 250};
	const int panel_heights[] = {240, 260, 280};
	const int contents_widths[] = {190, 210, 230};
	const int contents_heights[] = {120, 140, 160};
	SetMinSize(FROM_DIP(this, wxSize(panel_widths[density], panel_heights[density])));
	SetScrollRate(0, FROM_DIP(this, 12));

	wxBoxSizer* root_sizer = newd wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* details = newd wxFlexGridSizer(2, FROM_DIP(this, 5), FROM_DIP(this, 10));
	details->AddGrowableCol(1, 1);

	position_value = AddValueRow(details, "Position");
	ground_value = AddValueRow(details, "Ground");
	creature_value = AddValueRow(details, "Creature");
	spawn_value = AddValueRow(details, "Spawn");
	house_value = AddValueRow(details, "House");
	zone_value = AddValueRow(details, "Zone");
	state_value = AddValueRow(details, "State");
	item_count_value = AddValueRow(details, "Contents");

	root_sizer->Add(details, 0, wxEXPAND | wxALL, FROM_DIP(this, 10));
	root_sizer->Add(newd wxStaticText(this, wxID_ANY, "Tile contents"), 0,
		wxLEFT | wxRIGHT | wxTOP, FROM_DIP(this, 10));

	contents = newd wxListBox(this, wxID_ANY);
	contents->SetName("Tile contents");
	contents->SetMinSize(FROM_DIP(this, wxSize(contents_widths[density], contents_heights[density])));
	root_sizer->Add(contents, 1, wxEXPAND | wxALL, FROM_DIP(this, 10));

	wxWrapSizer* actions = newd wxWrapSizer(wxHORIZONTAL);
	edit_button = newd wxButton(this, wxID_ANY, "Edit properties");
	copy_position_button = newd wxButton(this, wxID_ANY, "Copy position");
	copy_ids_button = newd wxButton(this, wxID_ANY, "Copy IDs");
	edit_button->SetToolTip("Edit properties for the inspected tile's top visible object.");
	copy_position_button->SetToolTip("Copy the inspected XYZ position.");
	copy_ids_button->SetToolTip("Copy server and appearance IDs for the inspected tile contents.");
	actions->Add(edit_button, 0, wxRIGHT, FROM_DIP(this, 5));
	actions->Add(copy_position_button, 0, wxRIGHT, FROM_DIP(this, 5));
	actions->Add(copy_ids_button, 0);
	root_sizer->Add(actions, 0, wxLEFT | wxRIGHT | wxBOTTOM, FROM_DIP(this, 10));

	SetSizer(root_sizer);
	edit_button->Bind(wxEVT_BUTTON, &TileInspectorWindow::OnEditProperties, this);
	copy_position_button->Bind(wxEVT_BUTTON, &TileInspectorWindow::OnCopyPosition, this);
	copy_ids_button->Bind(wxEVT_BUTTON, &TileInspectorWindow::OnCopyIds, this);
	Clear();
}

wxStaticText* TileInspectorWindow::AddValueRow(wxFlexGridSizer* grid, const wxString& label)
{
	wxStaticText* name = newd wxStaticText(this, wxID_ANY, label + ":");
	wxFont font = name->GetFont();
	font.SetWeight(wxFONTWEIGHT_BOLD);
	name->SetFont(font);
	grid->Add(name, 0, wxALIGN_TOP);

	wxStaticText* value = newd wxStaticText(this, wxID_ANY, "-", wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
	value->Wrap(FROM_DIP(this, 190));
	grid->Add(value, 1, wxEXPAND);
	return value;
}

void TileInspectorWindow::SetValue(wxStaticText* control, const wxString& value)
{
	control->SetLabel(value.IsEmpty() ? wxString("-") : value);
	control->SetToolTip(value);
}

void TileInspectorWindow::Clear()
{
	has_position = false;
	has_tile = false;
	ids_text.clear();
	SetValue(position_value, "Move the cursor over a tile");
	SetValue(ground_value, "-");
	SetValue(creature_value, "-");
	SetValue(spawn_value, "-");
	SetValue(house_value, "-");
	SetValue(zone_value, "-");
	SetValue(state_value, "-");
	SetValue(item_count_value, "0");
	contents->Clear();
	edit_button->Enable(false);
	copy_position_button->Enable(false);
	copy_ids_button->Enable(false);
	Layout();
	FitInside();
}

void TileInspectorWindow::SetTile(const Editor* editor, const Position& position)
{
	if(!editor || position.x < 0 || position.y < 0 || position.z < 0) {
		Clear();
		return;
	}

	const Tile* tile = editor->getMap().getTile(position);
	inspected_position = position;
	has_position = true;
	has_tile = tile != nullptr;
	ids_text.clear();
	copy_position_button->Enable(true);
	copy_ids_button->Enable(false);
	edit_button->Enable(tile != nullptr && (tile->spawn || tile->creature || tile->getTopItem()));
	SetValue(position_value, wxString::Format("%d, %d, %d", position.x, position.y, position.z));
	contents->Freeze();
	contents->Clear();

	if(!tile) {
		SetValue(ground_value, "Empty tile");
		SetValue(creature_value, "-");
		SetValue(spawn_value, "-");
		SetValue(house_value, "-");
		SetValue(zone_value, "-");
		SetValue(state_value, "-");
		SetValue(item_count_value, "0");
		contents->Thaw();
		Layout();
		FitInside();
		return;
	}

	SetValue(ground_value, DescribeItem(wxEmptyString, tile->ground));
	if(tile->ground) {
		contents->Append(DescribeItem("Ground: ", tile->ground));
		ids_text += wxString::Format("Ground: server %u, appearance %u\n",
			tile->ground->getID(), tile->ground->getAppearanceID());
	}
	for(const Item* item : tile->items) {
		contents->Append(DescribeItem(wxEmptyString, item));
		ids_text += wxString::Format("Item: server %u, appearance %u\n",
			item->getID(), item->getAppearanceID());
	}

	if(tile->creature) {
		wxString creature = tile->creature->isNpc() ? "NPC: " : "Monster: ";
		creature += wxstr(tile->creature->getName());
		if(!tile->creature->getTitle().empty()) {
			creature += " (" + wxstr(tile->creature->getTitle()) + ")";
		}
		SetValue(creature_value, creature);
		contents->Append(creature);
	} else {
		SetValue(creature_value, "-");
	}

	SetValue(spawn_value, tile->spawn ?
		wxString::Format("Radius %d", tile->spawn->getSize()) : wxString("-"));
	SetValue(house_value, tile->isHouseTile() ?
		wxString::Format("House ID %u", tile->getHouseID()) : wxString("-"));

	const uint32_t flags = tile->getMapFlags();
	const std::string zone_category = getZoneCategoryFromFlags(flags);
	SetValue(zone_value, zone_category.empty() ?
		wxString("-") : wxstr(getZoneCategoryDisplayName(zone_category)));

	wxString states;
	if(tile->isPZ()) AppendState(states, "Protection zone");
	if(flags & TILESTATE_NOPVP) AppendState(states, "No PvP");
	if(flags & TILESTATE_NOLOGOUT) AppendState(states, "No logout");
	if(flags & TILESTATE_PVPZONE) AppendState(states, "PvP zone");
	if(tile->isBlocking()) AppendState(states, "Blocking");
	if(tile->isModified()) AppendState(states, "Modified");
	if(tile->isSelected()) AppendState(states, "Selected");
	SetValue(state_value, states);
	SetValue(item_count_value, wxString::Format("%d", tile->size()));
	copy_ids_button->Enable(!ids_text.IsEmpty());

	contents->Thaw();
	Layout();
	FitInside();
}

void TileInspectorWindow::OnEditProperties(wxCommandEvent& event)
{
	MapTab* tab = g_gui.GetCurrentMapTab();
	if(has_tile && tab && tab->GetCanvas()->EditTileProperties(inspected_position)) {
		SetTile(tab->GetEditor(), inspected_position);
	}
}

void TileInspectorWindow::OnCopyPosition(wxCommandEvent& event)
{
	if(!has_position || !wxTheClipboard->Open()) {
		return;
	}
	wxTheClipboard->SetData(newd wxTextDataObject(wxString::Format(
		"%d, %d, %d", inspected_position.x, inspected_position.y, inspected_position.z)));
	wxTheClipboard->Close();
}

void TileInspectorWindow::OnCopyIds(wxCommandEvent& event)
{
	if(ids_text.IsEmpty() || !wxTheClipboard->Open()) {
		return;
	}
	wxString text = ids_text;
	text.Trim();
	wxTheClipboard->SetData(newd wxTextDataObject(text));
	wxTheClipboard->Close();
}
